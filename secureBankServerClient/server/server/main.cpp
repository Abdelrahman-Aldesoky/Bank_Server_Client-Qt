#include <QCoreApplication>
#include <signal.h>
#include <QTimer>

#include "databasemanager.h"
#include "backupmanager.h"
#include "Server.h"
#include "Logger.h"

void handleSignal(int signal);
void initializeDatabase();

int main(int argc, char *argv[])
{
    QCoreApplication bankServer(argc, argv);

    // Handle SIGINT, SIGTERM, and SIGHUP
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    //it's annoying in windows to implement this...
    //signal(SIGHUP, handleSignal);

    initializeDatabase();

    DatabaseManager databaseManager("DatabaseBackupConnection");
    BackupManager backupManager(&databaseManager);

    // Create a QTimer object
    QTimer *timer = new QTimer(&bankServer);
    // Connect the timeout signal to the createFullBackup slot
    QObject::connect(timer, &QTimer::timeout, &backupManager,
                     &BackupManager::createFullBackup);
    // Start the timer to trigger every 6 hours for periodic backups
    timer->start(6 * 60 * 60 * 1000);

    // Connect the aboutToQuit signal to the handleShutdown slot
    QObject::connect(&bankServer, &QCoreApplication::aboutToQuit, &backupManager,
                     &BackupManager::handleShutdown);

    Server server(&bankServer);

    Logger mainLogger("Main");

    if (!server.isListening())
    {
        mainLogger.log("Failed to start the server.");
        return 1;
    }

    mainLogger.log("Event loop Started.");

    bankServer.processEvents();
    return bankServer.exec();
}

void initializeDatabase()
{
    DatabaseManager databaseManager("DatabaseInitializationConnection");
    databaseManager.initializeDatabase();
}

void handleSignal(int signal)
{
    Q_UNUSED(signal);
    Logger signalLogger("ExitSignal");

    signalLogger.log("Received exit signal. Initiating server shutdown.");
    QCoreApplication::quit();
}
