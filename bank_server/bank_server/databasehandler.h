#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QDir>
#include <QDebug>

class DatabaseHandler
{
public:
    DatabaseHandler();

    bool openDatabaseConnection();
    bool initializeDatabase();
    bool createTables();
    bool login(const QString &username, const QString &password, bool &isAdmin, qint64 &accountNumber);

    void closeDatabaseConnection();

    ~DatabaseHandler();
private:
    QSqlDatabase dbConnection;
    QString dbPath;
    QDir dir;
};

#endif // DATABASEHANDLER_H
