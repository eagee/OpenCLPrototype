#include <QObject>
#include <CL/cl.h>
#pragma once

class OpenClProgram: public QObject
{
    Q_OBJECT

public:

    struct DeviceType
    {
        enum enum_type
        {
            CPU,
            GPU
        };
    };

    struct ErrorState
    {
        enum enum_type
        {
            None,
            ContextError,
            PlatformError,
            DeviceError,
            ProgramError,
            CompilationError,
            CommandQueueError,
            KernelError,
            ScanFileError,
            BufferError
        };
    };

    OpenClProgram(QString programFileName, QString functionName, DeviceType::enum_type deviceType, QObject *parent = nullptr);

    ~OpenClProgram();

    Q_INVOKABLE bool createCommandQueue(cl_command_queue &commandQueue);

    Q_INVOKABLE void freeCommandQueue(cl_command_queue &commandQueue);

    Q_INVOKABLE bool SetKernelArg(cl_uint argumentIndex, size_t argumentSize, cl_mem &argumentValue);

    Q_INVOKABLE bool CreateBuffer(cl_mem &buffer, cl_mem_flags flags, size_t size, void *hostPointer);

    Q_INVOKABLE bool WriteBuffer(cl_command_queue &commandQueue, cl_mem &buffer, size_t offset, size_t sizeInBytes, const void *data, cl_event *eventCallback, bool blockThread = true);

    Q_INVOKABLE bool ReadBuffer(cl_command_queue &commandQueue, cl_mem &buffer, size_t offset, size_t sizeInBytes, void *data, cl_event *eventCallback, bool blockThread /*= true*/);

    Q_INVOKABLE void ReleaseBuffer(cl_mem &buffer);

    //CL_MAP_WRITE | CL_MAP_READ
    Q_INVOKABLE void* MapBuffer(cl_command_queue &commandQueue, cl_mem &buffer, cl_map_flags flags, size_t sizeInBytes);

    Q_INVOKABLE void UnMapBuffer(cl_command_queue &commandQueue, cl_mem &buffer, void *mappedPointer);

    Q_INVOKABLE int ExecuteKernel(cl_command_queue &commandQueue, const size_t workItemCount, const size_t computeGroupSize, cl_event *eventCallback);

    Q_INVOKABLE QString programFileName() const;
    
    Q_INVOKABLE DeviceType::enum_type deviceType() const;

    Q_INVOKABLE ErrorState::enum_type errorState() const;

    static QString errorName(cl_int code);

private:
    QString m_programFileName;
    QString m_kernelFunctionName;
    DeviceType::enum_type m_deviceType;
    ErrorState::enum_type m_errorState;
    cl_device_id m_deviceID;
    cl_context m_context;
    cl_program m_program;
    //cl_command_queue m_commandQueue;
    cl_kernel m_kernel;
    int m_workItemsToExecute;
    int m_computeGroupSize;
    cl_uint m_addressBits;
    cl_uint m_computeUnits;
    size_t m_maxComputeGroupSize;

    // Populate our DeviceID via OpenCL API
    ErrorState::enum_type populateDeviceID();

    ErrorState::enum_type createContext();

    ErrorState::enum_type buildOpenClProgram();

    ErrorState::enum_type configureKernel();

    ErrorState::enum_type createKernel();

};

