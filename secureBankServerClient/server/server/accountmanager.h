#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QObject>

#include "DatabaseManager.h"
#include "Logger.h"

class AccountManager : public QObject
{
    Q_OBJECT

public:
    AccountManager(DatabaseManager* databaseManager, QObject *parent = nullptr);
    ~AccountManager();

    // Functions related to account management
    QJsonObject login(QJsonObject requestJson);
    QJsonObject getAccountNumber(QJsonObject requestJson);
    QJsonObject getAccountBalance(QJsonObject requestJson);
    QJsonObject createNewAccount(QJsonObject requestJson);
    QJsonObject deleteAccount(QJsonObject requestJson);
    QJsonObject updateUserData(QJsonObject requestJson);
    QJsonObject viewDatabase();

private:
    QString connectionName;
    DatabaseManager* databaseManager = nullptr;
    Logger logger;
};

#endif // ACCOUNTMANAGER_H
