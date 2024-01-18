#ifndef CLIENTRUNNABLE_H
#define CLIENTRUNNABLE_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

#include "Logger.h"
#include "RequestHandler.h"
#include "DatabaseManager.h"

class ClientRunnable : public QObject
{
    Q_OBJECT

public:
    ClientRunnable(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientRunnable();

public slots:
    void run();
    void readyRead();
    void sendResponseToClient(QByteArray responseData);

signals:
    void clientDisconnected(qintptr socketDescriptor);

private slots:
    void socketDisconnected();

private:
    qintptr socketDescriptor;
    QTcpSocket *clientSocket = nullptr;
    DatabaseManager* databaseManager = nullptr;
    Logger logger;
};

#endif // CLIENTRUNNABLE_H
