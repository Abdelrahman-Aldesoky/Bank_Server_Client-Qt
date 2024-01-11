#ifndef CLIENTRUNNABLE_H
#define CLIENTRUNNABLE_H

#include <QCoreApplication>
#include <QObject>
#include <QTcpSocket>
#include <QThread>

#include "databasemanager.h"
#include "requesthandler.h"
#include "Logger.h"

class ClientRunnable : public QThread
{
    Q_OBJECT

public:
    ClientRunnable(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientRunnable() override;
    void run() override;
    void readyRead();
    void sendResponseToClient(QByteArray responseData);

signals:
    void clientConnected(qintptr socketDescriptor);
    void clientDisconnected(qintptr socketDescriptor);

private slots:
    void socketDisconnected();

private:
    qintptr socketDescriptor;
    QTcpSocket *clientSocket;
    DatabaseManager *databaseManager;
    Logger logger;
};

#endif // CLIENTRUNNABLE_H
