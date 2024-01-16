#include "RequestHandler.h"

RequestHandler::RequestHandler(const QString &connectionName, QObject *parent)
    : QObject(parent), connectionName(connectionName), logger("RequestHandler")
{
    logger.log("RequestHandler Object Created.");
    databaseManager = new DatabaseManager(connectionName, this);
    databaseManager->openConnection();
    if(databaseManager == nullptr)
    {
        logger.log("Failed to create DatabaseManager.");
        return;
    }
}

RequestHandler::~RequestHandler()
{
    databaseManager->closeConnection();
    if(databaseManager != nullptr)
    {
        delete databaseManager;
        databaseManager = nullptr;
    }
    logger.log("RequestHandler Object Destroyed");
}

QByteArray RequestHandler::handleRequest(QByteArray requestData)
{
    // Convert the received data to a JSON document
    QJsonDocument jsonDoc = QJsonDocument::fromJson(requestData);

    // Convert the JSON document to a JSON object
    QJsonObject jsonObj = jsonDoc.object();

    // Process the request using the DatabaseManager
    QJsonObject responseObj = databaseManager->processRequest(jsonObj);

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array using compact removing white spaces
    QByteArray responseData = jsonResponse.toJson(QJsonDocument::Compact);

    // Compress the response data
    responseData = qCompress(responseData);

    // Return the compressed data
    return responseData;
}
