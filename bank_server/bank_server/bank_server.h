#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QDebug>
#include "clientrequesthandler.h"

class BankServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit BankServer(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void readClient();
    void sendResponse(const QByteArray &response);
    void disconnected();

private:
    QTcpSocket *client;
    ClientRequestHandler *clientRequestHandler;
};

#endif // BANK_SERVER_H
