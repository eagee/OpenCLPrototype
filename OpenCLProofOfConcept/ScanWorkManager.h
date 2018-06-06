#pragma once

#include <QRunnable>
#include <QtCore>
#include <QQueue>
#include <QEventLoop>
#include <QAtomicInt>
#include "CPUScanWorker.h"
#include "GPUScanWorker.h"

class TestScannerListModel;

class ScanWorkManager : public QObject
{
    Q_OBJECT

public:

    ScanWorkManager(QObject *parent = nullptr);

    int totalFilesToScan();

signals:
    void workFinished();
    void fileProcessingComplete(QString filePath, int totalFilesToScan);
    void infectionFound(QString filePath);

public slots:
    // Called by started() signal of thread this object is moved to
    void doWork();

private slots:
    void OnInfectionFound(QString filePath);
    void OnStateChanged(void *scanWorkerPtr);

private:
    QScopedPointer<QFileInfoList> m_filesToScan;
    QList<IScanWorker*> m_programPool;
    QScopedPointer<QAtomicInt> m_fileIndex;

    bool CanProcessFile(QString filePath);
    const QFileInfo* GetNextFile();

    /// Handles initializing all the objects that will need to interact with opencl and the scanworkmanager.
    /// Note: This objects must be initialized inside of the doWork operation or they'll exist on the calling
    ///       or UI thread instead of the thread ScanWorkManager is moved to.
    void InitWorkers();
};
