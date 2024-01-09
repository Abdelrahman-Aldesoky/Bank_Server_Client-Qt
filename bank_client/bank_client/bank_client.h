#ifndef BANK_CLIENT_H
#define BANK_CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDebug>


namespace Ui
{
class bank_client;
}

class bank_client : public QMainWindow
{
    Q_OBJECT

public:
    bank_client(QWidget *parent = nullptr);
    ~bank_client();

private:
    Ui::bank_client *ui;
    QTcpSocket *socket;

    // Regular expressions for username and password validation
    static const QRegularExpression usernameRegex;
    static const QRegularExpression passwordRegex;

    // Function to handle login response
    void handleLoginResponse(const QStringList &responseParts);

public slots:
    void showOldWindow();

private slots:
    void readyRead();
    void on_pushButton_login_clicked();
};

#endif // BANK_CLIENT_H
