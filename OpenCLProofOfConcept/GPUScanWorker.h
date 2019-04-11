#pragma once

#include "IScanWorker.h"
#include <memory>
#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>
#include <CL\cl.h>
#include <chrono>
#include <Windows.h>
#include <winnt.h>


class OpenClProgram;

class GPUScanWorker : public IScanWorker
{
    Q_OBJECT

public:

    GPUScanWorker(QObject *parent = nullptr, QString id = "");

    ~GPUScanWorker();

    virtual QString id() override;

    virtual bool queueLoadOperation(QString filePath) override;

    virtual bool queueScanOperation() override;

    virtual bool queueReadOperation() override;

    void run();

    void readResultsFromGPU();

    virtual ScanWorkerState::enum_type state() const override;

    virtual void setState(ScanWorkerState::enum_type state) override;

    virtual bool createFileBuffers(size_t bytesPerFile, int numberOfFiles) override;

    virtual bool areBuffersFull() override;

    void resetFileBuffers() override;

    int queuedFileCount() const override;

    virtual void processResults() override;

    void printTotalTime();

    qint64 GetProfileTimeElapsed() const;
    void SetProfileDateTime();
signals:
    void stateChanged(void *scanWorkerPtr);

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

    void OnSetStateDone()
    {
        setState(ScanWorkerState::Done);
    }

private:
    OperationType m_nextOperation;
    ScanWorkerState::enum_type m_state;
    QList<QString> m_filesToScan;
    bool m_buffersCreated;
    int m_maxFiles;
    size_t m_bytesPerFile;
    char* m_mappedHostFileData;
    cl_mem m_gpuFileDataBuffer;
    cl_mem m_dataSizeBuffer;
    cl_mem m_outputBuffer;
    cl_event m_gpuFinishedEvent;
    cl_command_queue m_commandQueue;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_finishTime;
    std::unique_ptr<char[]> m_hostResultData;
    static OpenClProgram m_openClProgram;
    QString m_id;
    QDateTime m_profileDateTime;
    LARGE_INTEGER m_StartingTime;
    LARGE_INTEGER m_Frequency;


    bool loadFileData(const QString &filePath);

    void executeKernelOperation();

    static void CL_CALLBACK gpuFinishedCallback(cl_event event, cl_int cmd_exec_status, void *user_data);
};
//Q_DECLARE_METATYPE(GPUScanWorker, "GPUScanWorker")
