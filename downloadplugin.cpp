#include "downloadplugin.h"

#include <QDebug>
#include <QFile>
#include <QUuid>
#include <QFileInfo>
#include <QTimer>

DownloadPlugin::DownloadPlugin(QObject * parent)
    : DownloadInterface (parent)
{

}

Q_EXPORT_PLUGIN2(DownloadPlugin, DownloadPlugin)

DownloadPlugin::~DownloadPlugin()
{

}

QString DownloadPlugin::name(void) const
{
    return "DownloadPlugin";
}

QString DownloadPlugin::version() const
{
    return "1.0";
}

void DownloadPlugin::append(const QString &_url)
{
    QUrl url = QUrl::fromEncoded(_url.toLocal8Bit());
    bool fileExist = false;
    bool tempExist = false;
    QString fileName = "";
    QString filePath = saveFilename(url, fileExist, fileName, tempExist);

    if (fileExist && m_existPolicy == DownloadInterface::ExistThenCancel) {
        qDebug() << fileName << "exist. Cancel download";
        emit status(_url, "Cancel", "File already exist", filePath);
        return;
    }

    DownloadItem item;
    item.url = _url;
    item.key = fileName;
    item.path = filePath;
    item.temp = filePath + ".part";
    item.tempExist = tempExist;

    if (downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SLOT(startNextDownload()));

    downloadQueue.enqueue(item);
}

void DownloadPlugin::append(const QStringList &urlList)
{
    foreach (QString url, urlList){
        append(url);
    }

    if (downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SIGNAL(finished()));
}

void DownloadPlugin::pause(const QString &url)
{
    QNetworkReply *reply = urlHash[url];
    qDebug() << url << reply;
    disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(downloadProgress(qint64,qint64)));
    disconnect(reply, SIGNAL(finished()),
            this, SLOT(downloadFinished()));
    disconnect(reply, SIGNAL(readyRead()),
            this, SLOT(downloadReadyRead()));
    disconnect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(downloadError(QNetworkReply::NetworkError)));
    disconnect(reply, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(downloadSslErrors(QList<QSslError>)));

    DownloadItem item = downloadHash[reply];
    reply->abort();
    item.file->write( reply->readAll());
    item.file->close();

    downloadHash.remove(reply);
    urlHash.remove(url);

    startNextDownload();

    reply->deleteLater();
}

void DownloadPlugin::pause(const QStringList &urlList)
{
    foreach (QString url, urlList){
        pause(url);
    }
}

void DownloadPlugin::resume(const QString &url)
{
    append(url);
}

void DownloadPlugin::resume(const QStringList & urlList)
{
    foreach (QString url, urlList){
        resume(url);
    }
}

void DownloadPlugin::stop(const QString &url)
{

}

void DownloadPlugin::stop(const QStringList &urlList)
{

}

void DownloadPlugin::setDownloadLimit(int size)
{

}

void DownloadPlugin::startNextDownload()
{
    if (downloadQueue.isEmpty()) {
        emit finishedAll();
        return;
    }

    if (downloadHash.size() < m_queueSize)
    {
        DownloadItem item = downloadQueue.dequeue();

        QNetworkRequest request(item.url);

        if (item.tempExist && m_partialPolicy == DownloadInterface::PartialThenContinue) {
            item.file = new QFile(item.temp);
            if (!item.file->open(QIODevice::ReadWrite)) {
                qDebug() << "Download error" << item.file->errorString();
                emit status(item.url, "Error", item.file->errorString(), item.path);
                startNextDownload();
                return;
            }
            item.file->seek(item.file->size());
            item.tempSize = item.file->size();
            QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(item.tempSize) + "-";
            request.setRawHeader("Range",rangeHeaderValue);
        }
        else {
            if (item.tempExist) {
                QFile::remove(item.temp);
            }

            item.file = new QFile(item.temp);
            if (!item.file->open(QIODevice::ReadWrite)) {
                qDebug() << "Download error" << item.file->errorString();
                emit status(item.url, "Error", item.file->errorString(), item.path);
                startNextDownload();
                return;
            }
            item.tempSize = 0;
        }

        QNetworkReply *reply = manager.get(request);
        connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                this, SLOT(downloadProgress(qint64,qint64)));
        connect(reply, SIGNAL(finished()),
                this, SLOT(downloadFinished()));
        connect(reply, SIGNAL(readyRead()),
                this, SLOT(downloadReadyRead()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(downloadError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
                this, SLOT(downloadSslErrors(QList<QSslError>)));

        qDebug() << "startNextDownload" << item.url << item.temp;
        emit status(item.url, "Download", "Start downloading file", item.url);

        item.time.start();
        downloadHash[reply] = item;
        urlHash[item.url] = reply;
        qDebug() << item.url << reply;

        startNextDownload();
    }
}

void DownloadPlugin::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];
    qint64 actualReceived = item.tempSize + bytesReceived;
    qint64 actualTotal = item.tempSize + bytesTotal;
    double speed = actualReceived * 1000.0 / item.time.elapsed();
    QString unit;
    if (speed < 1024) {
        unit = "bytes/sec";
    } else if (speed < 1024*1024) {
        speed /= 1024;
        unit = "kB/s";
    } else {
        speed /= 1024*1024;
        unit = "MB/s";
    }
    int percent = actualReceived * 100 / actualTotal;

    qDebug() << "downloadProgress" << item.url << bytesReceived << bytesTotal << percent << speed << unit;
    emit progress(item.url, actualReceived, actualTotal, percent, speed, unit);
}

void DownloadPlugin::downloadReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];
    item.file->write(reply->readAll());
}

void DownloadPlugin::downloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];
    item.file->close();
    item.file->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QFile::rename(item.temp, item.path);
        qDebug() << "downloadFinished" << item.url << item.path;

        emit status(item.url, "Complete", "Download file completed", item.url);
        emit finished(item.url, item.path);
    }

    downloadHash.remove(reply);
    urlHash.remove(item.url);

    startNextDownload();

    reply->deleteLater();
}

void DownloadPlugin::downloadError(QNetworkReply::NetworkError) {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];
    qDebug() << "downloadError: " << item.url << reply->errorString();

    emit status(item.url, "Error", reply->errorString(), item.url);
    emit progress(item.url, 0, 0, 0, 0, "bytes/sec");
}

void DownloadPlugin::downloadSslErrors(QList<QSslError>) {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->ignoreSslErrors();
}

void DownloadPlugin::addSocket(QIODevice *socket)
{

}

void DownloadPlugin::removeSocket(QIODevice *socket)
{
}

void DownloadPlugin::transfer()
{
}

void DownloadPlugin::scheduleTransfer()
{
}

QString DownloadPlugin::saveFilename(const QUrl &url, bool &exist, QString &fileName, bool &tempExist)
{
    QString path = url.path();
    QFileInfo fi = QFileInfo(path);
    QString basename = fi.baseName();
    QString suffix = fi.completeSuffix();

    if (basename.isEmpty())
        basename = QUuid::createUuid();

    QString filePath = m_downloadPath + "/" + basename + "." + suffix;

    // check if complete file exist
    if (QFile::exists(filePath))
    {
        exist = true;
        if (m_existPolicy == DownloadInterface::ExistThenRename) {
            qDebug() << "File" << filePath << "exist. Rename";
            int i = 0;
            basename += '(';
            while (QFile::exists(m_downloadPath + "/" + basename + "(" + QString::number(i) + ")" + suffix))
                ++i;

            basename += QString::number(i) + ")";
            filePath = m_downloadPath + "/" + basename + "." + suffix;
        }
        else if (m_existPolicy == DownloadInterface::ExistThenOverwrite) {
            qDebug() << "File" << filePath << "exist. Overwrite";
        }
    }

    // check if part file exist
    QString filePart = filePath + ".part";
    if (QFile::exists(filePart))
    {
        tempExist = true;
    }
    fileName = basename + "." + suffix;

    return filePath;
}
