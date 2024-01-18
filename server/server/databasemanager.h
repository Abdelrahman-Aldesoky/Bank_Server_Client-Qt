#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>

#include "Logger.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    DatabaseManager(const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseManager();

    // Functions to manage the database
    bool openConnection();
    void closeConnection();
    QSqlDatabase getDatabase();
    void initializeDatabase();
    bool createTables();

    // Common database operations used in the industry
    bool startDatabaseTransaction();
    bool commitDatabaseTransaction();
    bool rollbackDatabaseTransaction();

    //Crud operations
    QVariant fetchData(const QString &tableName,
                       const QString &fieldName,
                       const QJsonObject &searchCriteria);
    qint64 insertData(const QString &tableName,
                      const QJsonObject &data);
    bool updateData(const QString &tableName,
                    const QJsonObject &data,
                    const QJsonObject &searchCriteria);
    bool removeData(const QString &tableName,
                    const QJsonObject &searchCriteria);

private:
    QString connectionName;
    Logger logger;
};

#endif // DATABASEMANAGER_H
