#ifndef CLIENTRUNNABLE_H
#define CLIENTRUNNABLE_H

#include <QObject>
#include <QThread>
#include <QSslSocket>
#include <QTimer>

#include "RequestHandler.h"
#include "DatabaseManager.h"
#include "logger.h"

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
    void handleEncrypted();
    void disconnectIdleClient();
    void socketDisconnected();
    void handleSslErrors(const QList<QSslError> &errors);

private:
    qintptr socketDescriptor;
    QSslSocket *clientSocket = nullptr;
    DatabaseManager* databaseManager = nullptr;
    QTimer *idleTimer = nullptr;
    Logger logger;
};

#endif // CLIENTRUNNABLE_H
