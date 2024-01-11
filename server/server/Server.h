#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QThread>

#include "Logger.h"
#include "ClientRunnable.h"

class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);
    ~Server();

signals:
    void clientConnected(qintptr socketDescriptor);
    void clientDisconnected(qintptr socketDescriptor);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void handleClientDisconnected(qintptr socketDescriptor);

private:
    QVector<QThread*> clientThreads;
    Logger logger;
};

#endif // SERVER_H
