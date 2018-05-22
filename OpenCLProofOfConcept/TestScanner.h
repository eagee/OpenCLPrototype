#pragma once
#include <QObject>
#include <Qthread>
#include <QThreadPool>
#include <QDateTime>
#include <QFileInfoList>
#include <QAbstractListModel>
#include <QTimer>
#include "OpenClProgram.h"
#include "GPUScanWorker.h"
#include "ScanWorkManager.h"


class TestScannerListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentScanObject READ currentScanObject NOTIFY currentScanObjectChanged)
    Q_PROPERTY(int totalItems READ totalItems NOTIFY totalItemsChanged)
    Q_PROPERTY(int itemsScanned READ itemsScanned NOTIFY itemsScannedChanged)
    Q_PROPERTY(QString timeElapsed READ timeElapsed NOTIFY timeElapsedChanged)
    Q_PROPERTY(qreal scanProgress READ scanProgress NOTIFY scanProgressChanged)

public:

    enum Roles
    {
        FilePath = Qt::UserRole
    };

    TestScannerListModel(QObject *parent);

    int totalItems();

    qreal scanProgress();

    QString currentScanObject();

    int itemsScanned();

    QString timeElapsed();

    Q_INVOKABLE void runScan();

    int rowCount(const QModelIndex &) const;

    int columnCount(const QModelIndex &) const;

    QVariant data(const QModelIndex &index, int role) const;

    QHash<int, QByteArray> roleNames() const;

signals:
    void totalItemsChanged();
    void itemsScannedChanged();
    void currentScanObjectChanged();
    void timeElapsedChanged();
    void scanProgressChanged();

public slots:
    void OnFileProcessingComplete(QString filePath, int totalFilesToScan);
    void OnInfectionFound(QString filePath);

private:

    ScanWorkManager m_scanWorkManager;
    QList<QString> m_detections;
    QString m_currentScanObject;
    int m_itemsScanned;
    int m_totalItemsToScan;
    qreal m_startSeconds;
    qreal m_secondsElapsed;
    QDateTime m_startTime;
    QTimer m_timer;

    QString getTimeElapsedString(int secondsElapsed);

    void OnTimerTimeout();
};
