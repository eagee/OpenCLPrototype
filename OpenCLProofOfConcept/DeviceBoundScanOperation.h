#pragma once

#include <QRunnable>
#include <QtCore>

class DeviceBoundScanOperation : public QObject, public QRunnable
{
    Q_OBJECT

public:

    DeviceBoundScanOperation(QString &filePath, QObject *parent = nullptr)
    {

    }

    void run()
    {
        // 64bit checksum for Test Trojan is: 2553
    }

signals:

    void infectionFound(QString filePath);

private:
    QString m_filePath;
    QByteArray m_data;
};
