#include "Server.h"

Server::Server(QObject *parent)
    : QTcpServer(parent), logger("Server")
{
    listen(QHostAddress::LocalHost, 54321);
    logger.log("Listening on Port 54321");
}

Server::~Server()
{
    // Stop and delete all threads
    for (auto it = clientThreads.begin(); it != clientThreads.end(); ++it)
    {
        it.value()->quit();
        it.value()->wait();
        delete it.value();
    }
    close();
    logger.log("All Threads have been closed");
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    if (clientThreads.size() < 4)
    {
        // Create a new thread for each client
        QThread* clientThread = new QThread();
        // Insert the thread into the QMap
        clientThreads.insert(socketDescriptor, clientThread);

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
        // Log the number of connected clients
        logger.log(QString("Number of connected clients: %1").arg(clientThreads.size()));
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
    // Log the thread ID before quitting
    logger.log(QString("Thread with ID: %1 is about to quit.")
                   .arg(reinterpret_cast<quintptr>
                        (clientThreads.value(socketDescriptor)->
                                                   currentThreadId())));
    // Quit the thread
    clientThreads.value(socketDescriptor)->quit();
    // Wait for the thread to finish
    clientThreads.value(socketDescriptor)->wait();
    // Delete the thread
    delete clientThreads.value(socketDescriptor);
    // Remove the thread from the QMap
    clientThreads.remove(socketDescriptor);
    // Log the number of connected clients
    logger.log(QString("Number of connected clients: %1").arg(clientThreads.size()));
    emit clientDisconnected(socketDescriptor);
}
