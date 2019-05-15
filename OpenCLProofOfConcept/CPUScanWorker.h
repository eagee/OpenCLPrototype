#pragma once

#include "IScanWorker.h"
#include <memory>
#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>

class OpenClProgram;

// Types we'll use for calculating the md5
#define BLOCK_SIZE 2048
#define TOTAL_CHECKSUMS ((1024*1024*2) - 8) //1048568 

typedef struct
{
    ulong values[TOTAL_CHECKSUMS];
} DdsChecksums;


/// Implements QRunnable to right highly parallel DDS checksum operations based on the file buffer/data specified in the constructor
class CPUKernelRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:

    CPUKernelRunnable(QByteArray &fileDataBuffer, int fileIndex, int checksumIndex, DdsChecksums &checksums, unsigned int bytesPerFile);

protected:
    void run();

signals:

    void ProcessingComplete(int fileIndex, int checksumIndex, QString dds);

private:
    QByteArray &m_fileDataBuffer;
    int m_fileIndex;
    int m_checksumIndex;
    unsigned int m_bytesPerFile;
    DdsChecksums &m_checksums;

    void generateMd5();
    void generateDDSSig();

    // same method as below, but used for QRunnable implementation (which should garner far better performance)
    ulong createDdsChecksum64(const ulong *buffer);
};


class CPUScanWorker : public IScanWorker
{
    Q_OBJECT

public:

    CPUScanWorker(QObject *parent = nullptr, QString id = "");

    ~CPUScanWorker();

    virtual QString id() override;

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

    void OnProcessingComplete(int fileIndex, int checksumIndex, QString md5);

signals:
    void stateChanged(void *scanWorkerPtr);

private:
    OperationType m_nextOperation;
    ScanWorkerState::enum_type m_state;
    QList<QString> m_filesToScan;
    bool m_buffersCreated;
    int m_maxFiles;
    unsigned int m_bytesPerFile;
    QByteArray m_fileDataBuffer;
    unsigned int m_dataSizeBuffer;
    QMap<int, QString> m_hostResultData;
    static OpenClProgram m_openClProgram;
    int m_pendingWorkItems;
    QString m_id;
    DdsChecksums m_checksums;
    int m_checksumsToRun;

    bool loadFileData(const QString &filePath);

    void executeKernelOperation();

    ulong createDdsChecksum64(const ulong *buffer);
};
//Q_DECLARE_METATYPE(GPUScanWorker, "GPUScanWorker")
