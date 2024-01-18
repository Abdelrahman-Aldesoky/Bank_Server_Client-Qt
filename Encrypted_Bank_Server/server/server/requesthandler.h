#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>

#include "AccountManager.h"
#include "TransactionManager.h"
#include "DatabaseManager.h"
#include "Logger.h"

class RequestHandler : public QObject
{
    Q_OBJECT

public:
    RequestHandler(DatabaseManager* databaseManager, QObject *parent = nullptr);
    ~RequestHandler();

    QByteArray handleRequest(QByteArray requestData);

private:
    QMutex mutex;
    QString connectionName;
    AccountManager *accountManager = nullptr;
    TransactionManager *transactionManager = nullptr;
    DatabaseManager* databaseManager = nullptr;
    Logger logger;
};

#endif // REQUESTHANDLER_H
