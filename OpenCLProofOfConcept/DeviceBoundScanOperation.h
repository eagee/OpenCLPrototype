#pragma once

#include <QRunnable>
#include <QtCore>
#include <QByteArray>
#include <QVector>
#include <qclcontext.h>
#include <qclbuffer.h>
#include <qclvector.h>

class DeviceBoundScanOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    DeviceBoundScanOperation(QObject *parent = nullptr);

    void queueOperation(QString filePath, QByteArray *data);

    bool isFull();

    void run();

signals:

    void infectionFound(QString filePath);

private:
    int m_totalOperations;
    int m_maxOperations;
    QByteArray m_data;
    // OpenCL specific objects
    QCLContext m_OpenClContext;
    QVector<QString> m_filePaths;
    QByteArray m_localBuffer;
    QCLBuffer m_clBuffer;
    QCLVector<long> m_clStartingOffsets;
    QCLVector<long> m_clLengths;
    QCLVector<long> m_clChecksums;

};
