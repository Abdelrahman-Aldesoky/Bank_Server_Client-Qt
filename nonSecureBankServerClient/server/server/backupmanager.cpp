#include "backupmanager.h"

BackupManager::BackupManager(DatabaseManager* dbManager, QObject *parent)
    : QObject(parent), dbManager(dbManager), logger("BackupManager")
{
    logger.log("BackupManager Object Created.");
}

BackupManager::~BackupManager()
{
    logger.log("BackupManager Object Destroyed.");
}

void BackupManager::createFullBackup()
{
    QDir dir;
    if (!dir.exists("backup"))
    {
        dir.mkpath("backup");
    }

    if (!dbManager->openConnection())
    {
        logger.log("Failed to open database connection.");
        return;
    }

    QSqlDatabase dbConnection = dbManager->getDatabase();
    QSqlQuery backUpQuery(dbConnection);

    // Generate a timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");

    // Save the backup in the backup directory with a timestamp in its name
    QString backupFileName = QString("backup/backup_%1.db").arg(timestamp);
    QString vacuumCommand = QString("VACUUM INTO '%1';").arg(backupFileName);
    if (!backUpQuery.exec(vacuumCommand))
    {
        logger.log("Backup Creation Failed: " + backUpQuery.lastError().text());
        dbManager->closeConnection();
        return;
    }

    logger.log("Created a full backup of the database.");

    deleteOldBackups();

    dbManager->closeConnection();
}

void BackupManager::deleteOldBackups()
{
    QDir dir("backup");
    if (!dir.exists())
    {
        return;
    }

    QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Time);
    QDateTime oneMonthAgo = QDateTime::currentDateTime().addMonths(-1);

    deleteFilesOlderThan(files, oneMonthAgo);
}

void BackupManager::deleteFilesOlderThan(const QFileInfoList& files,
                                         const QDateTime& time)
{
    for (const QFileInfo &file : files)
    {
        if (file.birthTime() < time)
        {
            QFile::remove(file.absoluteFilePath());
        }
    }
}

void BackupManager::handleShutdown()
{
    logger.log("Received shutdown signal. Creating a full backup before shutting down.");
    createFullBackup();
}
