#include "ClientRunnable.h"

ClientRunnable::ClientRunnable(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor), logger("ClientRunnable")
{
    logger.log("Object Created.");
}

ClientRunnable::~ClientRunnable()
{
    if(databaseManager != nullptr)
    {
        databaseManager->closeConnection();
        delete databaseManager;
        QSqlDatabase::removeDatabase(QString::number(socketDescriptor));
        databaseManager = nullptr;
    }
    logger.log("Object Destroyed.");
}

void ClientRunnable::run()
{
    // Create QTcpSocket
    clientSocket = new QTcpSocket();

    if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
        logger.log("Failed to set socket descriptor. Thread will be finished.");
        emit clientDisconnected(socketDescriptor);
        return;
    }

    // Create databaseManager object for the client connected
    databaseManager = new DatabaseManager(QString::number(socketDescriptor), this);
    if(databaseManager == nullptr)
    {
        logger.log("Failed to create DatabaseManager.");
        return;
    }
    databaseManager->openConnection();

    connect(clientSocket, &QTcpSocket::readyRead, this, &ClientRunnable::readyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientRunnable::socketDisconnected);
    logger.log(QString("Client setup completed in thread ID: %1").
               arg((quintptr)QThread::currentThreadId()));
}

void ClientRunnable::readyRead()
{
    QByteArray data = clientSocket->readAll();
    RequestHandler requestHandler(databaseManager, this);
    QByteArray responseData = requestHandler.handleRequest(data);
    sendResponseToClient(responseData);
}

void ClientRunnable::sendResponseToClient(QByteArray responseData)
{
    if (clientSocket->write(responseData) == -1)
    {
        logger.log("Failed to write data to client: " + clientSocket->errorString());
    }
}

void ClientRunnable::socketDisconnected()
{
    emit clientDisconnected(socketDescriptor);
    logger.log(QString("Client disconnected in thread ID: %1").
               arg((quintptr)QThread::currentThreadId()));
    deleteLater();
}
