#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>


#include "DatabaseManager.h"
#include "Logger.h"

class BackupManager : public QObject
{
    Q_OBJECT

public:
    BackupManager(DatabaseManager* dbManager, QObject *parent = nullptr);
    ~BackupManager();

    void createFullBackup();
    void deleteOldBackups();
    void handleShutdown();

private slots:
    void handleEmailSent();

private:
    DatabaseManager* dbManager;
    QNetworkReply *reply = nullptr;
    Logger logger;

    //helper function to deleteOldBackups and e-mail notifications
    void deleteFilesOlderThan(const QFileInfoList& files,
                              const QDateTime& time);
    void emailNotificationAfterBackup();
};

#endif // BACKUPMANAGER_H
