#include "RequestHandler.h"

RequestHandler::RequestHandler(DatabaseManager *databaseManager,
                               const QString &connectionName,
                               QObject *parent)
    : QObject(parent), databaseManager(databaseManager),
    connectionName(connectionName), logger("RequestHandler")
{}

void RequestHandler::handleDifferentClientRequests(QByteArray requestData)
{
    logger.log("Proccesing Request: " + QString(requestData));
    // Convert the received data to a JSON document
    QJsonDocument jsonDoc = QJsonDocument::fromJson(requestData);

    // Convert the JSON document to a JSON object
    QJsonObject jsonObj = jsonDoc.object();

    // Extract the request ID from the JSON object
    int requestId = jsonObj["requestId"].toInt();

    switch(requestId)
    {
    case 0:
        handleLoginRequest(jsonObj);
        break;
    case 1:
        handleGetAccountNumber(jsonObj);
        break;
    case 2:
        handleViewAccountBalanceRequest(jsonObj);
        break;
    case 3:
        handleCreateAccountRequest(jsonObj);
        break;
    case 4:
        handleDeleteAccountRequest(jsonObj);
        break;
    case 5:
        handleFetchAllUserDataRequest(jsonObj);
        break;
    case 6:
        handleMakeTransactionRequest(jsonObj);
        break;
    case 7:
        handleMakeTransferRequest(jsonObj);
        break;
    case 8:
        handleViewTransactionHistoryRequest(jsonObj);
        break;
    default:
        // Handle unknown request
        logger.log("Unknown request");
        break;
    }
}

void RequestHandler::handleLoginRequest(QJsonObject jsonObj)
{
    // Extract the username and password from the request
    QString username = jsonObj["username"].toString();
    QString password = jsonObj["password"].toString();

    // Create a DatabaseManager object and call the login method
    DatabaseManager databaseManager;
    QJsonObject responseObj = databaseManager.login(username, password, connectionName);

    // Add the responseId to the response object
    responseObj["responseId"] = 0;

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal
    emit responseReady(responseData);
}

void RequestHandler::handleGetAccountNumber(QJsonObject jsonObj)
{
    QString username = jsonObj["username"].toString();
    QJsonObject responseObj = databaseManager->getAccountNumber(username, connectionName);
    responseObj["responseId"] = 1;
    QJsonDocument jsonResponse(responseObj);
    QByteArray responseData = jsonResponse.toJson();
    emit responseReady(responseData);
}

void RequestHandler::handleViewAccountBalanceRequest(QJsonObject jsonObj)
{
    qint64 accountNumber = jsonObj["accountNumber"].toVariant().toLongLong();

    // Create a DatabaseManager object and call the getAccountBalance method
    double accountBalance = databaseManager->getAccountBalance(accountNumber, connectionName);

    // Create the response object and set "responseId" to 2
    QJsonObject responseObj;
    responseObj["responseId"] = 2;

    if (accountBalance >= 0.0)
    {
        // View Account Balance successful
        responseObj["viewBalanceSuccess"] = true;
        responseObj["accountBalance"] = accountBalance;
    }
    else
    {
        // View Account Balance failed
        responseObj["viewBalanceSuccess"] = false;
    }

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal
    emit responseReady(responseData);
}

void RequestHandler::handleCreateAccountRequest(QJsonObject jsonObj)
{
    // Call the createNewAccount method in the DatabaseManager
    QJsonObject responseObj = databaseManager->createNewAccount(jsonObj, connectionName);

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal to send the response back to the client
    emit responseReady(responseData);
}

void RequestHandler::handleDeleteAccountRequest(QJsonObject jsonObj)
{
    qint64 accountNumber = jsonObj["accountNumber"].toVariant().toLongLong();

    // Call the deleteAccount method in the DatabaseManager
    bool deleteAccountSuccess = databaseManager->deleteAccount(accountNumber, connectionName);

    // Create the response object and set "responseId" to the appropriate value
    QJsonObject responseObj;
    responseObj["responseId"] = 4;

    responseObj["deleteAccountSuccess"] = deleteAccountSuccess;

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal to send the response back to the client
    emit responseReady(responseData);
}

void RequestHandler::handleFetchAllUserDataRequest(QJsonObject &jsonObject)
{
    // Call the appropriate method in DatabaseManager to fetch all user data
    QJsonObject responseObj = databaseManager->fetchAllUserData(connectionName);

    // Add the requestId from the original request to the response object
    responseObj["responseId"] = jsonObject["requestId"];

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal
    emit responseReady(responseData);
}

void RequestHandler::handleMakeTransactionRequest(QJsonObject jsonObj)
{
    logger.log("Handling Make Transaction Request: " + QJsonDocument(jsonObj).toJson());

    // Add more logging statements here to trace the flow of execution

    // Example: Log before and after calling a database operation
    logger.log("Before calling database method");
    // Call the appropriate method in DatabaseManager
    QJsonObject responseObj = databaseManager->makeTransaction(jsonObj, connectionName);
    logger.log("After calling database method");

    // Add more logging statements here

    // Add the requestId from the original request to the response object
    responseObj["responseId"] = jsonObj["requestId"];

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal
    emit responseReady(responseData);
}

void RequestHandler::handleMakeTransferRequest(QJsonObject jsonObj)
{
    logger.log("Handling Make Transfer Request: " + QJsonDocument(jsonObj).toJson());

    // Extract transfer details from the JSON object
    qint64 fromAccountNumber = jsonObj["fromAccountNumber"].toVariant().toLongLong();
    qint64 toAccountNumber = jsonObj["toAccountNumber"].toVariant().toLongLong();
    double amount = jsonObj["amount"].toDouble();

    // Perform the transfer logic
    QJsonObject responseObj = databaseManager->makeTransfer(fromAccountNumber, toAccountNumber, amount, connectionName);

    // Add the requestId from the original request to the response object
    responseObj["responseId"] = jsonObj["requestId"];

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal
    emit responseReady(responseData);
}

void RequestHandler::handleViewTransactionHistoryRequest(QJsonObject jsonObj)
{
    // Extract the account number from the request
    qint64 accountNumber = jsonObj["accountNumber"].toVariant().toLongLong();

    // Call the viewTransactionHistory method in the DatabaseManager
    QJsonArray transactionHistoryArray = databaseManager->viewTransactionHistory(accountNumber, connectionName);

    // Create the response object and set "responseId" to the appropriate value
    QJsonObject responseObj;
    responseObj["responseId"] = 8;

    responseObj["viewTransactionHistorySuccess"] = true;
    responseObj["transactionHistory"] = transactionHistoryArray;

    // Convert the response object to a JSON document
    QJsonDocument jsonResponse(responseObj);

    // Convert the JSON document to a byte array
    QByteArray responseData = jsonResponse.toJson();

    // Emit the responseReady signal to send the response back to the client
    emit responseReady(responseData);
}
