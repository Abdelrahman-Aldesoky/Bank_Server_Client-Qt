#ifndef CLIENTRUNNABLE_H
#define CLIENTRUNNABLE_H

#include <QObject>
#include <QThread>
#include <QSslSocket>
#include "RequestHandler.h"
#include "Logger.h"

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
    void socketDisconnected();
    void handleSslErrors(const QList<QSslError> &errors);

private:
    qintptr socketDescriptor;
    QSslSocket *clientSocket = nullptr;
    Logger logger;
};

#endif // CLIENTRUNNABLE_H
