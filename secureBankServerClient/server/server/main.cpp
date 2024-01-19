#include <QCoreApplication>
#include <signal.h>
#include "databasemanager.h"
#include "Server.h"
#include "Logger.h"

void handleSignal(int signal);
void initializeDatabase(void);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    signal(SIGINT, handleSignal);

    
    // Initialize the database
    initializeDatabase();

    // Create the Server object
    Server server(&a);

    Logger mainLogger("Main");

    if (!server.isListening())
    {
        mainLogger.log("Failed to start the server.");
        return 1;
    }

    mainLogger.log("Event loop Started.");

    a.processEvents();
    return a.exec();
}

void initializeDatabase(void)
{
    DatabaseManager databaseManager("InitializeDatabase");
    databaseManager.initializeDatabase();
}


void handleSignal(int signal)
{
    Q_UNUSED(signal);
    Logger signalLogger("ExitSignal");
    signalLogger.log("Received exit signal. Initiating server shutdown.");
    signalLogger.log("Event Loop Ended.");
    QCoreApplication::quit();
}
