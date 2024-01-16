#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QSslSocket>
#include <QRegularExpression>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDebug>

namespace Ui
{
    class client;
}

class client : public QMainWindow
{
    Q_OBJECT

public:
    client(QWidget *parent = nullptr);
    ~client();

public slots:
    void showOldWindow();
    void readyRead();

private slots:
    void on_pushButton_login_clicked();
    void on_pbn_connect_clicked();
    void handleStateChanged(QAbstractSocket::SocketState socketState);

private:
    Ui::client *ui;
    QSslSocket *socket;

    // Regular expressions for username and password validation
    static const QRegularExpression usernameRegex;
    static const QRegularExpression passwordRegex;

    void connectToServer(void);
    // Function to handle login response
    void handleLoginResponse(const QJsonObject &responseObject);
};

#endif // CLIENT_H
