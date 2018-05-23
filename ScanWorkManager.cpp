#include <QObject>
#include <QQueue>
#include "ScanWorkManager.h"
#include "TestScanner.h"
#include "OpenClProgram.h"
#include "CPUBoundScanOperations.h"
#include "GPUScanWorker.h"

const int BYTES_PER_FILE = 1024;
const int GPU_PROGRAM_POOL_SIZE = 8;

ScanWorkManager::ScanWorkManager(QObject *parent, TestScannerListModel *testScanner): QObject(parent), m_parent(testScanner), m_fileIndex(0)
{
    //qRegisterMetaType<GPUScanWorker>("GPUScanWorker");

    QDir testDir("C:\\TestFiles");
    m_filesToScan = testDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
    qDebug() << Q_FUNC_INFO << " Preparing to scan this many files: " << m_filesToScan.count();

    // Set up our pool of worker objects
    for (int ix = 0; ix < GPU_PROGRAM_POOL_SIZE; ix++)
    {
        GPUScanWorker *newProgram = new GPUScanWorker(this);
        newProgram->createFileBuffers(BYTES_PER_FILE, 2048);
        QObject::connect(newProgram, &GPUScanWorker::stateChanged, this, &ScanWorkManager::OnStateChanged);
        QObject::connect(newProgram, &GPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
        m_gpuProgramPool.push_back(newProgram);
    }
}

int ScanWorkManager::totalFilesToScan()
{
    return m_filesToScan.count();
}

const QFileInfo* ScanWorkManager::GetNextFile()
{
    const QFileInfo *nextFile = nullptr;
    
    if (m_fileIndex <= m_filesToScan.count() - 1)
    {
        nextFile = &m_filesToScan.at(m_fileIndex);

        // Atomic increment to the next index in the file list
        while (!CanProcessFile(nextFile->filePath()))
        {
            if (m_fileIndex > m_filesToScan.count() - 1)
            {
                break;
            }

            m_fileIndex++;
            nextFile = &m_filesToScan.at(m_fileIndex);
        }
        QMetaObject::invokeMethod(m_parent, "OnFileProcessingComplete", Q_ARG(QString, nextFile->filePath()), Q_ARG(int, m_filesToScan.count()));
        m_fileIndex++;
    }
    return nextFile;
}

void ScanWorkManager::doWork()
{
    for (int ix = 0; ix < GPU_PROGRAM_POOL_SIZE; ix++)
    {
        const QFileInfo* nextFile = GetNextFile();
        m_gpuProgramPool.at(ix)->queueLoadOperation(nextFile->filePath());
    }

    for (int ix = 0; ix < GPU_PROGRAM_POOL_SIZE; ix++)
    {
        m_gpuProgramPool.at(ix)->run();
    }

    // QEventLoop eventLoop;
    // eventLoop.exec();
}

void ScanWorkManager::OnStateChanged(void *scanWorkerPtr)
{
    GPUScanWorker *scanWorker = static_cast<GPUScanWorker*>(scanWorkerPtr);
    qDebug() << Q_FUNC_INFO << "State changed for scan worker to: " << ScanWorkerState::ToString(scanWorker->state());
    
    // If the state of the worker is Available and there are more files to scan we'll queue the next file
    if (scanWorker->state() == ScanWorkerState::Available)
    {
        const QFileInfo* nextFile = GetNextFile();
        if(nextFile != nullptr)
        { 
            scanWorker->queueLoadOperation(nextFile->filePath());
            scanWorker->run();
        }
    }
    // If the state of the worker is Ready we'll queue a scan operation to run on the worker
    else if (scanWorker->state() == ScanWorkerState::Ready)
    {
        scanWorker->queueScanOperation();
        scanWorker->run();
    }
    // If the state of the worker is Complete we'll queue a read results operation
    // Once this is done the worker should signal us again with an available state
    else if (scanWorker->state() == ScanWorkerState::Complete)
    {
        scanWorker->queueReadOperation();
        scanWorker->run();
    }
    else
    {
        Q_ASSERT_X(false, Q_FUNC_INFO, QString("This isn't a state we support in work manager. Please examine results: %1").arg(ScanWorkerState::ToString(scanWorker->state())).toStdString().c_str() );
    }
}

void ScanWorkManager::OnInfectionFound(QString filePath)
{
    QMetaObject::invokeMethod(m_parent, "OnInfectionFound", Q_ARG(QString, filePath));
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
        QMetaObject::invokeMethod(m_parent, "OnFileProcessingComplete", Q_ARG(QString, filePath), Q_ARG(int, m_filesToScan.count()));
        return false;
    }
    return true;
}