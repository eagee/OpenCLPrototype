#include "OpenClProgram.h"
#include <QtCore>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <CL/cl.h>

OpenClProgram::OpenClProgram(const QString programFileName, const QString functionName, DeviceType::enum_type deviceType, QObject *parent /*= nullptr*/):
    QObject(parent),
    m_programFileName(programFileName),
    m_kernelFunctionName(functionName),
    m_deviceType(deviceType),
    m_errorState(ErrorState::None),
    m_workItemsToExecute(2048), // This should be set to the # of rules that need to be executed on a given file
    m_computeGroupSize(16) // This should be set to the ideal number of threads that can run on a compute unit at once (hopefully divisible by work items)
{
    m_errorState = populateDeviceID();

    if (m_errorState == ErrorState::None)
    {
        m_errorState = createContext();
    }

    if(m_errorState == ErrorState::None)
    {
        m_errorState = buildOpenClProgram();
    }

    if(m_errorState == ErrorState::None)
    {
        m_errorState = createCommandQueue();
    }

    if(m_errorState == ErrorState::None)
    {
        m_errorState = createKernel();
    }
}

OpenClProgram::~OpenClProgram()
{
    clReleaseKernel(m_kernel);
    clReleaseCommandQueue(m_commandQueue);
    clReleaseProgram(m_program);
    clReleaseContext(m_context);
}

bool OpenClProgram::SetKernelArg(cl_uint argumentIndex, size_t argumentSize, cl_mem &argumentValue)
{
    int errorCode = 0;
    errorCode  = clSetKernelArg(m_kernel, argumentIndex, argumentSize, &argumentValue);
    if (errorCode < 0)
    {
        qDebug() << Q_FUNC_INFO << " Failed to set Kernel arguments with error: " << errorName(errorCode);
        return false;
    }
    return true;
}

bool OpenClProgram::CreateBuffer(cl_mem &buffer, cl_mem_flags flags, size_t size, void *hostPointer)
{
    int errorCode = 0;
    buffer = clCreateBuffer(m_context, flags, size, hostPointer, &errorCode);
    if(errorCode < 0)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file buffer with error code: " << errorName(errorCode);
        return false;
    }
    return true;
}

bool OpenClProgram::WriteBuffer(cl_mem &buffer, size_t offset, size_t sizeInBytes, const void *data, cl_event *eventCallback)
{
    int errorCode = 0;
    // Perform a non-blocking write operation that will trigger eventCallback when it's complete
    errorCode = clEnqueueWriteBuffer(m_commandQueue, buffer, CL_TRUE, offset, sizeInBytes, data, NULL, nullptr, eventCallback);
    if (errorCode < CL_SUCCESS)
    {
        qDebug() << Q_FUNC_INFO << " Failed to write cl_mem file buffer with error code: " << errorName(errorCode);
        return false;
    }
    return true;
}

Q_INVOKABLE bool OpenClProgram::ReadBuffer(cl_mem &buffer, size_t offset, size_t sizeInBytes, void *data, cl_event *eventCallback)
{
    Q_UNUSED(eventCallback);
    Q_UNUSED(offset);

    cl_int errorCode = clEnqueueReadBuffer(m_commandQueue, buffer, CL_TRUE, 0, sizeInBytes, data, 0, NULL, NULL);
    if (errorCode < 0) {
        qDebug() << Q_FUNC_INFO << " Failed to read output buffer with error code: " << errorName(errorCode);
        return false;
    }

    return true;
}

void OpenClProgram::ReleaseBuffer(cl_mem &buffer)
{
    clReleaseMemObject(buffer);
}

int OpenClProgram::ExecuteKernel(const size_t workItemCount, const size_t computeGroupSize, cl_event *eventCallback)
{
    int errorCode = 0;
    errorCode = clEnqueueNDRangeKernel(m_commandQueue, m_kernel, 1, NULL, &workItemCount, &computeGroupSize, 0, NULL, eventCallback);
    if (errorCode < 0) {
        qDebug() << Q_FUNC_INFO << " Failed to enqueue kernel with error code: " << errorName(errorCode);
        return false;
    }

    return errorCode;
}

QString OpenClProgram::programFileName() const
{
    return m_programFileName;
}

OpenClProgram::DeviceType::enum_type OpenClProgram::deviceType() const
{
    return m_deviceType;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::errorState() const
{
    return m_errorState;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::populateDeviceID()
{
    cl_platform_id platform;
    int errorCode;

    // Attempt to dientify a platform for OpenCL
    errorCode = clGetPlatformIDs(1, &platform, NULL);
    if(errorCode < 0)
    {
        qCritical() << Q_FUNC_INFO << "Couldn't identify a platform!";
        return ErrorState::PlatformError;
    }

    // Then get a device based on that platform. If GPU is selected we'll attempt to
    // obtain a GPU device first, if we're unable to then we'll fall back to CPU.
    if(m_deviceType == DeviceType::GPU)
    {
        qDebug() << Q_FUNC_INFO << " Attempting to obtain GPU device id";
        errorCode = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &m_deviceID, NULL);
        if(errorCode < 0)
        {
            qDebug() << Q_FUNC_INFO << "GPU Device ID Unavailable, attempting to obtain CPU device id";
            errorCode = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &m_deviceID, NULL);
        }

        // TODO: Get some essential stats we need to know about the device and use this in our calculations
        clGetDeviceInfo(m_deviceID, CL_DEVICE_ADDRESS_BITS, sizeof(m_addressBits), &m_addressBits, NULL);
        clGetDeviceInfo(m_deviceID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(m_computeUnits), &m_computeUnits, NULL);
        clGetDeviceInfo(m_deviceID, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(m_maxComputeGroupSize), &m_maxComputeGroupSize, NULL);
    }
    else
    {
        qDebug() << Q_FUNC_INFO << " Attempting to obtain CPU device id";
        errorCode = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &m_deviceID, NULL);
    }
    if(errorCode < 0)
    {
        qCritical() << Q_FUNC_INFO << "Unable to access any devices via OpenCL!";
        return ErrorState::DeviceError;
    }

    return ErrorState::None;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::createContext()
{
    int errorCode = 0;
    m_context = clCreateContext(NULL, 1, &m_deviceID, NULL, NULL, &errorCode);
    if (errorCode < 0)
    {
        qCritical() << Q_FUNC_INFO << "Unable to access any devices via OpenCL! " << errorName(errorCode);
        return ErrorState::ContextError;
    }

    return ErrorState::None;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::buildOpenClProgram()
{
    int errorCode = 0;

    // Create our program from the file passed to this class
    QFile file(m_programFileName);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "Unable to open file" << m_programFileName;
        return ErrorState::ProgramError;
    }

    qint64 size = file.size();
    uchar *data;
    if (size > 0 && size <= 0x7fffffff && (data = file.map(0, size)) != 0) {
        QByteArray array = QByteArray::fromRawData(reinterpret_cast<char *>(data), int(size));
        cl_int error = CL_INVALID_CONTEXT;
        const char *code = array.constData();
        size_t length = array.size();
        m_program = clCreateProgramWithSource(m_context, 1, &code, &length, &error);
        if (errorCode != CL_SUCCESS)
        {
            qCritical() << Q_FUNC_INFO << "Failed to create program! " << errorName(error);
            return ErrorState::ProgramError;
        }
        file.unmap(data);
    }

    // Now build our program, and if we error out we'll print our compilation error from the build log
    errorCode = clBuildProgram(m_program, 0, NULL, NULL, NULL, NULL);
    if(errorCode < 0)
    {
        QByteArray programLog;
        size_t buildLogSize;
        clGetProgramBuildInfo(m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
        programLog.resize(buildLogSize + 1);
        char *data = programLog.data();
        clGetProgramBuildInfo(m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, buildLogSize + 1, data, NULL);
        qCritical() << Q_FUNC_INFO << "Failed to create program!" << QString::fromStdString(programLog.toStdString());
        return ErrorState::CompilationError;
    }
    
    return ErrorState::None;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::createCommandQueue()
{
    int errorCode = 0;
    m_commandQueue = clCreateCommandQueue(m_context, m_deviceID, 0, &errorCode);
    if (errorCode< 0) {
        qCritical() << Q_FUNC_INFO << "Failed to create command queue!";
        return ErrorState::CommandQueueError;
    };
    
    return ErrorState::None;
}

OpenClProgram::ErrorState::enum_type OpenClProgram::createKernel()
{
    int errorCode = 0;
    m_kernel = clCreateKernel(m_program, m_kernelFunctionName.toStdString().c_str(), &errorCode);
    if (errorCode < 0) {
        qCritical() << Q_FUNC_INFO << "Failed to create kernel! " << errorName(errorCode);
        return ErrorState::KernelError;
    };
    
    return ErrorState::None;
}

QString OpenClProgram::errorName(cl_int code)
{
    switch (code) {
#ifdef CL_SUCCESS
    case CL_SUCCESS: return QLatin1String("CL_SUCCESS");
#endif
#ifdef CL_DEVICE_NOT_FOUND
    case CL_DEVICE_NOT_FOUND: return QLatin1String("CL_DEVICE_NOT_FOUND");
#endif
#ifdef CL_DEVICE_NOT_AVAILABLE
    case CL_DEVICE_NOT_AVAILABLE: return QLatin1String("CL_DEVICE_NOT_AVAILABLE");
#endif
#ifdef CL_COMPILER_NOT_AVAILABLE
    case CL_COMPILER_NOT_AVAILABLE: return QLatin1String("CL_COMPILER_NOT_AVAILABLE");
#endif
#ifdef CL_MEM_OBJECT_ALLOCATION_FAILURE
    case CL_MEM_OBJECT_ALLOCATION_FAILURE: return QLatin1String("CL_MEM_OBJECT_ALLOCATION_FAILURE");
#endif
#ifdef CL_OUT_OF_RESOURCES
    case CL_OUT_OF_RESOURCES: return QLatin1String("CL_OUT_OF_RESOURCES");
#endif
#ifdef CL_OUT_OF_HOST_MEMORY
    case CL_OUT_OF_HOST_MEMORY: return QLatin1String("CL_OUT_OF_HOST_MEMORY");
#endif
#ifdef CL_PROFILING_INFO_NOT_AVAILABLE
    case CL_PROFILING_INFO_NOT_AVAILABLE: return QLatin1String("CL_PROFILING_INFO_NOT_AVAILABLE");
#endif
#ifdef CL_MEM_COPY_OVERLAP
    case CL_MEM_COPY_OVERLAP: return QLatin1String("CL_MEM_COPY_OVERLAP");
#endif
#ifdef CL_IMAGE_FORMAT_MISMATCH
    case CL_IMAGE_FORMAT_MISMATCH: return QLatin1String("CL_IMAGE_FORMAT_MISMATCH");
#endif
#ifdef CL_IMAGE_FORMAT_NOT_SUPPORTED
    case CL_IMAGE_FORMAT_NOT_SUPPORTED: return QLatin1String("CL_IMAGE_FORMAT_NOT_SUPPORTED");
#endif
#ifdef CL_BUILD_PROGRAM_FAILURE
    case CL_BUILD_PROGRAM_FAILURE: return QLatin1String("CL_BUILD_PROGRAM_FAILURE");
#endif
#ifdef CL_MAP_FAILURE
    case CL_MAP_FAILURE: return QLatin1String("CL_MAP_FAILURE");
#endif
#ifdef CL_INVALID_VALUE
    case CL_INVALID_VALUE: return QLatin1String("CL_INVALID_VALUE");
#endif
#ifdef CL_INVALID_DEVICE_TYPE
    case CL_INVALID_DEVICE_TYPE: return QLatin1String("CL_INVALID_DEVICE_TYPE");
#endif
#ifdef CL_INVALID_PLATFORM
    case CL_INVALID_PLATFORM: return QLatin1String("CL_INVALID_PLATFORM");
#endif
#ifdef CL_INVALID_DEVICE
    case CL_INVALID_DEVICE: return QLatin1String("CL_INVALID_DEVICE");
#endif
#ifdef CL_INVALID_CONTEXT
    case CL_INVALID_CONTEXT: return QLatin1String("CL_INVALID_CONTEXT");
#endif
#ifdef CL_INVALID_QUEUE_PROPERTIES
    case CL_INVALID_QUEUE_PROPERTIES: return QLatin1String("CL_INVALID_QUEUE_PROPERTIES");
#endif
#ifdef CL_INVALID_COMMAND_QUEUE
    case CL_INVALID_COMMAND_QUEUE: return QLatin1String("CL_INVALID_COMMAND_QUEUE");
#endif
#ifdef CL_INVALID_HOST_PTR
    case CL_INVALID_HOST_PTR: return QLatin1String("CL_INVALID_HOST_PTR");
#endif
#ifdef CL_INVALID_MEM_OBJECT
    case CL_INVALID_MEM_OBJECT: return QLatin1String("CL_INVALID_MEM_OBJECT");
#endif
#ifdef CL_INVALID_IMAGE_FORMAT_DESCRIPTOR
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return QLatin1String("CL_INVALID_IMAGE_FORMAT_DESCRIPTOR");
#endif
#ifdef CL_INVALID_IMAGE_SIZE
    case CL_INVALID_IMAGE_SIZE: return QLatin1String("CL_INVALID_IMAGE_SIZE");
#endif
#ifdef CL_INVALID_SAMPLER
    case CL_INVALID_SAMPLER: return QLatin1String("CL_INVALID_SAMPLER");
#endif
#ifdef CL_INVALID_BINARY
    case CL_INVALID_BINARY: return QLatin1String("CL_INVALID_BINARY");
#endif
#ifdef CL_INVALID_BUILD_OPTIONS
    case CL_INVALID_BUILD_OPTIONS: return QLatin1String("CL_INVALID_BUILD_OPTIONS");
#endif
#ifdef CL_INVALID_PROGRAM
    case CL_INVALID_PROGRAM: return QLatin1String("CL_INVALID_PROGRAM");
#endif
#ifdef CL_INVALID_PROGRAM_EXECUTABLE
    case CL_INVALID_PROGRAM_EXECUTABLE: return QLatin1String("CL_INVALID_PROGRAM_EXECUTABLE");
#endif
#ifdef CL_INVALID_KERNEL_NAME
    case CL_INVALID_KERNEL_NAME: return QLatin1String("CL_INVALID_KERNEL_NAME");
#endif
#ifdef CL_INVALID_KERNEL_DEFINITION
    case CL_INVALID_KERNEL_DEFINITION: return QLatin1String("CL_INVALID_KERNEL_DEFINITION");
#endif
#ifdef CL_INVALID_KERNEL
    case CL_INVALID_KERNEL: return QLatin1String("CL_INVALID_KERNEL");
#endif
#ifdef CL_INVALID_ARG_INDEX
    case CL_INVALID_ARG_INDEX: return QLatin1String("CL_INVALID_ARG_INDEX");
#endif
#ifdef CL_INVALID_ARG_VALUE
    case CL_INVALID_ARG_VALUE: return QLatin1String("CL_INVALID_ARG_VALUE");
#endif
#ifdef CL_INVALID_ARG_SIZE
    case CL_INVALID_ARG_SIZE: return QLatin1String("CL_INVALID_ARG_SIZE");
#endif
#ifdef CL_INVALID_KERNEL_ARGS
    case CL_INVALID_KERNEL_ARGS: return QLatin1String("CL_INVALID_KERNEL_ARGS");
#endif
#ifdef CL_INVALID_WORK_DIMENSION
    case CL_INVALID_WORK_DIMENSION: return QLatin1String("CL_INVALID_WORK_DIMENSION");
#endif
#ifdef CL_INVALID_WORK_GROUP_SIZE
    case CL_INVALID_WORK_GROUP_SIZE: return QLatin1String("CL_INVALID_WORK_GROUP_SIZE");
#endif
#ifdef CL_INVALID_WORK_ITEM_SIZE
    case CL_INVALID_WORK_ITEM_SIZE: return QLatin1String("CL_INVALID_WORK_ITEM_SIZE");
#endif
#ifdef CL_INVALID_GLOBAL_OFFSET
    case CL_INVALID_GLOBAL_OFFSET: return QLatin1String("CL_INVALID_GLOBAL_OFFSET");
#endif
#ifdef CL_INVALID_EVENT_WAIT_LIST
    case CL_INVALID_EVENT_WAIT_LIST: return QLatin1String("CL_INVALID_EVENT_WAIT_LIST");
#endif
#ifdef CL_INVALID_EVENT
    case CL_INVALID_EVENT: return QLatin1String("CL_INVALID_EVENT");
#endif
#ifdef CL_INVALID_OPERATION
    case CL_INVALID_OPERATION: return QLatin1String("CL_INVALID_OPERATION");
#endif
#ifdef CL_INVALID_GL_OBJECT
    case CL_INVALID_GL_OBJECT: return QLatin1String("CL_INVALID_GL_OBJECT");
#endif
#ifdef CL_INVALID_BUFFER_SIZE
    case CL_INVALID_BUFFER_SIZE: return QLatin1String("CL_INVALID_BUFFER_SIZE");
#endif
#ifdef CL_INVALID_MIP_LEVEL
    case CL_INVALID_MIP_LEVEL: return QLatin1String("CL_INVALID_MIP_LEVEL");
#endif
#ifdef CL_INVALID_GLOBAL_WORK_SIZE
    case CL_INVALID_GLOBAL_WORK_SIZE: return QLatin1String("CL_INVALID_GLOBAL_WORK_SIZE");
#endif

        // OpenCL 1.1
    case CL_MISALIGNED_SUB_BUFFER_OFFSET: return QLatin1String("CL_MISALIGNED_SUB_BUFFER_OFFSET");
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return QLatin1String("CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST");

    default: break;
    }
    return QLatin1String("Error ") + QString::number(code);
}
