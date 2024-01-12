#include "ClientRunnable.h"

ClientRunnable::ClientRunnable(qintptr socketDescriptor, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor),
    clientSocket(nullptr), logger("ClientRunnable")
{
    databaseManager = new DatabaseManager(this);
}

ClientRunnable::~ClientRunnable()
{
    if (clientSocket)
    {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
    if(databaseManager)
    {
        databaseManager->deleteLater();
        databaseManager = nullptr;
    }
}

void ClientRunnable::run()
{
    clientSocket = new QTcpSocket();

    if (!clientSocket->setSocketDescriptor(socketDescriptor))
    {
        emit clientDisconnected(socketDescriptor);
        logger.log("Failed to set socket descriptor. Thread will be finished.");
        return;
    }

    emit clientConnected(socketDescriptor);

    QString connectionName = QString::number(socketDescriptor);
    if (!databaseManager->openConnection(connectionName))
    {
        logger.log("Failed to open database connection.");
        return;
    }

    connect(clientSocket, &QTcpSocket::readyRead, this, &ClientRunnable::readyRead, Qt::DirectConnection);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientRunnable::socketDisconnected, Qt::DirectConnection);

    // Log the thread ID when it's started
    logger.log("Thread with ID: " +
               QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())) +
               " has started.");

    logger.log("Client setup completed.");
}

void ClientRunnable::readyRead()
{
    QByteArray data = clientSocket->readAll();
    logger.log("Received data from client: " + QString(data));

    QString connectionName = QString::number(socketDescriptor);
    RequestHandler *requestHandler = new RequestHandler(databaseManager,connectionName, this);

    connect(requestHandler, &RequestHandler::responseReady, this, &ClientRunnable::sendResponseToClient);
    requestHandler->handleDifferentClientRequests(data);
}


void ClientRunnable::sendResponseToClient(QByteArray responseData)
{
    // Send the response data back to the client
    clientSocket->write(responseData);
    logger.log("Sent data to Client: " + QString(responseData));
}

void ClientRunnable::socketDisconnected()
{
    QString connectionName = QString::number(socketDescriptor);
    databaseManager->closeConnection(connectionName);
    emit clientDisconnected(socketDescriptor);

    // Log the thread ID before quitting
    logger.log("Thread with ID: " +
               QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())) +
               " is about to quit.");

    quit();
    deleteLater();
    logger.log("Client disconnected.");
}
