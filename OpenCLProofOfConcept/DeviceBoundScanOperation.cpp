#include"DeviceBoundScanOperation.h"
#include <QMutex>
#include "OpenClProgram.h"

DeviceBoundScanOperation::DeviceBoundScanOperation(OpenClProgram &programToRun, QObject *parent /*= nullptr*/)
    : QObject(parent), QRunnable(), m_openClProgram(programToRun), m_buffersCreated(false), m_fileToScan("")
{

}

DeviceBoundScanOperation::~DeviceBoundScanOperation()
{
    QMutex mutex;
    QMutexLocker locker(&mutex);

    if(m_buffersCreated == true)
    {
        m_openClProgram.ReleaseBuffer(m_fileBuffer);
        m_openClProgram.ReleaseBuffer(m_outputBuffer);
        m_openClProgram.ReleaseBuffer(m_fileSizeBuffer);
    }
}

bool DeviceBoundScanOperation::queueOperation(QString filePath)

{
    m_fileToScan = filePath;
    m_buffersCreated = createFileBuffers(filePath);
    return m_buffersCreated;
}


void DeviceBoundScanOperation::run()
{
    if(!m_buffersCreated)
    {
        qDebug() << Q_FUNC_INFO << " This should never happen, quit it.";
        return;
    }

    // We have to use a mutex so we don't accidentally queue kenel execution at the same time
    QMutex mutex;
    QMutexLocker locker(&mutex);

    // Set our kernel arguments
    if(!m_openClProgram.SetKernelArg(0, sizeof(cl_mem), m_fileBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set buffer arg for file.";
        return;
    }

    if(!m_openClProgram.SetKernelArg(1, sizeof(size_t), m_fileSizeBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set buffer size arg for file.";
        return;
    }

    if(!m_openClProgram.SetKernelArg(2, sizeof(int), m_outputBuffer))
    {
        qDebug() << Q_FUNC_INFO << " Failed to set output buffer arg for file.";
        return;
    }

    const size_t workItemSize = 1;
    const size_t workGroupSize = 1;
    int result = m_openClProgram.ExecuteKernel(workItemSize, workGroupSize, m_outputBuffer);
    qDebug() << "File: " << m_fileToScan << "Checksum: " << result;
    //if(result == 14330)
    //{
    //    qDebug() << Q_FUNC_INFO << " Infection found in " << m_fileToScan;
    //    emit infectionFound(m_fileToScan);
    //}
}

bool DeviceBoundScanOperation::createFileBuffers(QString scanFileName)
{
    QMutex mutex;
    QMutexLocker locker(&mutex);

    if(!QFile::exists(scanFileName))
    {
        qDebug() << Q_FUNC_INFO << " File: " << scanFileName << " not found.";
        return false;
    }

    QFile file(scanFileName);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << Q_FUNC_INFO << "Unable to open file" << scanFileName;
        return false;
    }

    bool success = false;
    QByteArray clFileBufferData = file.read(255);
    void *data = (void*)clFileBufferData.constData();
    success = m_openClProgram.CreateBuffer(m_fileBuffer, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, clFileBufferData.size(), data);
    if(!success)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file buffer";
        return false;
    }

    size_t scanFilelength = clFileBufferData.size();
    success = m_openClProgram.CreateBuffer(m_fileSizeBuffer, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(size_t), &scanFilelength);
    if(!success)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem file size";
        return false;
    }

    int buffer;
    success = m_openClProgram.CreateBuffer(m_outputBuffer, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &buffer);
    if(!success)
    {
        qDebug() << Q_FUNC_INFO << " Failed to create cl_mem output buffer";
        return false;
    }

    return true;
}

