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

/*
 * This function sends an email notification after a successful backup using SendGrid's API.
 * In order to use this function, you need to sign up for SendGrid or any mail service
   and obtain an API key.
 *
 * * Additional Requirements:
 * - You must use a work email (like `name@yourcompany.com`) rather than a personal email address.
 * - tried mailgun also same story.
 *
 * Here are the steps to get started with SendGrid:
 * 1. Go to https://sendgrid.com and click on 'Start For Free'.
 * 2. Fill out the form to create a new account.
 * 3. Once your account is created, go to Settings > API Keys in the SendGrid dashboard.
 * 4. Click on 'Create API Key', give it a name, and set the permissions. For sending emails,
      you can use a 'Restricted Access' key with 'Mail Send' permissions.
 * 5. Click on 'Create & View' to create the API key.
      Make sure to copy the API key and store it somewhere safe, as you won't be able to view it again.
 * 6. Replace 'YOUR_SENDGRID_API_KEY' in the code below with your actual SendGrid API key.
 *
 *
 * Please note that SendGrid's free tier allows you to send a limited number of emails per day.
 * If you need to send more emails, you may need to upgrade to a paid plan.
 *
 * Disclaimer: Please be aware that sending emails programmatically must comply with
 * all relevant laws and terms of service. Be sure to respect privacy and use this responsibly.
 */
void BackupManager::emailNotificationAfterBackup()
{
    QUrl url("https://api.sendgrid.com/v3/mail/send");

    QJsonObject mailObject;
    QJsonObject personalization;
    QJsonArray toAddresses;
    QJsonObject toAddress;
    // here goes the e-mail that you will be getting notifications on
    toAddress["email"] = "abdelrahman.mohamed.aldesoky@gmail.com";
    toAddresses.append(toAddress);
    personalization["to"] = toAddresses;
    QJsonArray personalizations;
    personalizations.append(personalization);
    mailObject["personalizations"] = personalizations;

    QJsonObject fromObject;
    //you send grid email that you signed up with goes here
    fromObject["email"] = "wolviernee@gmail.com";
    mailObject["from"] = fromObject;

    mailObject["subject"] = "Backup Successful";

    QJsonObject contentObject;
    contentObject["type"] = "text/plain";
    contentObject["value"] = "A full backup of the database was created.";
    QJsonArray content;
    content.append(contentObject);
    mailObject["content"] = content;

    QByteArray data = QJsonDocument(mailObject).toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    //you send grid api key goes here
    request.setRawHeader("Authorization", "Bearer YOUR_SENDGRID_API_KEY");


    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    reply = manager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &BackupManager::handleEmailSent);
}

void BackupManager::handleEmailSent()
{
    if (reply)
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            logger.log("Email sent successfully");
        }
        else
        {
            logger.log("Failed to send email: " + reply->errorString());
        }
        reply->deleteLater();
        reply = nullptr;
    }
    else
    {
        logger.log("Reply object is null");
    }
}

void BackupManager::handleShutdown()
{
    logger.log("Received shutdown signal. Creating a full backup before shutting down.");
    createFullBackup();
}
