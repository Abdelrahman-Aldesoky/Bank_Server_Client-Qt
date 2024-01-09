#include "databasehandler.h"

DatabaseHandler::DatabaseHandler() : dbPath("./database/bank.db"), dir("./database")
{
    // Create the directory if it doesn't exist
    if (!dir.exists())
    {
        dir.mkpath(".");
        qDebug() << "Created database directory.";
    }
}

bool DatabaseHandler::openDatabaseConnection()
{
    dbConnection = QSqlDatabase::addDatabase("QSQLITE");
    dbConnection.setDatabaseName(dbPath);
    if (dbConnection.open())
    {
        qDebug() << "Opened database connection successfully";
        return true;
    }

    qDebug() << "Failed to open database connection.";
    return false;
}

void DatabaseHandler::closeDatabaseConnection()
{
    if (dbConnection.isOpen())
    {
        dbConnection.close();
        qDebug() << "Closed database connection.";
    }
}

bool DatabaseHandler::initializeDatabase()
{
    bool databaseInitialized = false;

    // If the database file does not exist, create it and tables
    if (!QFile(dbPath).exists())
    {
        // Try opening the database connection
        databaseInitialized = openDatabaseConnection();
        // If the connection is successful, create tables in the database
        if (databaseInitialized)
        {
            databaseInitialized = createTables();
        }
    }
    else
    {
        // If the database file already exists, connect to it
        qDebug() << "Database file exists.";
        databaseInitialized = openDatabaseConnection();
    }

    return databaseInitialized;
}

bool DatabaseHandler::createTables()
{
    QSqlQuery qry(dbConnection);

    // Begin a transaction
    dbConnection.transaction();

    // Create Accounts table with an additional Admin column
    const QString prep_accounts = "CREATE TABLE Accounts ("
                                  "AccountNumber INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "Username TEXT COLLATE NOCASE UNIQUE NOT NULL,"
                                  "Password TEXT NOT NULL,"
                                  "Admin BOOLEAN);";

    if (!qry.exec(prep_accounts))
    {
        qDebug() << "Failed execution for Accounts table.";
        qDebug() << "Error" << dbConnection.lastError();

        // Rollback the transaction if an error occurs
        dbConnection.rollback();
        return false;
    }

    // Insert default admin account
    const QString insert_default_admin = "INSERT INTO Accounts (Username, Password, Admin) VALUES ('admin', 'admin', 1);";

    if (!qry.exec(insert_default_admin))
    {
        qDebug() << "Failed to insert default admin account.";
        qDebug() << "Error" << dbConnection.lastError();

        // Rollback the transaction if an error occurs
        dbConnection.rollback();
        return false;
    }

    // Create Users_Personal_Data table
    const QString prep_users_personal_data = "CREATE TABLE Users_Personal_Data ("
                                             "AccountNumber INTEGER PRIMARY KEY,"
                                             "Name TEXT,"
                                             "Age INTEGER CHECK(Age >= 18 AND Age <= 120),"
                                             "Balance REAL,"
                                             "FOREIGN KEY(AccountNumber) REFERENCES Accounts(AccountNumber));";

    if (!qry.exec(prep_users_personal_data))
    {
        qDebug() << "Failed execution for Personal Data table.";
        qDebug() << "Error" << dbConnection.lastError();

        // Rollback the transaction if an error occurs
        dbConnection.rollback();
        return false;
    }

    // Create Transaction_History table
    const QString prep_transaction_history = "CREATE TABLE Transaction_History ("
                                             "TransactionID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                             "AccountNumber INTEGER,"
                                             "Date TEXT,"
                                             "Time TEXT,"
                                             "Amount REAL,"
                                             "FOREIGN KEY(AccountNumber) REFERENCES Accounts(AccountNumber));";

    if (!qry.exec(prep_transaction_history))
    {
        qDebug() << "Failed execution for Transaction history table.";
        qDebug() << "Error" << dbConnection.lastError();

        // Rollback the transaction if an error occurs
        dbConnection.rollback();
        return false;
    }

    // Commit the transaction if all queries are successful
    dbConnection.commit();

    qDebug() << "Created all tables successfully.";
    return true;
}

bool DatabaseHandler::login(const QString &username, const QString &password,
                            bool &isAdmin, qint64 &accountNumber)
{
    QSqlQuery qry(dbConnection);

    // Check if the username and password match an account
    qry.prepare("SELECT AccountNumber, Admin FROM Accounts "
                "WHERE Username = :username AND Password = :password;");
    qry.bindValue(":username", username);
    qry.bindValue(":password", password);

    if (qry.exec() && qry.next())
    {
        // Login successful
        accountNumber = qry.value("AccountNumber").toLongLong();
        isAdmin = qry.value("Admin").toBool();
        return true;
    }

    // Login failed
    return false;
}

DatabaseHandler::~DatabaseHandler()
{
    closeDatabaseConnection();
}
