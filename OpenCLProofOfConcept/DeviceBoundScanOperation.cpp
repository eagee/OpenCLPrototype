#include "DeviceBoundScanOperation.h"

DeviceBoundScanOperation::DeviceBoundScanOperation(QObject *parent) : QObject(parent), m_maxOperations(100), m_totalOperations(0) // QRunnable(), 
{
    if (!m_OpenClContext.create(QCLDevice::GPU)) {
        qCritical() << "Could not create OpenCL context for the GPU!";
        exit(EXIT_FAILURE);
    }
    m_clStartingOffsets = m_OpenClContext.createVector<long>(m_maxOperations);
    m_clLengths = m_OpenClContext.createVector<long>(m_maxOperations);
    m_clChecksums = m_OpenClContext.createVector<long>(m_maxOperations);
    m_program = m_OpenClContext.buildProgramFromSourceFile(QLatin1String(":/createChecksum.cl"));
    m_kernel = m_program.createKernel("createChecksum");
}

void DeviceBoundScanOperation::queueOperation(QString filePath, QByteArray *data)
{
    if(m_totalOperations == m_maxOperations)
    {
        return;
    }

    m_filePaths.push_back(filePath);

    if(m_totalOperations == 0)
    {
        m_clStartingOffsets[m_totalOperations] = 0;
    }
    else {
        m_clStartingOffsets[m_totalOperations] = m_clStartingOffsets[m_totalOperations - 1] + 1;
    }
    
    m_clLengths[m_totalOperations] = data->count();
    m_clChecksums[m_totalOperations] = 0;
    
    QDataStream dataStream(&m_localBuffer, QIODevice::WriteOnly);
    dataStream << data;

    m_totalOperations++;
}

bool DeviceBoundScanOperation::isFull()
{
    return (m_totalOperations == m_maxOperations);
}

void DeviceBoundScanOperation::run()
{
    // Set up our program and kernel based on the opencl objects
    m_clBuffer = m_OpenClContext.createBufferCopy(reinterpret_cast<char*>(m_localBuffer.data()), (m_localBuffer.count() / sizeof(char)), QCLMemoryObject::ReadOnly);

    m_kernel.setGlobalWorkSize(m_totalOperations + 1);

    // Execute all objects using an opencl kernel
    m_kernel(m_clBuffer, m_clStartingOffsets, m_clLengths, m_clChecksums);

    QMetaObject::invokeMethod(this, "evaluateChecksums");

}


void DeviceBoundScanOperation::evaluateChecksums()
{
    // then iterate through pour buffers and send out a signal for any infections found.
    // 64bit checksum for Test Trojan is: 2553
    for (int ix = 0; ix < m_totalOperations - 1; ix++)
    {
        qDebug() << m_filePaths.at(ix) << "Checksum: " << m_clChecksums[ix];
        if (m_clChecksums[ix] == 2553)
        {
            emit infectionFound(m_filePaths.at(ix));
        }
    }
}
