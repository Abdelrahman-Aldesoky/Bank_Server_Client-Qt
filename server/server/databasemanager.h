#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSettings>
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
    QJsonObject login(const QString &username,
                      const QString &password,
                      const QString &connectionName);
    QJsonObject getAccountNumber(const QString &username,
                                 const QString &connectionName);
    double getAccountBalance(qint64 accountNumber,
                             const QString &connectionName);
    QJsonObject createNewAccount(const QJsonObject &jsonObject,
                                 const QString &connectionName);
    bool deleteAccount(qint64 accountNumber, const QString &connectionName);
    QJsonObject fetchAllUserData(const QString &connectionName);
    QJsonObject makeTransaction(const QJsonObject &jsonObject,
                                const QString &connectionName);
    QJsonObject makeTransfer(qint64 fromAccountNumber, qint64 toAccountNumber,
                             double amount, const QString &connectionName);
    QJsonArray viewTransactionHistory(qint64 accountNumber,
                                      const QString &connectionName);
    QJsonObject updateUserData(const QJsonObject &jsonObject,
                                                const QString &connectionName);

private:
    QMutex mutex;
    Logger logger;
};

#endif // DATABASEMANAGER_H
