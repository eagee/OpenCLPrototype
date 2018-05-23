#pragma once
#include <QThread>
#include "ScanWorkerState.h"

/// Defines interface for ScanWorker implementations
class IScanWorker : public QThread
{
    Q_OBJECT
    
protected:

    struct OperationType
    {
        enum enum_type
        {
            Load,
            ExecuteScan,
            ReadResults
        };
        enum_type Type;
        QString FilePath;
    };

public:

    explicit IScanWorker(QObject* parent = nullptr) : QThread(parent) { }

    virtual ~IScanWorker() { }

    virtual bool queueLoadOperation(QString filePath) = 0;

    virtual bool queueScanOperation() = 0;
    
    virtual bool queueReadOperation() = 0;

    virtual ScanWorkerState::enum_type state() const = 0;

    virtual void setState(ScanWorkerState::enum_type state) = 0;

    virtual bool createFileBuffers(size_t bytesPerFile, int numberOfFiles) = 0;

    virtual bool areBuffersFull() = 0;

    virtual void resetFileBuffers() = 0;

    virtual int queuedFileCount() const = 0;

    virtual void processResults() = 0;

signals:

    void infectionFound(QString filePath);

};