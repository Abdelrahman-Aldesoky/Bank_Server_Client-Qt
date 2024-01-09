#include "adminwindow.h"
#include "ui_adminwindow.h"

AdminWindow::AdminWindow(QWidget *parent, qint64 accountNumber)
    : QMainWindow(parent), ui(new Ui::AdminWindow), accountNumber(accountNumber)
{
    ui->setupUi(this);
    setWindowTitle("Admin Window - Account Number: " + QString::number(accountNumber));

    // Set the Qt::WA_DeleteOnClose attribute to ensure the destructor is called on close
    setAttribute(Qt::WA_DeleteOnClose);
    qDebug() <<"Constructed Admin Window.";
}

AdminWindow::~AdminWindow()
{
    delete ui;
    // Emit the finished signal when the window is closed
    emit finished();
    qDebug() <<"Destroyed Admin Window";
}

