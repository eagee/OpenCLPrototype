#include <QObject>
#include "ScanWorkManager.h"
#include "TestScanner.h"
#include "OpenClProgram.h"
#include "CPUBoundScanOperations.h"
#include "GPUScanWorker.h"

#pragma OPENCL EXTENSION cl_nvidia_printf : enable
const int bytesPerFile = 255;

ScanWorkManager::ScanWorkManager(QObject *parent, TestScannerListModel *testScanner): QThread(parent), m_parent(testScanner), m_gpuProgram(this)
{
    QDir testDir("C:\\TestFiles");
    m_filesToScan = testDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
    m_gpuProgram.createFileBuffers(bytesPerFile, 2048);
}

int ScanWorkManager::totalFilesToScan()
{
    return m_filesToScan.count();
}

bool CanProcessFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << Q_FUNC_INFO << "Unable to open file" << fileName;
        return false;
    }

    QByteArray clFileBufferData = file.read(bytesPerFile);
    if (bytesPerFile != clFileBufferData.size())
    {
        return false;
    }
    return true;
}

void ScanWorkManager::run()
{
    QEventLoop eventLoop;
    QObject::connect(&m_gpuProgram, &GPUScanWorker::stateChanged, &eventLoop, &QEventLoop::quit);
    QObject::connect(&m_gpuProgram, &GPUScanWorker::infectionFound, this, &ScanWorkManager::OnInfectionFound);
    for (auto file : m_filesToScan)
    {
        if (CanProcessFile(file.filePath()))
        {
            QueueFileForScan(file.filePath(), eventLoop);
            if (m_gpuProgram.state() == ScanWorkerState::Ready)
            {
                m_gpuProgram.queueScanOperation();
                m_gpuProgram.run();
                eventLoop.exec();
                m_gpuProgram.resetFileBuffers();
            }
        }
    }
}

void ScanWorkManager::OnInfectionFound(QString filePath)
{
    QMetaObject::invokeMethod(m_parent, "OnInfectionFound", Q_ARG(QString, filePath));
}

void ScanWorkManager::QueueFileForScan(QString filePath, QEventLoop &eventLoop)
{
    m_gpuProgram.queueLoadOperation(filePath);
    m_gpuProgram.run();
    //eventLoop.exec();
    QMetaObject::invokeMethod(m_parent, "OnFileProcessingComplete", Q_ARG(QString, filePath), Q_ARG(int, m_filesToScan.count()));
}
