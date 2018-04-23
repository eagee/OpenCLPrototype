#include "CPUBoundScanOperation.h"

CPUBoundScanOperation::CPUBoundScanOperation(QString &filePath, QObject *parent) : QObject(parent), QRunnable(), m_filePath(filePath)
{
}

QByteArray CPUBoundScanOperation::data()
{
    return m_data;
}

void CPUBoundScanOperation::run()
{
    qDebug() << "Reading PE HEader from file on thread:" << QThread::currentThread();

    QFile file(m_filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"Error opening " << m_filePath;
    }

    QDataStream dataStream(&m_data, QIODevice::WriteOnly);
    dataStream << file.read(0xFF);

    emit processingComplete(m_filePath, m_data);
}
