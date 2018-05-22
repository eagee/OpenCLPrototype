#include"GPUScanWorker.h"
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "OpenClProgram.h"

OpenClProgram GPUScanWorker::m_openClProgram("createChecksum.cl", "createChecksum", OpenClProgram::DeviceType::GPU);

GPUScanWorker::GPUScanWorker(QObject *parent /*= nullptr*/)
    : IScanWorker(parent)
{
    m_state = ScanWorkerState::Available;
    m_bytesPerFile = 0;
    m_buffersCreated = false;
}

bool GPUScanWorker::loadFileData(const QString &filePath)
{
    Q_ASSERT_X(areBuffersFull() == false, Q_FUNC_INFO, "This function should never be called when buffers are full");

    if (!QFile::exists(filePath))
    {
        qDebug() << Q_FUNC_INFO << " File: " << filePath << " not found.";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << Q_FUNC_INFO << "Unable to open file" << filePath;
        return false;
    }

    QByteArray clFileBufferData = file.read(m_bytesPerFile);

    int lenght = clFileBufferData.size();
    void *data = (void*)clFileBufferData.constData();

    m_state = ScanWorkerState::Copying;

    size_t dataOffset = (m_filesToScan.size() * m_bytesPerFile);
    if (!m_openClProgram.WriteBuffer(m_fileDataBuffer, dataOffset, m_bytesPerFile, data, nullptr)) //&m_gpuFinishedEvent
    {
        qDebug() << Q_FUNC_INFO << "Unable to write file buffer to GPU: " << filePath;
        return false;
    }
    
    // Set up our event callback so that we're notified when the kernel is done (and can put ourselves back into the worker queue)
    //cl_int errcode = clSetEventCallback(m_gpuFinishedEvent, CL_COMPLETE, &GPUScanWorker::gpuFinishedCallback, this);
    //if (errcode != CL_SUCCESS)
    //{
    //    m_state = ScanWorkerState::Error;
    //    qDebug() << Q_FUNC_INFO << " Failed to set up event with error: " << OpenClProgram::errorName(errcode);
    //}

    m_filesToScan.append(filePath);

    if (areBuffersFull() == true)
    {
        setState(ScanWorkerState::Ready);
    }
    else
    {
        setState(ScanWorkerState::Available);
    }

    return true;
}

GPUScanWorker::~GPUScanWorker()
{
    if(m_buffersCreated == true)
    {
        m_openClProgram.ReleaseBuffer(m_fileDataBuffer);
        m_openClProgram.ReleaseBuffer(m_dataSizeBuffer);
        m_openClProgram.ReleaseBuffer(m_outputBuffer);
    }
}

bool GPUScanWorker::queueLoadOperation(QString filePath) 
{
    if(areBuffersFull() == true || m_state == ScanWorkerState::Ready)
    {
        qDebug() << "Unable to queue operation for file: " << filePath << " because queue is already full";
        return false;
    }
    m_nextOperation.Type = OperationType::Load;
    m_nextOperation.FilePath = filePath;
    return true;
}

bool GPUScanWorker::queueScanOperation()
{
    if(m_filesToScan.size() <= 0 || (m_state != ScanWorkerState::Available && m_state != ScanWorkerState::Ready))
    {
        qDebug() << Q_FUNC_INFO << "Scan operation called with no files to scan or worker in a bad state :(.";
        return false;
    }

    m_nextOperation.Type = OperationType::ExecuteScan;
    m_nextOperation.FilePath = "";
    return true;
}

void GPUScanWorker::run()
{
    if(m_nextOperation.Type == OperationType::Load)
    {
        loadFileData(m_nextOperation.FilePath);
    }
    else if (m_nextOperation.Type == OperationType::ExecuteScan)
    {
        executeKernelOperation();
    }
}

ScanWorkerState::enum_type GPUScanWorker::state() const
{
    return m_state;
}

void GPUScanWorker::setState(ScanWorkerState::enum_type state)
{
    m_state = state;
    emit stateChanged();
}

bool GPUScanWorker::createFileBuffers(size_t bytesPerFile, int numberOfFiles)
{
    m_maxFiles = numberOfFiles;
    m_bytesPerFile = bytesPerFile;

    bool success = false;

    success = m_openClProgram.CreateBuffer(m_fileDataBuffer, CL_MEM_READ_ONLY, (m_bytesPerFile * m_maxFiles), nullptr);
    if(!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file buffer";
        return false;
    }
    
    success = m_openClProgram.CreateBuffer(m_dataSizeBuffer, CL_MEM_READ_ONLY, sizeof(size_t), nullptr);
    if(!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file size";
        return false;
    }

    // Write bytes per file to the data size buffer
    success = m_openClProgram.WriteBuffer(m_dataSizeBuffer, 0, sizeof(size_t), &m_bytesPerFile, nullptr);
    if (!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Unable to write file size length";
        return false;
    }

    success = m_openClProgram.CreateBuffer(m_outputBuffer, CL_MEM_WRITE_ONLY, (sizeof(int) * m_maxFiles), nullptr);
    if(!success)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem output buffer";
        return false;
    }

    m_hostResultData.reset(new int[m_maxFiles]);

    m_buffersCreated = true;

    return true;
}

bool GPUScanWorker::areBuffersFull()
{
    return (m_filesToScan.count() >= m_maxFiles);
}

void GPUScanWorker::resetFileBuffers()
{
    m_filesToScan.clear();
    m_state = ScanWorkerState::Available;
}

int GPUScanWorker::queuedFileCount() const
{
    return m_filesToScan.size();
}

void GPUScanWorker::processResults()
{
    // Once we get our callback we read out the results of the output buffer...
    //success = m_openClProgram.CreateBuffer(m_outputBuffer, CL_MEM_WRITE_ONLY, (sizeof(int) * m_maxFiles), nullptr);
    bool success = m_openClProgram.ReadBuffer(m_outputBuffer, 0, (sizeof(int) * m_maxFiles), m_hostResultData.get(), nullptr);
    if (success)
    {
        for (int index = 0; index < m_filesToScan.count(); index++)
        {
            if (m_hostResultData[index] == 14330)
            {
                emit infectionFound(m_filesToScan.at(index));
            }
            //qDebug() << "Checksum for " << m_filesToScan.at(index) << ": " << m_hostResultData[index];
        }
    }
}

void GPUScanWorker::executeKernelOperation()
{
    if(!m_buffersCreated)
    {
        qDebug() << Q_FUNC_INFO << " This should never happen, quit it.";
        return;
    }

    // We have to use a mutex so we don't accidentally queue kenel execution or set arguments at the same time
    // as other threads.
    QMutex mutex;
    QMutexLocker locker(&mutex);

    // Set our kernel arguments
    if(!m_openClProgram.SetKernelArg(0, sizeof(cl_mem), m_fileDataBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set buffer arg for file.";
        return;
    }

    if(!m_openClProgram.SetKernelArg(1, sizeof(size_t), m_dataSizeBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set buffer size arg for file.";
        return;
    }

    if(!m_openClProgram.SetKernelArg(2, sizeof(cl_mem), m_outputBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set output buffer arg for file.";
        return;
    }

    // TODO: Set our work item and group sizes appropriately for our 5 compute node GPU!
    const size_t workItemSize = m_maxFiles;
    const size_t workGroupSize = 1024;

    // Now if we kick off our kernel operation it'll process all of the files we've enqueued and send us an event when it's
    // done that we can use to read out the contents of the output buffer when it's done.
    int result = m_openClProgram.ExecuteKernel(workItemSize, workGroupSize, &m_gpuFinishedEvent);
    if( result != CL_SUCCESS)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to set up event with error: " << OpenClProgram::errorName(result);
    }

    // Set up our event callback so that we're notified when the kernel is done (and can put ourselves back into the worker queue)
    cl_int errcode = clSetEventCallback(m_gpuFinishedEvent, CL_COMPLETE, &GPUScanWorker::gpuFinishedCallback, this);
    if (errcode != CL_SUCCESS)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to set up event with error: " << OpenClProgram::errorName(errcode);
    }

    //setState(ScanWorkerState::Scanning);
    m_state = ScanWorkerState::Scanning;
}

void GPUScanWorker::gpuFinishedCallback(cl_event event, cl_int cmd_exec_status, void *user_data)
{
    Q_UNUSED(event);
    qDebug() << Q_FUNC_INFO << " Event received command execute status: " << OpenClProgram::errorName(cmd_exec_status);
    GPUScanWorker *worker = static_cast<GPUScanWorker*>(user_data);

    qDebug() << Q_FUNC_INFO << " Worker state: " << worker->state();
    
    if (worker->state() == ScanWorkerState::Scanning)
    {    
        worker->processResults();
        worker->setState(ScanWorkerState::Complete);
    }
    else if (worker->state() == ScanWorkerState::Copying)
    {
        if (worker->areBuffersFull() == true)
        {
            worker->setState(ScanWorkerState::Ready);
        }
        else
        {
            worker->setState(ScanWorkerState::Available);
        }
    }
    else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "State unsupported");
    }
}

