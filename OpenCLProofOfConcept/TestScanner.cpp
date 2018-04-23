#include <QDir>
#include <QByteArray>
#include <QDebug>
#include <memory>
#include <qclcontext.h>
#include "TestScanner.h"
#include "CPUBoundScanObject.h"


TestScannerListModel::TestScannerListModel(QObject *parent): QAbstractListModel(parent),
    m_secondsElapsed(0), m_currentScanObject(""), m_itemsScanned(0), m_startTime(QDateTime::currentDateTime())
{
    QDir testDir("C:\\TestFiles");
    m_filesToScan = testDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
    if(m_filesToScan.count() > 0)
    {
        m_currentScanObject = m_filesToScan.at(0).absoluteFilePath();
        m_itemsScanned = 0;
    }

    beginResetModel();
    m_detections.clear();
    endResetModel();

    m_timer.setInterval(1000);
    QObject::connect(&m_timer, &QTimer::timeout, this, &TestScannerListModel::OnTimerTimeout);
    ConfigureOpenCLObjects();
}

void TestScannerListModel::ConfigureOpenCLObjects()
{
    m_OpenClContext.reset(new QCLContext());
    if (!m_OpenClContext->create()) {
        qCritical() << "Could not create OpenCL context for the GPU";
    }
}

void TestScannerListModel::runScan()
{
    m_timer.start();
    for(auto file : m_filesToScan)
    {
        CPUBoundScanObject *fileToScan = new CPUBoundScanObject(file.absoluteFilePath());
        QObject::connect(fileToScan, &CPUBoundScanObject::processingComplete, this, &TestScannerListModel::OnCPUFilePopulated, Qt::QueuedConnection);
        QThreadPool::globalInstance()->start(fileToScan);

        //beginInsertRows(QModelIndex(), 0, 0);
        //m_detections.push_back(file.absoluteFilePath());
        //endInsertRows();
    }

    m_timer.stop();
}

int TestScannerListModel::totalItems()
{
    return m_filesToScan.count();
}

qreal TestScannerListModel::scanProgress()
{
    if(m_itemsScanned)
    {
        qreal scanProgress = (qreal)m_itemsScanned / (qreal)m_filesToScan.count();
        return scanProgress;
    }
    else
    {
        return 0;
    }
}

QString TestScannerListModel::currentScanObject()
{
    return m_currentScanObject;
}

int TestScannerListModel::itemsScanned()
{
    return m_itemsScanned;
}

QString TestScannerListModel::timeElapsed()
{
    return getTimeElapsedString(m_secondsElapsed);
}

int TestScannerListModel::rowCount(const QModelIndex &) const
{
    return m_detections.count();
}

int TestScannerListModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant TestScannerListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role)
    {

    case FilePath:
        return m_detections.at(index.row());

    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TestScannerListModel::roleNames() const
{
    return{ { FilePath, "filepath" } };
}


QString TestScannerListModel::getTimeElapsedString(int secondsElapsed)
{
    auto secs = secondsElapsed % 60;
    auto mins = secondsElapsed / 60;
    auto hrs = mins / 60;
    mins = mins % 60;   // remaining mins in the hour
    return QString("%1:%2:%3").arg(hrs, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void TestScannerListModel::OnTimerTimeout()
{
    ++m_secondsElapsed;
    emit timeElapsedChanged();
}

void TestScannerListModel::OnCPUFilePopulated(QString filePath, QByteArray data)
{
    // 64bit checksum for Test Trojan is: 2553
    // qint64 checksum64 = 0;
    // for (int ix = 0; ix < data.count(); ix++)
    // {
    //     checksum64 += data.at(ix);
    // }
    // qDebug() << "File: " << filePath << " Data Size: " << data.count() << " Checksum: " << checksum64;

    m_currentScanObject = filePath;
    m_itemsScanned++;
    emit itemsScannedChanged();
    emit scanProgressChanged();
    emit currentScanObjectChanged();
}
