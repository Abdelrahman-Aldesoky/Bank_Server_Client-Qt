#include <QApplication>
#include <QCoreApplication>
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    client loginWindow;
    loginWindow.show();
    return a.exec();
}
