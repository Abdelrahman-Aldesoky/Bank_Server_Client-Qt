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
#include <QRegularExpression>

#include "Logger.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseManager();

    // Functions to manage the database
    bool openConnection();
    void closeConnection();
    void initializeDatabase();
    bool createTables();

    // Specific functions for the banking application. These functions utilize the common database operations.
    QJsonObject login(QJsonObject requestJson);
    QJsonObject getAccountNumber(QJsonObject requestJson);
    QJsonObject getAccountBalance(QJsonObject requestJson);
    QJsonObject createNewAccount(QJsonObject requestJson);
    QJsonObject deleteAccount(QJsonObject requestJson);
    QJsonObject viewDatabase();
    QJsonObject makeTransaction(QJsonObject requestJson);
    QJsonObject makeTransfer(QJsonObject requestJson);
    QJsonObject viewTransactionHistory(QJsonObject requestJson);
    QJsonObject updateUserData(QJsonObject requestJson);

    // Process incoming request and send response back
    QJsonObject processRequest(QJsonObject requestJson);

private:
    QMutex mutex;
    QString connectionName;
    Logger logger;

    // Common database operations used in the industry
    bool startDatabaseTransaction();
    bool commitDatabaseTransaction();
    bool rollbackDatabaseTransaction();

    //The Four Pillars of modularity
    QVariant fetchData(const QString &tableName,
                       const QString &fieldName,
                       const QJsonObject &searchCriteria);
    qint64 insertData(const QString &tableName,
                      const QJsonObject &data);
    bool updateData(const QString &tableName,
                    const QJsonObject &data,
                    const QJsonObject &searchCriteria);
    bool removeData(const QString &tableName,
                    const QJsonObject &searchCriteria);

    // Helper Function for logging transaction
    bool logTransaction(qint64 accountNumber, double amount);
};

#endif // DATABASEMANAGER_H
