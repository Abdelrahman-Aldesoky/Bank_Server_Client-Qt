#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(const QString &connectionName, QObject *parent)
    : QObject(parent), connectionName(connectionName), logger("DatabaseManager")
{
    logger.log("DatabaseManager Object Created.");
    QSqlDatabase dbConnection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    dbConnection.setDatabaseName("bankdatabase.db");
}

DatabaseManager::~DatabaseManager()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (dbConnection.isOpen())
    {
        dbConnection.close();
    }
    logger.log("DatabaseManager Object Destroyed.");
}

bool DatabaseManager::openConnection()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.open())
    {
        logger.log(QString("Failed to Open database connection '%1'").arg(connectionName));
        return false;
    }

    logger.log(QString("Opened database connection '%1'").arg(connectionName));
    return true;
}

QSqlDatabase DatabaseManager::getDatabase()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.isOpen())
    {
        logger.log("Database connection is not open.");
    }
    return dbConnection;
}

void DatabaseManager::closeConnection()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (dbConnection.isOpen())
    {
        dbConnection.close();
        logger.log(QString("Closed database connection '%1'").arg(connectionName));
    }
    else
    {
        logger.log(QString("Database connection '%1' is not open.").arg(connectionName));
    }
}

void DatabaseManager::initializeDatabase()
{
    QFile databaseFile("bankdatabase.db");
    if (databaseFile.exists())
    {
        logger.log("bankdatabase already exists.");
    }
    else
    {
        // Create the file if it doesn't exist
        if (databaseFile.open(QIODevice::WriteOnly))
        {
            databaseFile.close();
            logger.log("Created database file: bankdatabase.db");
            openConnection();
            createTables();
            closeConnection();
        }
        else
        {
            logger.log("Failed to create database file!");
        }
    }
}

bool DatabaseManager::createTables()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery createTablesQuery(dbConnection);

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for table creation.");
        return false;
    }

    // Create Accounts table
    const QString prep_accounts =
        "CREATE TABLE Accounts (AccountNumber INTEGER PRIMARY KEY AUTOINCREMENT,"
        " Username TEXT COLLATE NOCASE UNIQUE NOT NULL, Password TEXT NOT NULL,"
        " Admin BOOLEAN);";
    if (!createTablesQuery.exec(prep_accounts))
    {
        logger.log("Failed execution for Accounts table.");
        logger.log("Error: " + createTablesQuery.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Insert default admin account
    QJsonObject defaultAdminData;
    defaultAdminData["Username"] = "admin";
    defaultAdminData["Password"] = "admin";
    defaultAdminData["Admin"] = true;
    if (!insertData("Accounts", defaultAdminData))
    {
        dbConnection.rollback();
        return false;
    }

    // Create Users_Personal_Data table
    const QString prep_users_personal_data =
        "CREATE TABLE Users_Personal_Data (AccountNumber INTEGER PRIMARY KEY, Name TEXT,"
        " Age INTEGER CHECK(Age >= 18 AND Age <= 120), Balance REAL, FOREIGN KEY(AccountNumber)"
        " REFERENCES Accounts(AccountNumber));";
    if (!createTablesQuery.exec(prep_users_personal_data))
    {
        logger.log("Failed execution for Personal Data table.");
        logger.log("Error: " + createTablesQuery.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Create Transaction_History table
    const QString prep_transaction_history =
        "CREATE TABLE Transaction_History (TransactionID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " AccountNumber INTEGER, Date TEXT, Time TEXT, Amount REAL, FOREIGN KEY(AccountNumber)"
        " REFERENCES Accounts(AccountNumber));";
    if (!createTablesQuery.exec(prep_transaction_history))
    {
        logger.log("Failed execution for Transaction history table.");
        logger.log("Error: " + createTablesQuery.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        logger.log("Failed to commit transaction for table creation.");
        dbConnection.rollback();
        return false;
    }

    logger.log("Created all tables successfully.");

    return true;
}

bool DatabaseManager::startDatabaseTransaction()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a database transaction.");
        return false;
    }
    return true;
}

bool DatabaseManager::commitDatabaseTransaction()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.commit())
    {
        logger.log("Failed to commit database transaction.");
        return false;
    }
    return true;
}

bool DatabaseManager::rollbackDatabaseTransaction()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.rollback())
    {
        logger.log("Failed to rollback database transaction.");
        return false;
    }
    return true;
}

QVariant DatabaseManager::fetchData(const QString &tableName,
                                    const QString &fieldName,
                                    const QJsonObject &searchCriteria)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery fetchQuery(dbConnection);

    QStringList criteriaList;
    for (auto it = searchCriteria.begin(); it != searchCriteria.end(); ++it)
    {
        criteriaList.append(QString("%1 = ?").arg(it.key()));
    }
    QString criteriaString = criteriaList.join(" AND ");

    fetchQuery.prepare(QString("SELECT %1 FROM %2 WHERE %3").
                       arg(fieldName, tableName, criteriaString));

    for (auto it = searchCriteria.begin(); it != searchCriteria.end(); ++it)
    {
        fetchQuery.addBindValue(it.value().toVariant());
    }

    if (!fetchQuery.exec())
    {
        logger.log("Failed to fetch data from the database.");
        logger.log("Error: " + fetchQuery.lastError().text());
        return QVariant();
    }

    if (fetchQuery.next())
    {
        return fetchQuery.value(0);
    }

    return QVariant();
}

qint64 DatabaseManager::insertData(const QString &tableName,
                                   const QJsonObject &data)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery insertQuery(dbConnection);

    QStringList keys = data.keys();
    QString fields = keys.join(",");

    // Use a static QRegularExpression object to not load it every single call
    static const QRegularExpression re("[^,]+");
    QString placeholders = keys.join(",").replace(re, "?");

    insertQuery.prepare(QString("INSERT INTO %1 (%2) VALUES (%3)").
                        arg(tableName, fields, placeholders));

    foreach (const QString &key, keys)
    {
        insertQuery.addBindValue(data[key].toVariant());
    }

    if (!insertQuery.exec())
    {
        logger.log("Failed to insert data into the database.");
        logger.log("Error: " + insertQuery.lastError().text());
        return 0;
    }

    return insertQuery.lastInsertId().toLongLong();
}

bool DatabaseManager::updateData(const QString &tableName,
                                 const QJsonObject &data,
                                 const QJsonObject &searchCriteria)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery updateQuery(dbConnection);

    QStringList keys = data.keys();
    QString setStatement = keys.join("=?,") + "=?";

    QStringList criteriaKeys = searchCriteria.keys();
    QStringList criteriaList;
    for (const QString &key : criteriaKeys)
    {
        criteriaList.append(QString("%1 = ?").arg(key));
    }
    QString criteriaString = criteriaList.join(" AND ");

    updateQuery.prepare(QString("UPDATE %1 SET %2 WHERE %3").
                        arg(tableName, setStatement, criteriaString));

    foreach (const QString &key, keys)
    {
        updateQuery.addBindValue(data[key].toVariant());
    }
    foreach (const QString &key, criteriaKeys)
    {
        updateQuery.addBindValue(searchCriteria[key].toVariant());
    }

    if (!updateQuery.exec())
    {
        logger.log("Failed to update data in the database.");
        logger.log("Error: " + updateQuery.lastError().text());
        return false;
    }

    return true;
}

bool DatabaseManager::removeData(const QString &tableName,
                                 const QJsonObject &searchCriteria)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery removeQuery(dbConnection);

    QStringList criteriaKeys = searchCriteria.keys();
    QStringList criteriaList;
    for (const QString &key : criteriaKeys)
    {
        criteriaList.append(QString("%1 = ?").arg(key));
    }
    QString criteriaString = criteriaList.join(" AND ");

    removeQuery.prepare(QString("DELETE FROM %1 WHERE %2").
                        arg(tableName, criteriaString));

    foreach (const QString &key, criteriaKeys)
    {
        removeQuery.addBindValue(searchCriteria[key].toVariant());
    }

    if (!removeQuery.exec())
    {
        logger.log("Failed to remove data from the database.");
        logger.log("Error: " + removeQuery.lastError().text());
        return false;
    }

    return true;
}//315 line
