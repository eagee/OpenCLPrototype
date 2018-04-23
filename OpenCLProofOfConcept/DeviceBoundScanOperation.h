#pragma once

#include <QRunnable>
#include <QtCore>
#include <QByteArray>

class QCLContext;

class DeviceBoundScanOperations : public QObject, public QRunnable
{
    Q_OBJECT

public:

    DeviceBoundScanOperations(QObject *parent = nullptr) : QObject(parent), QRunnable(), m_maxOperations(100), m_totalOperations(0)
    {

    }

    void queueOperation(QString filePath, QByteArray *data)
    {
        m_totalOperations++;
    }

    bool isFull()
    {
        return (m_totalOperations == m_maxOperations)
    }

    void run()
    {
        // 64bit checksum for Test Trojan is: 2553
    }

signals:

    void infectionFound(QString filePath);

private:
    int m_totalOperations;
    int m_maxOperations;
    QString m_filePath;
    QByteArray m_data;
    // OpenCL specific objects
    QCLContext m_OpenClContext;

};
