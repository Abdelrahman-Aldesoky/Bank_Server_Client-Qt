#include "RequestHandler.h"

RequestHandler::RequestHandler(DatabaseManager* databaseManager, QObject *parent)
    : QObject(parent), databaseManager(databaseManager), logger("RequestHandler")
{
    logger.log("RequestHandler Object Created.");
    accountManager = new AccountManager(databaseManager, this);
    if(accountManager == nullptr)
    {
        logger.log("Failed to create AccountManager.");
        return;
    }
    transactionManager = new TransactionManager(databaseManager, this);
    if(transactionManager == nullptr)
    {
        logger.log("Failed to create TransactionManager.");
        return;
    }
}

RequestHandler::~RequestHandler()
{
    if(transactionManager != nullptr)
    {
        delete transactionManager;
        transactionManager = nullptr;
    }
    if(accountManager != nullptr)
    {
        delete accountManager;
        accountManager = nullptr;
    }
    logger.log("RequestHandler Object Destroyed");
}

QByteArray RequestHandler::handleRequest(QByteArray requestData)
{
    QMutexLocker locker(&mutex);
    // Convert the received data to a JSON document
    QJsonDocument jsonDoc = QJsonDocument::fromJson(requestData);

    // Convert the JSON document to a JSON object
    QJsonObject requestJson  = jsonDoc.object();

    // Extract the request ID from the request JSON
    qint16 requestId = requestJson ["requestId"].toInt();

    // Add the response ID to the response JSON
    QJsonObject responseJson;

    // Process the request based on the request ID
    switch (requestId)
    {
    case 0:
        responseJson = accountManager->login(requestJson);
        break;
    case 1:
        responseJson = accountManager->getAccountNumber(requestJson);
        break;
    case 2:
        responseJson = accountManager->getAccountBalance(requestJson);
        break;
    case 3:
        responseJson = accountManager->createNewAccount(requestJson);
        break;
    case 4:
        responseJson = accountManager->deleteAccount(requestJson);
        break;
    case 5:
        responseJson = accountManager->viewDatabase();
        break;
    case 6:
        responseJson = transactionManager->makeTransaction(requestJson);
        break;
    case 7:
        responseJson = transactionManager->makeTransfer(requestJson);
        break;
    case 8:
        responseJson = transactionManager->viewTransactionHistory(requestJson);
        break;
    case 9:
        responseJson = accountManager->updateUserData(requestJson);
        break;
    default:
        // Handle unknown request
        logger.log("Unknown request");
        break;
    }

    responseJson["responseId"] = requestId;

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseJson);

    // Convert the JSON document to a byte array using compact removing white spaces
    QByteArray responseData = jsonResponse.toJson(QJsonDocument::Compact);

    // Compress the response data
    responseData = qCompress(responseData);

    // Return the compressed data
    return responseData;
}
