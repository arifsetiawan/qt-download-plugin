#ifndef DOWNLOADPLUGIN_H
#define DOWNLOADPLUGIN_H

#include "downloader.h"
#include <QObject>
#include <QtPlugin>

class DownloadPlugin : public DownloadInterface
{
  Q_OBJECT
  Q_INTERFACES(DownloadInterface)

public:
  DownloadPlugin(QObject * parent = 0);
  ~DownloadPlugin();

  QString name() const;
  QString version() const;

  void download(const QString &url);

private slots:

};

#endif // DOWNLOADPLUGIN_H
