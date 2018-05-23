#pragma once

#include <QRunnable>
#include <QtCore>
#include <QQueue>
#include <QEventLoop>
#include <QAtomicInt>
#include "GPUScanWorker.h"

class TestScannerListModel;

class ScanWorkManager : public QObject
{
    Q_OBJECT

public:

    ScanWorkManager(QObject *parent = nullptr, TestScannerListModel *testScanner = nullptr);

    int totalFilesToScan();

    // Called by started() signal of thread this object is moved to
    void doWork();

signals:
    // Indicates to thread this object is moved to that work is finished.
    void workFinished();

private slots:
    void OnInfectionFound(QString filePath);
    void OnStateChanged(void *scanWorkerPtr);

private:
    QFileInfoList m_filesToScan;
    TestScannerListModel *m_parent;
    QList<GPUScanWorker*> m_gpuProgramPool;
    
    QAtomicInt m_fileIndex;

    bool CanProcessFile(QString filePath);
    const QFileInfo* GetNextFile();

};
