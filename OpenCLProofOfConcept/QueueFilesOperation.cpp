#include <QObject>
#include "QueueFilesOperation.h"
#include "TestScanner.h"
#include "OpenClProgram.h"
#include "CPUBoundScanOperations.h"
#include "DeviceBoundScanOperation.h"

OpenClProgram QueueFilesOperation::openClProgram("createChecksum.cl", "createChecksum", OpenClProgram::DeviceType::GPU);

QueueFilesOperation::QueueFilesOperation(QObject *parent, TestScannerListModel *testScanner): QObject(parent), QRunnable(), m_parent(testScanner), m_gpuProgram(openClProgram)
{
    QDir testDir("C:\\TestFiles");
    m_filesToScan = testDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
}

int QueueFilesOperation::totalFilesToScan()
{
    return m_filesToScan.count();
}

void QueueFilesOperation::run()
{
    for(auto file : m_filesToScan)
    {
        RunCPUBoundScan(file);
    }
}
