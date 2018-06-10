#include"GPUScanWorker.h"
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "OpenClProgram.h"

#define BYTES_INTS_IN_MD5_HASH 16

OpenClProgram GPUScanWorker::m_openClProgram("createMd5.cl", "createMd5", OpenClProgram::DeviceType::GPU);

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
    m_state = ScanWorkerState::Copying;
    unsigned int dataOffset = (m_filesToScan.size() * m_bytesPerFile);
    memcpy(&m_mappedHostFileData[dataOffset], clFileBufferData.data(), m_bytesPerFile);

    // We've deprecated the WriteBuffer code in lieu of using mapped buffers instead (theoritcally this should allow us a faster copy performance)
    //size_t dataOffset = (m_filesToScan.size() * m_bytesPerFile);
    //if (!m_openClProgram.WriteBuffer(m_gpuFileDataBuffer, dataOffset, m_bytesPerFile, data, &m_gpuFinishedEvent, false)) //&m_gpuFinishedEvent
    //{
    //    qDebug() << Q_FUNC_INFO << "Unable to write file buffer to GPU: " << filePath;
    //    return false;
    //}
    //
    //// Set up our event callback so that we're notified when the kernel is done (and can put ourselves back into the worker queue)
    //cl_int errcode = clSetEventCallback(m_gpuFinishedEvent, CL_COMPLETE, &GPUScanWorker::gpuFinishedCallback, this);
    //if (errcode != CL_SUCCESS)
    //{
    //    m_state = ScanWorkerState::Error;
    //    qDebug() << Q_FUNC_INFO << " Failed to set up event with error: " << OpenClProgram::errorName(errcode);
    //}

    m_filesToScan.append(filePath);

    // Since we no longer interact with the GPU during this operation, we'll trigger our slot changes here.
    if (areBuffersFull() == true)
    {
        QTimer::singleShot(0, this, &GPUScanWorker::OnSetStateReady);
    }
    else
    {
        QTimer::singleShot(0, this, &GPUScanWorker::OnSetStateAvailabile);
    }
    
    return true;
}

GPUScanWorker::~GPUScanWorker()
{
    if(m_buffersCreated == true)
    {
        m_openClProgram.UnMapBuffer(m_gpuFileDataBuffer, m_mappedHostFileData);
        m_openClProgram.ReleaseBuffer(m_gpuFileDataBuffer);
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
        //qDebug() << Q_FUNC_INFO << "Scan operation called with no files to scan.";
        return false;
    }

    m_nextOperation.Type = OperationType::ExecuteScan;
    m_nextOperation.FilePath = "";
    return true;
}

bool GPUScanWorker::queueReadOperation()
{
    //Q_ASSERT_X(m_state == ScanWorkerState::Complete, Q_FUNC_INFO, "read operations should not be queued in state: " + ScanWorkerState::ToString(m_state));
    if (m_state != ScanWorkerState::Complete)
    {
        qDebug() << Q_FUNC_INFO << "Read operations should not be queued in state: " + ScanWorkerState::ToString(m_state);
        return false;
    }
    m_nextOperation.Type = OperationType::ReadResults;
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
    else if (m_nextOperation.Type == OperationType::ReadResults)
    {
        processResults();
        resetFileBuffers();
        setState(ScanWorkerState::Available);
    }
}

ScanWorkerState::enum_type GPUScanWorker::state() const
{
    return m_state;
}

void GPUScanWorker::setState(ScanWorkerState::enum_type state)
{
    m_state = state;
    emit stateChanged(static_cast<void*>(this));
}

bool GPUScanWorker::createFileBuffers(size_t bytesPerFile, int numberOfFiles)
{
    m_maxFiles = numberOfFiles;
    m_bytesPerFile = bytesPerFile;

    bool success = false;

    // Create a buffer that will be mapped between host and gpu for read/write operations (this will allow us to build the file
    // contents until the buffer is full on the host, and then we can unmap before the kernel executs, and remap when buffers are reset.
    success = m_openClProgram.CreateBuffer(m_gpuFileDataBuffer, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, (m_bytesPerFile * m_maxFiles), nullptr);
    if(!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file buffer";
        return false;
    }
    
    // Create a buffer that contains the number of bytes we're processing in our data on the GPU
    success = m_openClProgram.CreateBuffer(m_dataSizeBuffer, CL_MEM_READ_ONLY, sizeof(size_t), nullptr);
    if(!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file size";
        return false;
    }

    // Write to the above buffer confirming how much data we have to process
    success = m_openClProgram.WriteBuffer(m_dataSizeBuffer, 0, sizeof(size_t), &m_bytesPerFile, nullptr);
    if (!success)
    {
        m_state = ScanWorkerState::Error;
        qDebug() << Q_FUNC_INFO << " Unable to write file size length";
        return false;
    }

    // Create the output buffer that will contain our hashes/checksums
    success = m_openClProgram.CreateBuffer(m_outputBuffer, CL_MEM_WRITE_ONLY, (BYTES_INTS_IN_MD5_HASH * m_maxFiles), nullptr);
    if(!success)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem output buffer";
        return false;
    }

    m_hostResultData.reset(new char[m_maxFiles * BYTES_INTS_IN_MD5_HASH]);

    // We need to call this the first time the method is run so that m_mappedHostFileData is mapped, enabling us to
    // access it directly (this will be called after each subsequent kernel operation is executed after that)
    resetFileBuffers();

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
    m_mappedHostFileData = (char*)m_openClProgram.MapBuffer(m_gpuFileDataBuffer, CL_MAP_WRITE, (m_bytesPerFile * m_maxFiles));
}

int GPUScanWorker::queuedFileCount() const
{
    return m_filesToScan.size();
}

void GPUScanWorker::processResults()
{
    // Once we get our callback we read out the results of the output buffer...
    //bool success = m_openClProgram.ReadBuffer(m_outputBuffer, 0, (BYTES_INTS_IN_MD5_HASH * m_maxFiles), m_hostResultData.get(), nullptr);
    //if (success)
    //{
    //    for (int index = 0; index < m_filesToScan.count(); index++)
    //    {
    //        QByteArray hash;
    //        hash.setRawData(&m_hostResultData[index * BYTES_INTS_IN_MD5_HASH], BYTES_INTS_IN_MD5_HASH);
    //        QString hashString = hash.toHex();
    //        
    //        if (hashString  == "95e78b5b9da5d0feb5c4ab5a516608e9" ||
    //            hashString  == "8448a87b66e10f80be542f1650208f12" ||
    //            hashString  == "f42a7d487de3a5bd7deb18c933aa3d5e" ||
    //            hashString  == "8ea6592c4f1e8b766e8e6a5861bf5b19")
    //        {
    //            emit infectionFound(m_filesToScan.at(index));
    //        }
    //        //qDebug() << "Hash for " << m_filesToScan.at(index) << ": " << hashString;
    //    }
    //}
}

void GPUScanWorker::executeKernelOperation()
{
    if(!m_buffersCreated)
    {
        qDebug() << Q_FUNC_INFO << " This should never happen, quit it.";
        return;
    }

    // We have to use a mutex so we don't accidentally queue kernel execution or set arguments at the same time
    // as other threads.
    QMutex mutex;
    QMutexLocker locker(&mutex);

    m_openClProgram.UnMapBuffer(m_gpuFileDataBuffer, m_mappedHostFileData);

    // Set our kernel arguments
    if(!m_openClProgram.SetKernelArg(0, sizeof(cl_mem), m_gpuFileDataBuffer))
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
    const size_t workGroupSize = m_maxFiles / 8;

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
    Q_UNUSED(cmd_exec_status);
    
    //qDebug() << Q_FUNC_INFO << " Event received command execute status: " << OpenClProgram::errorName(cmd_exec_status);
    GPUScanWorker *worker = static_cast<GPUScanWorker*>(user_data);

    //qDebug() << Q_FUNC_INFO << " Worker state: " << ScanWorkerState::ToString(worker->state());
    
    if (worker->state() == ScanWorkerState::Scanning)
    {    
        worker->setState(ScanWorkerState::Complete);
    }
    else if (worker->state() == ScanWorkerState::Copying)
    {
        // TODO: Remove this code, this state will be deprecated now that we're using mapped buffers...
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

