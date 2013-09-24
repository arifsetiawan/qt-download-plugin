#ifndef DOWNLOADPLUGIN_H
#define DOWNLOADPLUGIN_H

#include "downloader.h"
#include <QObject>
#include <QtPlugin>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QUrl>
#include <QTime>
#include <QSet>
#include <QString>
#include <QFile>
#include <QHash>
#include <QStringList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>

struct DownloadItem{
    QString key;
    QString url;
    QString path;
    QString temp;
    QFile *file;
    QTime time;
    bool tempExist;
    qint64 tempSize;
};

class DownloadPlugin : public DownloadInterface
{
    Q_OBJECT
    Q_INTERFACES(DownloadInterface)

public:
    DownloadPlugin(QObject * parent = 0);
    ~DownloadPlugin();

    QString name() const;
    QString version() const;
    void setDefaultParameters();

    void append(const QString &url);
    void append(const QStringList & urlList);

    void pause(const QString &url);
    void pause(const QStringList & urlList);

    void resume(const QString &url);
    void resume(const QStringList & urlList);

    void stop(const QString &url);
    void stop(const QStringList & urlList);

    void setBandwidthLimit(int size);

    QString getStatus() const;

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadReadyRead();
    void downloadFinished();
    void downloadError(QNetworkReply::NetworkError);
    void downloadSslErrors(QList<QSslError>);

private:
    void addSocket(QIODevice *socket);
    void removeSocket(QIODevice *socket);
    void transfer();
    void scheduleTransfer();

    void stopDownload(const QString &url, bool pause);
    QString saveFilename(const QUrl &url, bool &exist, QString &fileName, bool &tempExist);

private:
    QNetworkAccessManager manager;
    QQueue<DownloadItem> downloadQueue;
    QHash<QNetworkReply*, DownloadItem> downloadHash;
    QHash<QString, QNetworkReply*> urlHash;
    QList<DownloadItem> completedList;

    QSet<QIODevice*> replies;
    QTime stopWatch;
    bool transferScheduled;
};

#endif // DOWNLOADPLUGIN_H
