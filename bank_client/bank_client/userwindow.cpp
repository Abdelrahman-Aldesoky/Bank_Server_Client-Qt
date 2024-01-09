#include "userwindow.h"
#include "ui_userwindow.h"

UserWindow::UserWindow(QWidget *parent, qint64 accountNumber)
    : QMainWindow(parent), ui(new Ui::UserWindow), accountNumber(accountNumber)
{
    ui->setupUi(this);
    setWindowTitle("User Window - Account Number: " + QString::number(accountNumber));

    // Set the Qt::WA_DeleteOnClose attribute to ensure the destructor is called on close
    setAttribute(Qt::WA_DeleteOnClose);
    qDebug() <<"Constructed User Window.";
}

UserWindow::~UserWindow()
{
    delete ui;
    // Emit the finished signal when the window is closed
    emit finished();
    qDebug() <<"Destroyed User Window";
}
