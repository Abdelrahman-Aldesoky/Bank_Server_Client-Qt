#include "ClientRunnable.h"

ClientRunnable::ClientRunnable(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor), logger("ClientRunnable")
{
    if (socketDescriptor == -1)
    {
        logger.log("Invalid socket descriptor.");
        return;
    }

    idleTimer = new QTimer(this);
    connect(idleTimer, &QTimer::timeout, this, &ClientRunnable::disconnectIdleClient);
    idleTimer->start(IDLE_TIMEOUT);

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

    // Check if the database connection was removed successfully
    if(QSqlDatabase::contains(QString::number(socketDescriptor)))
    {
        logger.log("Failed to remove database connection.");
    }
    else
    {
        logger.log("Database connection removed successfully.");
    }

    // Delete the QTimer object
    if(idleTimer != nullptr)
    {
        if(idleTimer->isActive())
        {
            idleTimer->stop();
        }
        delete idleTimer;
        idleTimer = nullptr;
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
    // Reset idle timer on every message received
    idleTimer->start(IDLE_TIMEOUT);

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

void ClientRunnable::disconnectIdleClient()
{
    logger.log("Client idle. Disconnecting...");
    clientSocket->disconnectFromHost();
}

void ClientRunnable::socketDisconnected()
{
    emit clientDisconnected(socketDescriptor);
    logger.log(QString("Client disconnected in thread ID: %1").
               arg((quintptr)QThread::currentThreadId()));
    deleteLater();
}
