#include <QDir>
#include <QByteArray>
#include <QDebug>
#include <memory>
#include "CPUBoundScanOperations.h"
#include "GPUScanWorker.h"
#include "TestScanner.h"
#include "ScanWorkManager.h"


TestScannerListModel::TestScannerListModel(QObject *parent): QAbstractListModel(parent),
    m_secondsElapsed(0),
    m_startSeconds(0),
    m_currentScanObject(""), 
    m_itemsScanned(0),
    m_totalItemsToScan(0),
    m_startTime(QDateTime::currentDateTime())
{
    m_currentScanObject = ""; // m_filesToScan.at(0).absoluteFilePath();
    m_itemsScanned = 0;

    beginResetModel();
    m_detections.clear();
    endResetModel();

    m_timer.setInterval(100);
    QObject::connect(&m_timer, &QTimer::timeout, this, &TestScannerListModel::OnTimerTimeout);

    m_scanThread = new QThread();
}

void TestScannerListModel::runScan()
{
    m_itemsScanned = 0;
    m_secondsElapsed = 0;
    m_startSeconds = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000;
    m_timer.start();

    // Set up our scan work manager so that it runs all of it's operations on another thread using it's own event loop
    m_scanWorker = new ScanWorkManager();
    m_scanWorker->moveToThread(m_scanThread);
    QObject::connect(m_scanThread, &QThread::started, m_scanWorker, &ScanWorkManager::doWork);
    QObject::connect(m_scanWorker, &ScanWorkManager::workFinished, m_scanThread, &QThread::quit);
    QObject::connect(m_scanWorker, &ScanWorkManager::workFinished, m_scanThread, &ScanWorkManager::deleteLater);
    QObject::connect(m_scanWorker, &ScanWorkManager::infectionFound, this, &TestScannerListModel::OnInfectionFound);
    QObject::connect(m_scanWorker, &ScanWorkManager::fileProcessingComplete, this, &TestScannerListModel::OnFileProcessingComplete);
    QObject::connect(m_scanThread, &QThread::finished, m_scanThread, &QThread::deleteLater);
    m_scanThread->start();
}

int TestScannerListModel::totalItems()
{
    return m_totalItemsToScan;
}

qreal TestScannerListModel::scanProgress()
{
    if(m_itemsScanned)
    {
        qreal scanProgress = (qreal)m_itemsScanned / (qreal)m_totalItemsToScan;
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
    m_secondsElapsed = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000 - m_startSeconds;
    emit timeElapsedChanged();
    emit totalItemsChanged();
    emit itemsScannedChanged();
    emit currentScanObjectChanged();
    emit scanProgressChanged();
}

void TestScannerListModel::OnFileProcessingComplete(QString filePath, int totalFilesToScan)
{
    m_totalItemsToScan = totalFilesToScan;
    m_currentScanObject = filePath;
    m_itemsScanned++;
    if(m_itemsScanned == m_totalItemsToScan)
    {
        m_timer.stop();
        emit timeElapsedChanged();
        emit totalItemsChanged();
        emit itemsScannedChanged();
        emit currentScanObjectChanged();
        emit scanProgressChanged();
    }
}

void TestScannerListModel::OnInfectionFound(QString filePath)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_detections.push_back(filePath);
    endInsertRows();
}
