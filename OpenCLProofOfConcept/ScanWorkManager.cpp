#include <QObject>
#include <QQueue>
#include "ScanWorkManager.h"
#include "TestScanner.h"
#include "OpenClProgram.h"
#include "CPUBoundScanOperations.h"
#include "GPUScanWorker.h"
#include "CPUScanWorker.h"

const int BYTES_PER_FILE = 8192;
const int PROGRAM_POOL_SIZE = 64;
const int MAX_FILES_PER_PROGRAM = 256;

ScanWorkManager::ScanWorkManager(QObject *parent, bool useGPU, bool useBoth): QObject(parent), 
m_scanWorkersFinished(0), m_useGPU(useGPU), m_useBoth(useBoth)
{
}

int ScanWorkManager::totalFilesToScan()
{
    return m_filesToScan->count();
}

const QFileInfo* ScanWorkManager::GetNextFile()
{
    const QFileInfo *nextFile = nullptr;
    
    if (*m_fileIndex <= m_filesToScan->count() - 1)
    {
        nextFile = &m_filesToScan->at(*m_fileIndex);

        // Atomic increment to the next index in the file list
        while (!CanProcessFile(nextFile->filePath()))
        {
            if ((*m_fileIndex + 1) > m_filesToScan->count() - 1)
            {
                break;
            }

            m_fileIndex->fetchAndAddAcquire(1);
            nextFile = &m_filesToScan->at(*m_fileIndex);
        }
        emit fileProcessingComplete(nextFile->filePath(), m_filesToScan->count());
        m_fileIndex->fetchAndAddAcquire(1);
    }
    return nextFile;
}

void ScanWorkManager::InitWorkers()
{
    m_fileIndex.reset(new QAtomicInt(0));

    const int MAX_GPU_IN_BOTH = 2;
    int gpuWorkersCreated = 0;

    QDir testDir("C:\\TestFiles");
    m_filesToScan.reset(new QFileInfoList(testDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst)));
    //qDebug() << Q_FUNC_INFO << " Preparing to scan this many files: " << m_filesToScan->count();

    // Set up our pool of worker objects
    for (int ix = 0; ix < PROGRAM_POOL_SIZE; ix++)
    {
        QString id = QString("%1").arg(ix, 5, 10, QChar('0'));
        
        //if(ix % 2 == 0)
        if (m_useBoth)
        {
            if (gpuWorkersCreated < MAX_GPU_IN_BOTH)
            {
                GPUScanWorker *newGpuProgram = new GPUScanWorker(this, id);
                newGpuProgram->createFileBuffers(BYTES_PER_FILE, MAX_FILES_PER_PROGRAM);
                QObject::connect(newGpuProgram, &GPUScanWorker::stateChanged, this, &ScanWorkManager::OnStateChanged);
                QObject::connect(newGpuProgram, &GPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
                m_programPool.push_back(newGpuProgram);
                gpuWorkersCreated++;
            }
            else
            {
                CPUScanWorker *newCpuProgram = new CPUScanWorker(this, id);
                newCpuProgram->createFileBuffers(BYTES_PER_FILE, MAX_FILES_PER_PROGRAM);
                QObject::connect(newCpuProgram, &CPUScanWorker::stateChanged, this, &ScanWorkManager::OnStateChanged);
                QObject::connect(newCpuProgram, &CPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
                m_programPool.push_back(newCpuProgram);
            }
        }
        else if (m_useGPU)
        {
            GPUScanWorker *newGpuProgram = new GPUScanWorker(this, id);
            newGpuProgram->createFileBuffers(BYTES_PER_FILE, MAX_FILES_PER_PROGRAM);
            QObject::connect(newGpuProgram, &GPUScanWorker::stateChanged, this, &ScanWorkManager::OnStateChanged);
            QObject::connect(newGpuProgram, &GPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
            m_programPool.push_back(newGpuProgram);
        }
        else 
        {
            CPUScanWorker *newCpuProgram = new CPUScanWorker(this, id);
            newCpuProgram->createFileBuffers(BYTES_PER_FILE, MAX_FILES_PER_PROGRAM);
            QObject::connect(newCpuProgram, &CPUScanWorker::stateChanged, this, &ScanWorkManager::OnStateChanged);
            QObject::connect(newCpuProgram, &CPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
            m_programPool.push_back(newCpuProgram);
        }
    }
}

void ScanWorkManager::doWork()
{
    InitWorkers();

    for (int ix = 0; ix < PROGRAM_POOL_SIZE; ix++)
    {
        const QFileInfo* nextFile = GetNextFile();
        m_programPool.at(ix)->queueLoadOperation(nextFile->filePath());
    }
    
    for (int ix = 0; ix < PROGRAM_POOL_SIZE; ix++)
    {
        m_programPool.at(ix)->setAutoDelete(false);
        QThreadPool::globalInstance()->start(m_programPool.at(ix));
    }

    // Our thread won't return finished until we've processed our last file
    // we'll just hang out in this event loop until then (so that we can process all our opencl events)
    QEventLoop eventLoop;
    QObject::connect(this, &ScanWorkManager::workFinished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    m_programPool.clear();
}

void ScanWorkManager::OnStateChanged(void *scanWorkerPtr)
{
    GPUScanWorker *scanWorker = static_cast<GPUScanWorker*>(scanWorkerPtr);
    //qDebug() << Q_FUNC_INFO << QString("%1").arg(QDateTime::currentDateTime().currentMSecsSinceEpoch()) + "ID: " << scanWorker->id() << "State changed for scan worker to: " << ScanWorkerState::ToString(scanWorker->state());
    
    // If the state of the worker is Available and there are more files to scan we'll queue the next file
    if (scanWorker->state() == ScanWorkerState::Available)
    {
        const QFileInfo* nextFile = GetNextFile();
        if(nextFile != nullptr)
        { 
            scanWorker->queueLoadOperation(nextFile->filePath());
            QThreadPool::globalInstance()->start(scanWorker);
        }
        else 
        {
            if(scanWorker->queueScanOperation())
                QThreadPool::globalInstance()->start(scanWorker);
        }
    }
    // If the state of the worker is Ready we'll queue a scan operation to run on the worker
    else if (scanWorker->state() == ScanWorkerState::Ready)
    {
        if (scanWorker->queueScanOperation())
            QThreadPool::globalInstance()->start(scanWorker);
    }
    // If the state of the worker is Complete we'll queue a read results operation
    // Once this is done the worker should signal us again with an available state
    else if (scanWorker->state() == ScanWorkerState::Complete)
    {
        scanWorker->queueReadOperation();
        QThreadPool::globalInstance()->start(scanWorker);
    }
    else if (scanWorker->state() == ScanWorkerState::Done)
    {
        m_scanWorkersFinished++;
        if (m_scanWorkersFinished >= PROGRAM_POOL_SIZE)
        {
            emit workFinished();
        }
    }
    else
    {
        ScanWorkerState::enum_type state = scanWorker->state();
        Q_ASSERT_X(false, Q_FUNC_INFO, QString("This isn't a state we support in work manager. Please examine results: %1").arg(ScanWorkerState::ToString(state)).toStdString().c_str() );
    }
}

void ScanWorkManager::QueueAndRunScanWorker(IScanWorker *scanWorker)
{
    scanWorker->queueScanOperation();
    QThreadPool::globalInstance()->start(scanWorker);
}

void ScanWorkManager::OnInfectionFound(QString filePath)
{
    emit infectionFound(filePath);
}

bool ScanWorkManager::CanProcessFile(QString filePath)
{
    QFile file(filePath);
    
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << Q_FUNC_INFO << "Unable to open file" << filePath;
        return false;
    }

    if (BYTES_PER_FILE > file.size())
    {
        // Report the file we can't process as having been scanned...
        emit fileProcessingComplete(filePath, m_filesToScan->count());
        return false;
    }
    return true;
}