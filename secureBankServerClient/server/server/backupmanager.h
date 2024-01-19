#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>

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

private:
    DatabaseManager* dbManager;
    Logger logger;

    //helper function to deleteOldBackups
    void deleteFilesOlderThan(const QFileInfoList& files,
                              const QDateTime& time);
};

#endif // BACKUPMANAGER_H
