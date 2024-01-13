#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QObject>
#include <QFile>
#include <QMutex>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

#include "Logger.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    void initializeDatabase();
    bool openConnection(const QString &connectionName);
    void closeConnection(const QString &connectionName);
    bool createTables(const QString &connectionName);
    QJsonObject processRequest(QJsonObject requestJson, const QString &connectionName);

private:
    QMutex mutex;
    Logger logger;

    QJsonObject login(QJsonObject requestJson, const QString &connectionName);
    QJsonObject getAccountNumber(QJsonObject requestJson, const QString &connectionName);
    QJsonObject getAccountBalance(QJsonObject requestJson, const QString &connectionName);
    QJsonObject createNewAccount(QJsonObject requestJson, const QString &connectionName);
    QJsonObject deleteAccount(QJsonObject requestJson, const QString &connectionName);
    QJsonObject fetchAllUserData(QJsonObject requestJson, const QString &connectionName);
    QJsonObject makeTransaction(QJsonObject requestJson, const QString &connectionName);
    QJsonObject makeTransfer(QJsonObject requestJson, const QString &connectionName);
    QJsonObject viewTransactionHistory(QJsonObject requestJson, const QString &connectionName);
    QJsonObject updateUserData(QJsonObject requestJson, const QString &connectionName);
};

#endif // DATABASEMANAGER_H
