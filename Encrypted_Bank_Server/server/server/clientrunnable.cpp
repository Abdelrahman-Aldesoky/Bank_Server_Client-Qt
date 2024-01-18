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
    // Create QSslSocket
    clientSocket = new QSslSocket();

    // Load your local certificate and private key
    clientSocket->setLocalCertificate("server.crt");
    clientSocket->setPrivateKey("server.key");

    if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
        logger.log("Failed to set socket descriptor. Thread will be finished.");
        emit clientDisconnected(socketDescriptor);
        return;
    }

    // Start the SSL handshake.
    clientSocket->startServerEncryption();

    connect(clientSocket, &QSslSocket::encrypted,this,&ClientRunnable::handleEncrypted);
    connect(clientSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &ClientRunnable::handleSslErrors);
    connect(clientSocket, &QSslSocket::readyRead, this, &ClientRunnable::readyRead);
    connect(clientSocket, &QSslSocket::disconnected, this, &ClientRunnable::socketDisconnected);
    logger.log(QString("Client setup completed in thread ID: %1").
               arg((quintptr)QThread::currentThreadId()));
}

void ClientRunnable::handleEncrypted()
{
    logger.log("SSL handshake completed successfully.");
}

void ClientRunnable::readyRead()
{
    QByteArray data = clientSocket->readAll();
    RequestHandler requestHandler(QString::number(socketDescriptor));
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

void ClientRunnable::handleSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        logger.log("SSL error: " + error.errorString());
    }
}

void ClientRunnable::socketDisconnected()
{
    emit clientDisconnected(socketDescriptor);
    logger.log(QString("Client disconnected in thread ID: %1").
               arg((quintptr)QThread::currentThreadId()));
    deleteLater();
}
