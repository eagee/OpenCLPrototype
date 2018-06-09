#pragma once

#include "IScanWorker.h"
#include <memory>
#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>

class OpenClProgram;

class CPUScanWorker : public IScanWorker
{
    Q_OBJECT

public:

    CPUScanWorker(QObject *parent = nullptr);

    ~CPUScanWorker();

    virtual bool queueLoadOperation(QString filePath) override;

    virtual bool queueScanOperation() override;

    virtual bool queueReadOperation() override;

    void run();

    virtual ScanWorkerState::enum_type state() const override;

    virtual void setState(ScanWorkerState::enum_type state) override;

    virtual bool createFileBuffers(size_t bytesPerFile, int numberOfFiles) override;

    virtual bool areBuffersFull() override;

    void resetFileBuffers() override;

    int queuedFileCount() const override;

    virtual void processResults() override;

public slots:
    void OnSetStateAvailabile()
    {
        setState(ScanWorkerState::Available);
    }

    void OnSetStateReady()
    {
        setState(ScanWorkerState::Ready);
    }

    void OnSetStateComplete()
    {
        setState(ScanWorkerState::Complete);
    }

signals:
    void stateChanged(void *scanWorkerPtr);

private:
    OperationType m_nextOperation;
    ScanWorkerState::enum_type m_state;
    QList<QString> m_filesToScan;
    bool m_buffersCreated;
    int m_maxFiles;
    size_t m_bytesPerFile;
    QByteArray m_fileDataBuffer;
    size_t m_dataSizeBuffer;
    std::unique_ptr<int[]> m_hostResultData;
    static OpenClProgram m_openClProgram;

    bool loadFileData(const QString &filePath);

    void executeKernelOperation();
};
//Q_DECLARE_METATYPE(GPUScanWorker, "GPUScanWorker")
