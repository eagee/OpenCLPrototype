#include "CPUBoundScanOperations.h"


CPUBoundScanOperation::CPUBoundScanOperation(QString &filePath, QObject *parent) :
    QObject(parent), QRunnable(), m_filePath(filePath)
{
}

int CPUBoundScanOperation::RunCPUFileScan()
{
    QFile file(m_filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"Error opening " << m_filePath;
    }

    QDataStream dataStream(&m_data, QIODevice::WriteOnly);
    dataStream << file.read(255);

    // CPU bound checksum operation for first 255 bytes
    int checksum = 0;
    for (int ix = 0; ix < m_data.count(); ix++)
    {
        checksum += m_data.at(ix);
    }
    
    qDebug() << "File: " << m_filePath << "Checksum: " << checksum;

    // TODO: Check to see if checksum represents an infection, and if so trigger
    // the infectionFound signal.

    return checksum;
}

void CPUBoundScanOperation::run()
{
    //qDebug() << "Reading file on thread:" << QThread::currentThread();

    RunCPUFileScan();

    emit processingComplete(m_filePath, 0);
}
