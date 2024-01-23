#ifndef CLIENTRUNNABLE_H
#define CLIENTRUNNABLE_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QTimer>

#include "Logger.h"
#include "RequestHandler.h"
#include "DatabaseManager.h"

// Idle time out 30 seconds
#define IDLE_TIMEOUT 30000

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
    void disconnectIdleClient();
    void socketDisconnected();

private:
    qintptr socketDescriptor;
    QTcpSocket *clientSocket = nullptr;
    DatabaseManager* databaseManager = nullptr;
    QTimer *idleTimer = nullptr;
    Logger logger;
};

#endif // CLIENTRUNNABLE_H
