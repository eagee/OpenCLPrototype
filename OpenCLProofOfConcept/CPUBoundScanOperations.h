#pragma once

#include <QRunnable>
#include <QtCore>

class OpenClProgram;

class CPUBoundScanOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    CPUBoundScanOperation(QString &filePath, QObject *parent = nullptr);

    void run();

    int RunCPUFileScan();

    void RunGPUFileScan();

signals:

    void processingComplete(QString filePath, int checksum);
    void infectionFound(QString filePath);

private:
    QString m_filePath;
    QByteArray m_data;
};
