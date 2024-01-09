#include "clientrequesthandler.h"

ClientRequestHandler::ClientRequestHandler(QObject *parent) : QObject(parent)
{
    // Initialize any necessary components
}

void ClientRequestHandler::handleRequest(quint8 requestId, const QList<QByteArray> &requestParts, QTcpSocket *client)
{
    switch (requestId)
    {
    case 1:
        handleLoginRequest(requestParts, client);
        break;
    // Add cases for other request types
    default:
        qDebug() << "Unknown request ID:" << requestId;
        // Handle unknown request
        break;
    }
}

void ClientRequestHandler::handleLoginRequest(const QList<QByteArray> &requestParts, QTcpSocket *client)
{
    QByteArray response;
    if (requestParts.size() == 3)
    {
        QString username = requestParts[1];
        QString password = requestParts[2];

        bool isAdmin = false;
        qint64 accountNumber = 0;

        bool loginSuccess = databaseHandler.login(username, password, isAdmin, accountNumber);

        if (loginSuccess)
        {
            // Include the accountNumber and isAdmin status in the response
            response = QString("1|%1|%2").arg(accountNumber).arg(isAdmin).toUtf8();
        }
        else
        {
            // Send a failure response to the client
            response = "0|Login failed";
        }

        // Send the response back to the client
        emit sendResponse(response, client);
        qDebug() <<"Response: " <<response;
    }
    else
    {
        response = "-1|login format failure.";
        emit sendResponse(response, client);
        qDebug() <<"Response: " <<response;
    }
}
