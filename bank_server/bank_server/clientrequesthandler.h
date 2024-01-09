#ifndef CLIENTREQUESTHANDLER_H
#define CLIENTREQUESTHANDLER_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QTcpSocket>

#include "databasehandler.h"

class ClientRequestHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientRequestHandler(QObject *parent = nullptr);

public slots:
    void handleRequest(quint8 requestId, const QList<QByteArray> &requestParts, QTcpSocket *client);

signals:
    void sendResponse(const QByteArray &response, QTcpSocket *client);

private:
    DatabaseHandler databaseHandler;

    void handleLoginRequest(const QList<QByteArray> &requestParts, QTcpSocket *client);

};

#endif // CLIENTREQUESTHANDLER_H
