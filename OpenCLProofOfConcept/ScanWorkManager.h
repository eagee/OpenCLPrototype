#pragma once

#include <QRunnable>
#include <QtCore>
#include <QEventLoop>
#include "GPUScanWorker.h"

class TestScannerListModel;

class ScanWorkManager : public QThread
{
    Q_OBJECT

public:

    ScanWorkManager(QObject *parent = nullptr, TestScannerListModel *testScanner = nullptr);

    int totalFilesToScan();

    void run() override;


private slots:
    void OnInfectionFound(QString filePath);

private:
    QFileInfoList m_filesToScan;
    TestScannerListModel *m_parent;
    GPUScanWorker m_gpuProgram;

    void QueueFileForScan(QString filePath, QEventLoop &eventLoop);
};
