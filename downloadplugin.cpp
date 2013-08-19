#include "downloadplugin.h"

#include <QDebug>

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

void DownloadPlugin::download(const QString &url)
{
    qDebug() << "start download" << url;
}
