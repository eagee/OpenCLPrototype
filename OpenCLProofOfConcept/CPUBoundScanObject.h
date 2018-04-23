#pragma once

#include <QRunnable>
#include <QtCore>

class CPUBoundScanObject : public QObject, public QRunnable
{
    Q_OBJECT

public:

    CPUBoundScanObject(QString &filePath, QObject *parent = nullptr);

    QByteArray data();

    void run();

signals:

    void processingComplete(QString filePath, QByteArray data);

private:
    QString m_filePath;
    QByteArray m_data;
};
