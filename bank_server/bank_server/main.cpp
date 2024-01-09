#include <QCoreApplication>
#include "bank_server.h"
#include "databasehandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Create an instance of DatabaseHandler
    DatabaseHandler dbHandler;
    // Initialize the database
    if (dbHandler.initializeDatabase())
    {
        qDebug() << "Database initialized/connected successfully.";
    }
    else
    {
        qDebug() << "Failed to initialize/connect to database.";
        return 1;
    }

    // Create an instance of BankServer
    BankServer server;

    // Start the server
    server.startServer();

    return a.exec();
}
