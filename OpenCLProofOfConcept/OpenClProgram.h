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

    bool SetKernelArg(cl_uint argumentIndex, size_t argumentSize, cl_mem &argumentValue);

    bool CreateBuffer(cl_mem &buffer, cl_mem_flags flags, size_t size, void *hostPointer);

    void ReleaseBuffer(cl_mem buffer);

    int ExecuteKernel(const size_t workItemCount, const size_t computeGroupSize, cl_mem &outputBuffer);

    QString programFileName() const;
    
    DeviceType::enum_type deviceType() const;
    ErrorState::enum_type errorState() const;


private:
    QString m_programFileName;
    QString m_kernelFunctionName;
    DeviceType::enum_type m_deviceType;
    ErrorState::enum_type m_errorState;
    cl_device_id m_deviceID;
    cl_context m_context;
    cl_program m_program;
    cl_command_queue m_commandQueue;
    cl_kernel m_kernel;
    int m_workItemsToExecute;
    int m_computeGroupSize;

    // Populate our DeviceID via OpenCL API
    ErrorState::enum_type populateDeviceID();

    ErrorState::enum_type createContext();

    ErrorState::enum_type buildOpenClProgram();

    ErrorState::enum_type configureKernel();

    ErrorState::enum_type createCommandQueue();

    ErrorState::enum_type createKernel();

    QString errorName(cl_int code);

};

