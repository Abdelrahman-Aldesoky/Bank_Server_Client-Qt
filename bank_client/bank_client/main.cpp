#include <QApplication>
#include <QCoreApplication>
#include "bank_client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    bank_client loginWindow;
    loginWindow.show();
    return a.exec();
}
