#include "bank_server.h"
#include "clientrequesthandler.h"

BankServer::BankServer(QObject *parent) : QTcpServer(parent), client(nullptr), clientRequestHandler(nullptr)
{
    // Create an instance of ClientRequestHandler
    clientRequestHandler = new ClientRequestHandler(this);

    // Connect signals from clientRequestHandler to slots in BankServer
    connect(clientRequestHandler, &ClientRequestHandler::sendResponse, this, &BankServer::sendResponse);
}

void BankServer::startServer()
{
    if(!this->listen(QHostAddress::LocalHost, 54321))
    {
        qDebug() << "Server could not start!";
    }
    else
    {
        qDebug() << "Server started! Connect on localhost and listening on port 54321";
    }
}

void BankServer::incomingConnection(qintptr socketDescriptor)
{
    client = new QTcpSocket(this);
    client->setSocketDescriptor(socketDescriptor);

    qDebug() << "Client connected!"; // Message when a client connects

    connect(client, &QTcpSocket::readyRead, this, &BankServer::readClient);
    connect(client, &QTcpSocket::disconnected, this, &BankServer::disconnected);
}

void BankServer::readClient()
{
    QByteArray data = client->readAll();
    qDebug() << "Received:" << data;

    // Parse the received data based on the delimiter
    QList<QByteArray> parts = data.split('|');

    // Assuming the first part is the request ID
    if (!parts.isEmpty())
    {
        quint8 request_id = parts[0].toInt();

        // Pass the request to ClientRequestHandler
        clientRequestHandler->handleRequest(request_id, parts, client);
    }
}

void BankServer::sendResponse(const QByteArray &response)
{
    // Send the response back to the client
    client->write(response);
}

void BankServer::disconnected()
{
    qDebug() << "Disconnected!";
    client->deleteLater();
}
