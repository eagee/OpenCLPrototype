#pragma once

#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>
#include <qclcontext.h>
#include <qclbuffer.h>
#include <qclvector.h>

class OpenClProgram;

class DeviceBoundScanOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    DeviceBoundScanOperation(OpenClProgram &programToRun, QObject *parent = nullptr);

    ~DeviceBoundScanOperation();

    bool queueOperation(QString filePath);

    void run();

signals:

    void infectionFound(QString filePath);

private:
    QString m_fileToScan;
    OpenClProgram &m_openClProgram;
    cl_mem m_fileBuffer;
    cl_mem m_fileSizeBuffer;
    cl_mem m_outputBuffer;
    bool m_buffersCreated;

    bool createFileBuffers(QString scanFileName);
};
