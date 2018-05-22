#pragma once

#include "IScanWorker.h"
#include <memory>
#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>
#include <qclcontext.h>
#include <qclbuffer.h>
#include <qclvector.h>

class OpenClProgram;

class GPUScanWorker : public IScanWorker
{
    Q_OBJECT

public:

    GPUScanWorker(QObject *parent = nullptr);

    ~GPUScanWorker();

    virtual bool queueLoadOperation(QString filePath) override;

    virtual bool queueScanOperation() override;

    void run();

    virtual ScanWorkerState::enum_type state() const override;

    virtual void setState(ScanWorkerState::enum_type state) override;

    virtual bool createFileBuffers(size_t bytesPerFile, int numberOfFiles) override;

    virtual bool areBuffersFull() override;

    void resetFileBuffers() override;

    int queuedFileCount() const override;

    virtual void processResults() override;

private:
    OperationType m_nextOperation;
    ScanWorkerState::enum_type m_state;
    QList<QString> m_filesToScan;
    bool m_buffersCreated;
    int m_maxFiles;
    size_t m_bytesPerFile;
    cl_mem m_fileDataBuffer;
    cl_mem m_dataSizeBuffer;
    cl_mem m_outputBuffer;
    cl_event m_gpuFinishedEvent;
    std::unique_ptr<int[]> m_hostResultData;
    static OpenClProgram m_openClProgram;
    

    bool loadFileData(const QString &filePath);

    void executeKernelOperation();

    static void CL_CALLBACK gpuFinishedCallback(cl_event event, cl_int cmd_exec_status, void *user_data);
};
