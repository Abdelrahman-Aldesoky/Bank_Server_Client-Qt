#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "databasemanager.h"
#include "logger.h"

class RequestHandler : public QObject
{
    Q_OBJECT

public:
    explicit RequestHandler(DatabaseManager *databaseManager,
                            const QString &connectionName,
                            QObject *parent = nullptr);
    void handleDifferentClientRequests(QByteArray requestData);

signals:
    void responseReady(QByteArray responseData);

private:
    DatabaseManager *databaseManager;
    QString connectionName;
    Logger logger;

    void handleLoginRequest(QJsonObject jsonObj);
    void handleCreateAccountRequest(QJsonObject jsonObj);
    void handleGetAccountNumber(QJsonObject jsonObj);
    void handleViewAccountBalanceRequest(QJsonObject jsonObj);
    void handleFetchAllUserDataRequest(QJsonObject &jsonObject);
    void handleDeleteAccountRequest(QJsonObject jsonObj);
    void handleMakeTransactionRequest(QJsonObject jsonObj);
    void handleMakeTransferRequest(QJsonObject jsonObj);
    void handleViewTransactionHistoryRequest(QJsonObject jsonObj);
    void handleUpdateAccountRequest(QJsonObject jsonObj);
};

#endif // REQUESTHANDLER_H
