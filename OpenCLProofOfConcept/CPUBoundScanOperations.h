#pragma once

#include <QRunnable>
#include <QtCore>

class CPUBoundScanOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    CPUBoundScanOperation(QString &filePath, QObject *parent = nullptr);

    QByteArray data();

    void run();

signals:

    void processingComplete(QString filePath, QByteArray data);

private:
    QString m_filePath;
    QByteArray m_data;
};
