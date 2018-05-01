#pragma once

#include <QRunnable>
#include <QtCore>

class TestScannerListModel;

class QueueFilesOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    QueueFilesOperation(QObject *parent = nullptr, TestScannerListModel *testScanner = nullptr);

    int totalFilesToScan();

    void run();

    void CPUBoundFileScan(QFileInfo &file)
    {
        auto fileToScan = new CPUBoundScanOperation(file.absoluteFilePath());
        QObject::connect(fileToScan, &CPUBoundScanOperation::processingComplete, m_parent, &TestScannerListModel::OnFileProcessingComplete);
        QObject::connect(fileToScan, &CPUBoundScanOperation::infectionFound, m_parent, &TestScannerListModel::OnInfectionFound);
        QThreadPool::globalInstance()->start(fileToScan);
    }

    void GPUBoundFileScan(QFileInfo &file)
    {
        DeviceBoundScanOperation gpuScan(openClProgram);
        QObject::connect(&gpuScan, &DeviceBoundScanOperation::infectionFound, this, &CPUBoundScanOperation::infectionFound);
        bool success = gpuScan.queueOperation(m_filePath);
        if (success)
            gpuScan.run();
    }


private:
    QFileInfoList m_filesToScan;
    TestScannerListModel *m_parent;
    static OpenClProgram openClProgram;
    DeviceBoundScanOperation m_gpuProgram;
};
