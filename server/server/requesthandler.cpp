#include "RequestHandler.h"
#include "DatabaseManager.h"

RequestHandler::RequestHandler(const QString &connectionName, QObject *parent)
    : QObject(parent), connectionName(connectionName), logger("RequestHandler")
{}

QByteArray RequestHandler::handleRequest(QByteArray requestData)
{
    logger.log("Processing Request: " + QString(requestData));

    // Convert the received data to a JSON document
    QJsonDocument jsonDoc = QJsonDocument::fromJson(requestData);

    // Convert the JSON document to a JSON object
    QJsonObject jsonObj = jsonDoc.object();

    // Create a new DatabaseManager object for each request
    DatabaseManager databaseManager;

    // Open a new connection for each request
    if (!databaseManager.openConnection(connectionName)) {
        logger.log("Failed to open database connection.");
        return QByteArray();
    }

    // Process the request using the DatabaseManager
    QJsonObject responseObj = databaseManager.processRequest(jsonObj, connectionName);

    // Close the connection after the request has been processed
    databaseManager.closeConnection(connectionName);

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Return the response data
    return responseData;
}
