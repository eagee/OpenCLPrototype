#include"CPUScanWorker.h"
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "OpenClProgram.h"

CPUScanWorker::CPUScanWorker(QObject *parent /*= nullptr*/)
    : IScanWorker(parent)
{
    m_state = ScanWorkerState::Available;
    m_bytesPerFile = 0;
    m_pendingWorkItems = 0;
    m_buffersCreated = false;
}

bool CPUScanWorker::loadFileData(const QString &filePath)
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

    unsigned int filesToScan = (m_filesToScan.size());
    unsigned int dataOffset = (filesToScan * m_bytesPerFile);
    m_fileDataBuffer = m_fileDataBuffer.replace(dataOffset, m_bytesPerFile, clFileBufferData);

    m_filesToScan.append(filePath);

    if (areBuffersFull() == true)
    {
        QTimer::singleShot(0, this, &CPUScanWorker::OnSetStateReady);
    }
    else
    {
        QTimer::singleShot(0, this, &CPUScanWorker::OnSetStateAvailabile);
    }

    return true;
}

CPUScanWorker::~CPUScanWorker()
{
    if(m_buffersCreated == true)
    {
        m_fileDataBuffer.clear();
        m_hostResultData.release();
    }
}

bool CPUScanWorker::queueLoadOperation(QString filePath)
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

bool CPUScanWorker::queueScanOperation()
{
    if(m_filesToScan.size() <= 0 || (m_state != ScanWorkerState::Available && m_state != ScanWorkerState::Ready))
    {
        //qDebug() << Q_FUNC_INFO << "Scan operation called with no files to scan or worker in a bad state :(.";
        return false;
    }

    m_nextOperation.Type = OperationType::ExecuteScan;
    m_nextOperation.FilePath = "";
    return true;
}

bool CPUScanWorker::queueReadOperation()
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

void CPUScanWorker::run()
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
        QTimer::singleShot(0, this, &CPUScanWorker::OnSetStateAvailabile);
    }
}

ScanWorkerState::enum_type CPUScanWorker::state() const
{
    return m_state;
}

void CPUScanWorker::setState(ScanWorkerState::enum_type state)
{
    m_state = state;
    emit stateChanged(static_cast<void*>(this));
}

bool CPUScanWorker::createFileBuffers(size_t bytesPerFile, int numberOfFiles)
{
    m_maxFiles = numberOfFiles;
    m_bytesPerFile = bytesPerFile;

    m_fileDataBuffer.resize((m_bytesPerFile * m_maxFiles));
    m_dataSizeBuffer = m_bytesPerFile;
    m_hostResultData.reset(new int[m_maxFiles]);

    m_buffersCreated = true;

    return true;
}

bool CPUScanWorker::areBuffersFull()
{
    return (m_filesToScan.count() >= m_maxFiles);
}

void CPUScanWorker::resetFileBuffers()
{
    m_filesToScan.clear();
    m_state = ScanWorkerState::Available;
}

int CPUScanWorker::queuedFileCount() const
{
    return m_filesToScan.size();
}

void CPUScanWorker::processResults()
{
    // Iterate through the host result data and see if we've got a matching checksum
    for (int index = 0; index < m_filesToScan.count(); index++)
    {
        if (m_hostResultData[index] == 14330 || m_hostResultData[index] == 5974)
        {
            emit infectionFound(m_filesToScan.at(index));
        }
        //qDebug() << "Checksum for " << m_filesToScan.at(index) << ": " << m_hostResultData[index];
    }
}

void CPUScanWorker::executeKernelOperation()
{
    if(!m_buffersCreated)
    {
        qDebug() << Q_FUNC_INFO << " This should never happen, quit it.";
        return;
    }

    m_state = ScanWorkerState::Scanning;
    m_pendingWorkItems = 0;
    for (int index = 0; index < m_filesToScan.count(); index++)
    {
        // Kick each file we want to process in our image off in a thread pool
        CPUKernelRunnable *work = new CPUKernelRunnable(m_fileDataBuffer, index, m_bytesPerFile);
        QObject::connect(work, &CPUKernelRunnable::ProcessingComplete, this, &CPUScanWorker::OnProcessingComplete);
        QThreadPool::globalInstance()->start(work);
    }
}

void CPUScanWorker::OnProcessingComplete(int fileIndex, int checksum)
{
    m_hostResultData[fileIndex] = checksum;
    m_pendingWorkItems++;
    if (m_pendingWorkItems >= m_filesToScan.count())
    {
        QTimer::singleShot(0, this, &CPUScanWorker::OnSetStateComplete);
    }
}

CPUKernelRunnable::CPUKernelRunnable(QByteArray &fileDataBuffer, int fileIndex, unsigned int bytesPerFile) :
    m_fileDataBuffer(fileDataBuffer),
    m_fileIndex(fileIndex),
    m_bytesPerFile(bytesPerFile)
{

}

void CPUKernelRunnable::run()
{
    int outputChecksum = 0;
    unsigned int fileOffset = m_fileIndex * m_bytesPerFile;
    for (unsigned int checksumOffset = 0; checksumOffset < m_bytesPerFile; checksumOffset++)
    {
        outputChecksum += m_fileDataBuffer[fileOffset + checksumOffset];
    }
    emit ProcessingComplete(m_fileIndex, outputChecksum);
}
