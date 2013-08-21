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
    bool partExist = false;
    QString fileName = "";
    QString filePath = saveFilename(url, fileExist, fileName, partExist);

    if (fileExist && m_existPolicy == DownloadInterface::ExistThenCancel) {
        qDebug() << fileName << "exist. Cancel download";
        return;
    }

    DownloadItem item;
    item.url = _url;
    item.key = fileName;
    item.path = filePath;
    item.temp = filePath + ".part";

    qDebug() << "Download file" << filePath;

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

}

void DownloadPlugin::pause(const QStringList &urlList)
{

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

    DownloadItem item = downloadQueue.dequeue();

    item.file = new QFile(item.temp);
    if (!item.file->open(QIODevice::ReadWrite)) {
        qDebug() << "Download error" << item.file->errorString();
        emit error(item.url, item.file->errorString());
        startNextDownload();
        return;
    }

    QNetworkRequest request(item.url);
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
    item.time.start();
    downloadHash[reply] = item;

    if (downloadHash.size() < m_downloadLimit)
        startNextDownload();
}

void DownloadPlugin::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];

    double speed = bytesReceived * 1000.0 / item.time.elapsed();
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
    double percent = bytesReceived * 100.0 / bytesTotal;

    //qDebug() << "downloadProgress" << item.url << bytesReceived << bytesTotal << percent << speed << unit;
    emit progress(item.url, bytesReceived, bytesTotal, percent, speed, unit);
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
    bool a = QFile::rename(item.temp, item.path);

    qDebug() << "downloadFinished" << item.url << item.path << a;
    emit finished(reply->url().toString(), item.path);

    downloadHash.remove(reply);
    if (downloadHash.size() < m_downloadLimit)
        startNextDownload();

    reply->deleteLater();
}

void DownloadPlugin::downloadError(QNetworkReply::NetworkError) {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    DownloadItem item = downloadHash[reply];
    item.file->close();
    item.file->deleteLater();

    qDebug() << "downloadError: " << item.url << reply->errorString();
    emit error(item.url, reply->errorString());

    downloadHash.remove(reply);
    if (downloadHash.size() < m_downloadLimit)
        startNextDownload();

    reply->deleteLater();
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

QString DownloadPlugin::saveFilename(const QUrl &url, bool &exist, QString &fileName, bool &partExist)
{
    QString path = url.path();
    QFileInfo fi = QFileInfo(path);
    QString basename = fi.baseName();
    QString suffix = fi.completeSuffix();

    if (basename.isEmpty())
        basename = QUuid::createUuid();

    QString filePath = m_downloadPath + "/" + basename + "." + suffix;
    exist = false;
    // check if complete file exist
    if (QFile::exists(filePath))
    {
        exist = true;
        if (m_existPolicy == DownloadInterface::ExistThenRename) {
            qDebug() << filePath << "exist. Rename";
            int i = 0;
            basename += '(';
            while (QFile::exists(m_downloadPath + "/" + basename + "(" + QString::number(i) + ")" + suffix))
                ++i;

            basename += QString::number(i) + ")";
            filePath = m_downloadPath + "/" + basename + "." + suffix;
        }
        else if (m_existPolicy == DownloadInterface::ExistThenOverwrite) {
            qDebug() << filePath << "exist. Overwrite";
        }
    }

    // check if part file exist
    QString filePart = filePath + ".part";
    if (QFile::exists(filePart))
    {
        partExist = true;
    }
    fileName = basename + "." + suffix;

    return filePath;
}
