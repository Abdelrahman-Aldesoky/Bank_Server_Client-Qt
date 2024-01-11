#include "Server.h"

Server::Server(QObject *parent)
    : QTcpServer(parent), logger("Server")
{
    listen(QHostAddress::LocalHost, 54321);
    logger.log("Listening on Port 54321");
}

Server::~Server()
{
    // Stop and delete all client threads
    for (QThread* thread : clientThreads)
    {
        thread->quit();
        thread->wait();
        delete thread;
    }
    close();

    logger.log("All Threads have been closed");
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    if (clientThreads.size() < 1000)
    {
        // Create a new thread for each client
        QThread* clientThread = new QThread();
        clientThreads.append(clientThread);

        // Move the clientRunnable to the new thread
        ClientRunnable* clientRunnable = new ClientRunnable(socketDescriptor);
        clientRunnable->moveToThread(clientThread);

        // Connect signals for thread management
        connect(clientThread, &QThread::started, clientRunnable, &ClientRunnable::run);
        connect(clientRunnable, &ClientRunnable::clientDisconnected, this, &Server::handleClientDisconnected);
        connect(clientThread, &QThread::finished, clientRunnable, &ClientRunnable::deleteLater);
        connect(clientRunnable, &ClientRunnable::destroyed, clientThread, &QThread::quit);

        clientThread->start();
        logger.log(QString("Client connected with socket descriptor: %1").arg(socketDescriptor));
        emit clientConnected(socketDescriptor);
    }
    else
    {
        logger.log("Maximum number of clients reached. Rejecting connection.");

        QTcpSocket rejectedSocket;
        rejectedSocket.setSocketDescriptor(socketDescriptor);
        rejectedSocket.close();
    }
}

void Server::handleClientDisconnected(qintptr socketDescriptor)
{
    logger.log(QString("Client disconnected with socket descriptor: %1").arg(socketDescriptor));
    emit clientDisconnected(socketDescriptor);
}
