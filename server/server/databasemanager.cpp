#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent), logger("DatabaseManager")
{}

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
            createTables("CreateTables");
        }
        else
        {
            logger.log("Failed to create database file!");
        }
    }
}

DatabaseManager::~DatabaseManager()
{
    logger.log("DatabaseManager object destroyed.");
}

bool DatabaseManager::openConnection(const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    dbConnection.setDatabaseName("bankdatabase.db");

    if (!dbConnection.open())
    {
        logger.log(QString("Failed to Open database connection '%1'").arg(connectionName));
        return false;
    }

    logger.log(QString("Opened database connection '%1'").arg(connectionName));
    return true;
}

void DatabaseManager::closeConnection(const QString &connectionName)
{
    QMutexLocker locker(&mutex);
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

bool DatabaseManager::createTables(const QString &connectionName)
{
    openConnection(connectionName);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery qry(dbConnection);

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
    if (!qry.exec(prep_accounts))
    {
        logger.log("Failed execution for Accounts table.");
        logger.log("Error: " + qry.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Insert default admin account
    const QString insert_default_admin =
        "INSERT INTO Accounts (Username, Password, Admin) "
        "VALUES ('admin', 'admin', 1);";
    if (!qry.exec(insert_default_admin))
    {
        logger.log("Failed to insert default admin account.");
        logger.log("Error: " + qry.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Create Users_Personal_Data table
    const QString prep_users_personal_data =
        "CREATE TABLE Users_Personal_Data (AccountNumber INTEGER PRIMARY KEY, Name TEXT,"
        " Age INTEGER CHECK(Age >= 18 AND Age <= 120), Balance REAL, FOREIGN KEY(AccountNumber)"
        " REFERENCES Accounts(AccountNumber));";
    if (!qry.exec(prep_users_personal_data))
    {
        logger.log("Failed execution for Personal Data table.");
        logger.log("Error: " + qry.lastError().text());
        dbConnection.rollback();
        return false;
    }

    // Create Transaction_History table
    const QString prep_transaction_history =
        "CREATE TABLE Transaction_History (TransactionID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " AccountNumber INTEGER, Date TEXT, Time TEXT, Amount REAL, FOREIGN KEY(AccountNumber)"
        " REFERENCES Accounts(AccountNumber));";
    if (!qry.exec(prep_transaction_history))
    {
        logger.log("Failed execution for Transaction history table.");
        logger.log("Error: " + qry.lastError().text());
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

    qry.finish();
    logger.log("Created all tables successfully.");
    closeConnection(connectionName);

    return true;
}

QJsonObject DatabaseManager::login(const QString &username, const QString &password, const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery query(dbConnection);

    // Create the response object and set "responseId" to 1
    QJsonObject responseObj;
    responseObj["responseId"] = 1;

    query.prepare("SELECT AccountNumber, Admin FROM Accounts "
                  "WHERE Username = :username AND Password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    if (!query.exec())
    {
        logger.log("Failed to execute query for login request.");
        return responseObj;
    }

    if (query.next())
    {
        // Login successful
        responseObj["loginSuccess"] = true;
        responseObj["accountNumber"] = query.value("AccountNumber").toLongLong();
        responseObj["isAdmin"] = query.value("Admin").toBool();
        return responseObj;
    }
    else
    {
        // Login failed
        responseObj["loginSuccess"] = false;
        return responseObj;
    }
}

QJsonObject DatabaseManager::getAccountNumber(const QString &username, const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery query(dbConnection);

    QJsonObject responseObj;

    query.prepare("SELECT AccountNumber FROM Accounts WHERE Username = :username");
    query.bindValue(":username", username);
    if (!query.exec())
    {
        logger.log("Failed to execute query for getAccountNumber request.");
        responseObj["userFound"] = false;
        return responseObj;
    }

    if (query.next())
    {
        responseObj["accountNumber"] = query.value("AccountNumber").toLongLong();
        responseObj["userFound"] = true;
        return responseObj;
    }
    else
    {
        responseObj["userFound"] = false;
        return responseObj;
    }
}

double DatabaseManager::getAccountBalance(qint64 accountNumber, const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery query(dbConnection);

    query.prepare("SELECT Balance FROM Users_Personal_Data WHERE AccountNumber = :accountNumber");
    query.bindValue(":accountNumber", accountNumber);

    if (!query.exec())
    {
        logger.log("Failed to execute query for getAccountBalance request.");
        return -1.0; // Return a negative value to indicate an error
    }

    if (query.next())
    {
        return query.value("Balance").toDouble();
    }
    else
    {
        return -1.0; // Return a negative value to indicate account not found
    }
}

QJsonObject DatabaseManager::createNewAccount(const QJsonObject &jsonObject,
                                              const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QJsonObject responseObj;
    responseObj["responseId"] = 3;

    bool isAdmin = jsonObject["isAdmin"].toBool();
    QString username = jsonObject["username"].toString();
    QString password = jsonObject["password"].toString();
    QString name = jsonObject["name"].toString();
    int age = jsonObject["age"].toInt();
    double balance = 0.0;

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    db.transaction();

    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT COUNT(*) FROM Accounts WHERE Username = :username");
    checkQuery.bindValue(":username", username);

    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0)
    {
        responseObj["createAccountSuccess"] = false;
        responseObj["errorMessage"] = "exists";
        db.rollback();
        return responseObj;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO Accounts (Username, Password, Admin) VALUES (:username, :password, :admin)");
    insertQuery.bindValue(":username", username);
    insertQuery.bindValue(":password", password);
    insertQuery.bindValue(":admin", isAdmin);

    if (!insertQuery.exec())
    {
        responseObj["createAccountSuccess"] = false;
        responseObj["errorMessage"] = "failed";
        db.rollback();
        return responseObj;
    }

    qint64 accountNumber = insertQuery.lastInsertId().toLongLong();

    QSqlQuery personalDataQuery(db);
    personalDataQuery.prepare("INSERT INTO Users_Personal_Data (AccountNumber, Name, Age, Balance) "
                              "VALUES (:accountNumber, :name, :age, :balance)");
    personalDataQuery.bindValue(":accountNumber", accountNumber);
    personalDataQuery.bindValue(":name", name);
    personalDataQuery.bindValue(":age", age);
    personalDataQuery.bindValue(":balance", balance);

    if (!personalDataQuery.exec())
    {
        responseObj["createAccountSuccess"] = false;
        responseObj["errorMessage"] = "failed";
        db.rollback();
        return responseObj;
    }

    db.commit();
    responseObj["createAccountSuccess"] = true;
    responseObj["accountNumber"] = accountNumber;
    return responseObj;
}

bool DatabaseManager::deleteAccount(qint64 accountNumber, const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);

    // Start a transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start transaction.");
        return false;
    }

    QSqlQuery deleteQuery(dbConnection);
    deleteQuery.prepare("DELETE FROM Accounts WHERE AccountNumber = :accountNumber");
    deleteQuery.bindValue(":accountNumber", accountNumber);

    if (!deleteQuery.exec())
    {
        logger.log("Failed to delete account from Accounts table.");
        dbConnection.rollback();
        return false;
    }

    QSqlQuery deletePersonalDataQuery(dbConnection);
    deletePersonalDataQuery.prepare("DELETE FROM Users_Personal_Data WHERE AccountNumber = :accountNumber");
    deletePersonalDataQuery.bindValue(":accountNumber", accountNumber);

    if (!deletePersonalDataQuery.exec())
    {
        logger.log("Failed to delete account from Users_Personal_Data table.");
        dbConnection.rollback();
        return false;
    }

    QSqlQuery deleteTransactionQuery(dbConnection);
    deleteTransactionQuery.prepare("DELETE FROM Transaction_History WHERE AccountNumber = :accountNumber");
    deleteTransactionQuery.bindValue(":accountNumber", accountNumber);

    // Execute the delete query for Transaction_History, ignoring any failures
    deleteTransactionQuery.exec();

    // If all delete operations succeed, commit the transaction
    if (!dbConnection.commit())
    {
        logger.log("Failed to commit transaction.");
        return false;
    }

    return true;
}

QJsonObject DatabaseManager::fetchAllUserData(const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QJsonObject responseObj;

    QSqlDatabase db = QSqlDatabase::database(connectionName);

    QSqlQuery fetchAllUserDataQuery(db);
    fetchAllUserDataQuery.prepare("SELECT Accounts.AccountNumber, Accounts.Username, Users_Personal_Data.Name, "
                                  "Users_Personal_Data.Balance, Users_Personal_Data.Age "
                                  "FROM Accounts JOIN Users_Personal_Data "
                                  "ON Accounts.AccountNumber = Users_Personal_Data.AccountNumber");

    if (!fetchAllUserDataQuery.exec())
    {
        responseObj["fetchUserDataSuccess"] = false;
        responseObj["errorMessage"] = "failed";
        return responseObj;
    }

    // Create a JSON array to store user data
    QJsonArray userDataArray;

    while (fetchAllUserDataQuery.next())
    {
        QJsonObject userData;
        userData["AccountNumber"] = fetchAllUserDataQuery.value("AccountNumber").toLongLong();
        userData["Username"] = fetchAllUserDataQuery.value("Username").toString();
        userData["Name"] = fetchAllUserDataQuery.value("Name").toString();
        userData["Balance"] = fetchAllUserDataQuery.value("Balance").toDouble();
        userData["Age"] = fetchAllUserDataQuery.value("Age").toInt();

        userDataArray.append(userData);
    }

    responseObj["fetchUserDataSuccess"] = true;
    responseObj["userData"] = userDataArray;

    return responseObj;
}

QJsonObject DatabaseManager::makeTransaction(const QJsonObject &jsonObject, const QString &connectionName)
{
    QJsonObject responseObj;
    qint64 accountNumber = jsonObject["accountNumber"].toVariant().toLongLong();
    double amount = jsonObject["amount"].toDouble();
    // Check if the balance is sufficient
    double currentBalance = getAccountBalance(accountNumber, connectionName);

    QMutexLocker locker(&mutex);

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    db.transaction();

    if (currentBalance < 0 || currentBalance + amount < 0)
    {
        responseObj["transactionSuccess"] = false;
        responseObj["errorMessage"] = "Insufficient balance";
        db.rollback();
        return responseObj;
    }

    // Update the balance
    double newBalance = currentBalance + amount;  // Reverse the logic here
    QSqlQuery updateBalanceQuery(db);
    updateBalanceQuery.prepare("UPDATE Users_Personal_Data SET Balance = :balance WHERE AccountNumber = :accountNumber");
    updateBalanceQuery.bindValue(":balance", newBalance);
    updateBalanceQuery.bindValue(":accountNumber", accountNumber);

    if (!updateBalanceQuery.exec())
    {
        responseObj["transactionSuccess"] = false;
        responseObj["errorMessage"] = "Failed to update balance";
        db.rollback();
        return responseObj;
    }

    // Log the transaction in the Transaction_History table
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDate = currentDateTime.toString("dd-MM-yyyy");
    QString formattedTime = currentDateTime.toString("hh:mm:ss");

    QSqlQuery logTransactionQuery(db);
    logTransactionQuery.prepare("INSERT INTO Transaction_History (AccountNumber, Date, Time, Amount) "
                                "VALUES (:accountNumber, :date, :time, :amount)");
    logTransactionQuery.bindValue(":accountNumber", accountNumber);
    logTransactionQuery.bindValue(":date", formattedDate);
    logTransactionQuery.bindValue(":time", formattedTime);
    logTransactionQuery.bindValue(":amount", amount);

    if (!logTransactionQuery.exec())
    {
        responseObj["transactionSuccess"] = false;
        responseObj["errorMessage"] = "Failed to log transaction";
        db.rollback();
        return responseObj;
    }

    db.commit();
    responseObj["transactionSuccess"] = true;
    responseObj["newBalance"] = newBalance;

    return responseObj;
}


QJsonObject DatabaseManager::makeTransfer(qint64 fromAccountNumber, qint64 toAccountNumber, double amount, const QString &connectionName)
{
    QJsonObject responseObj;

    // Check if the 'from' account has sufficient balance
    double fromAccountBalance = getAccountBalance(fromAccountNumber, connectionName);

    if (fromAccountBalance < 0 || fromAccountBalance - amount < 0)
    {
        responseObj["transferSuccess"] = false;
        responseObj["errorMessage"] = "Insufficient balance for the transfer";
        return responseObj;
    }

    // Update the 'from' account balance
    double newFromBalance = fromAccountBalance - amount;
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery updateFromBalanceQuery(db);
    updateFromBalanceQuery.prepare("UPDATE Users_Personal_Data SET Balance = :balance WHERE AccountNumber = :accountNumber");
    updateFromBalanceQuery.bindValue(":balance", newFromBalance);
    updateFromBalanceQuery.bindValue(":accountNumber", fromAccountNumber);

    if (!updateFromBalanceQuery.exec())
    {
        responseObj["transferSuccess"] = false;
        responseObj["errorMessage"] = "Failed to update 'from' account balance";
        return responseObj;
    }

    // Update the 'to' account balance
    double toAccountBalance = getAccountBalance(toAccountNumber, connectionName);
    QMutexLocker locker(&mutex);
    double newToBalance = toAccountBalance + amount;
    QSqlQuery updateToBalanceQuery(db);
    updateToBalanceQuery.prepare("UPDATE Users_Personal_Data SET Balance = :balance WHERE AccountNumber = :accountNumber");
    updateToBalanceQuery.bindValue(":balance", newToBalance);
    updateToBalanceQuery.bindValue(":accountNumber", toAccountNumber);

    if (!updateToBalanceQuery.exec())
    {
        responseObj["transferSuccess"] = false;
        responseObj["errorMessage"] = "Failed to update 'to' account balance";
        return responseObj;
    }

    // Log the transfer in the Transaction_History table for 'from' account
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDate = currentDateTime.toString("dd-MM-yyyy");
    QString formattedTime = currentDateTime.toString("hh:mm:ss");

    QSqlQuery logFromTransactionQuery(db);
    logFromTransactionQuery.prepare("INSERT INTO Transaction_History (AccountNumber, Date, Time, Amount) "
                                    "VALUES (:accountNumber, :date, :time, :amount)");
    logFromTransactionQuery.bindValue(":accountNumber", fromAccountNumber);
    logFromTransactionQuery.bindValue(":date", formattedDate);
    logFromTransactionQuery.bindValue(":time", formattedTime);
    logFromTransactionQuery.bindValue(":amount", -amount); // Negative amount for 'from' account

    if (!logFromTransactionQuery.exec())
    {
        responseObj["transferSuccess"] = false;
        responseObj["errorMessage"] = "Failed to log 'from' account transaction";
        return responseObj;
    }

    // Log the transfer in the Transaction_History table for 'to' account
    QSqlQuery logToTransactionQuery(db);
    logToTransactionQuery.prepare("INSERT INTO Transaction_History (AccountNumber, Date, Time, Amount) "
                                  "VALUES (:accountNumber, :date, :time, :amount)");
    logToTransactionQuery.bindValue(":accountNumber", toAccountNumber);
    logToTransactionQuery.bindValue(":date", formattedDate);
    logToTransactionQuery.bindValue(":time", formattedTime);
    logToTransactionQuery.bindValue(":amount", amount); // Positive amount for 'to' account

    if (!logToTransactionQuery.exec())
    {
        responseObj["transferSuccess"] = false;
        responseObj["errorMessage"] = "Failed to log 'to' account transaction";
        return responseObj;
    }

    responseObj["transferSuccess"] = true;
    responseObj["newFromBalance"] = newFromBalance;
    responseObj["newToBalance"] = newToBalance;

    return responseObj;
}

QJsonArray DatabaseManager::viewTransactionHistory(qint64 accountNumber, const QString &connectionName)
{
    QMutexLocker locker(&mutex);
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery query(dbConnection);

    query.prepare("SELECT TransactionID, Date, Time, Amount FROM Transaction_History "
                  "WHERE AccountNumber = :accountNumber ORDER BY Date DESC, Time DESC");

    query.bindValue(":accountNumber", accountNumber);

    QJsonArray transactionHistoryArray;

    if (query.exec())
    {
        while (query.next())
        {
            QJsonObject transactionObj;
            transactionObj["TransactionID"] = query.value("TransactionID").toLongLong();
            transactionObj["Date"] = query.value("Date").toString();
            transactionObj["Time"] = query.value("Time").toString();
            transactionObj["Amount"] = query.value("Amount").toDouble();

            transactionHistoryArray.append(transactionObj);
        }
    }

    return transactionHistoryArray;
}

