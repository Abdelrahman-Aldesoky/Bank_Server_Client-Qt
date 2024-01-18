#ifndef TRANSACTIONMANAGER_H
#define TRANSACTIONMANAGER_H

#include <QObject>

#include "DatabaseManager.h"
#include "Logger.h"

class TransactionManager : public QObject
{
    Q_OBJECT

public:
    TransactionManager(DatabaseManager* databaseManager, QObject *parent = nullptr);
    ~TransactionManager();

    // Functions related to transaction management
    QJsonObject makeTransaction(QJsonObject requestJson);
    QJsonObject makeTransfer(QJsonObject requestJson);
    QJsonObject viewTransactionHistory(QJsonObject requestJson);

    // Helper Function for logging transaction
    bool logTransaction(qint64 accountNumber, double amount);

private:
    QString connectionName;
    DatabaseManager* databaseManager = nullptr;
    Logger logger;
};

#endif // TRANSACTIONMANAGER_H
