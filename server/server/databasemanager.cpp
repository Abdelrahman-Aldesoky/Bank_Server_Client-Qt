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
    QSqlDatabase::removeDatabase(connectionName);
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

QJsonObject DatabaseManager::processRequest(QJsonObject requestJson)
{
    QMutexLocker locker(&mutex);

    // Extract the request ID from the request JSON
    int requestId = requestJson["requestId"].toInt();

    QJsonObject responseJson;

    // Process the request based on the request ID
    switch (requestId)
    {
    case 0:
        responseJson = login(requestJson);
        break;
    case 1:
        responseJson = getAccountNumber(requestJson);
        break;
    case 2:
        responseJson = getAccountBalance(requestJson);
        break;
    case 3:
        responseJson = createNewAccount(requestJson);
        break;
    case 4:
        responseJson = deleteAccount(requestJson);
        break;
    case 5:
        responseJson = fetchAllUserData();
        break;
    case 6:
        responseJson = makeTransaction(requestJson);
        break;
    case 7:
        responseJson = makeTransfer(requestJson);
        break;
    case 8:
        responseJson = viewTransactionHistory(requestJson);
        break;
    case 9:
        responseJson = updateUserData(requestJson);
        break;
    default:
        // Handle unknown request
        logger.log("Unknown request");
        break;
    }

    // Add the response ID to the response JSON
    responseJson["responseId"] = requestId;

    return responseJson;
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
    const QString insert_default_admin =
        "INSERT INTO Accounts (Username, Password, Admin) "
        "VALUES ('admin', 'admin', 1);";
    if (!createTablesQuery.exec(insert_default_admin))
    {
        logger.log("Failed to insert default admin account.");
        logger.log("Error: " + createTablesQuery.lastError().text());
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

QJsonObject DatabaseManager::login(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery loginQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["loginSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for login.");
        return responseJson;
    }

    // Extract the username and password from the request JSON
    QString username = requestJson["username"].toString();
    QString password = requestJson["password"].toString();

    loginQuery.prepare("SELECT AccountNumber, Admin FROM Accounts "
                       "WHERE Username = :username AND Password = :password");
    loginQuery.bindValue(":username", username);
    loginQuery.bindValue(":password", password);

    if (!loginQuery.exec())
    {
        logger.log("Failed execution for login query.");
        logger.log("Error: " + loginQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    if (loginQuery.next())
    {
        // Login successful
        responseJson["loginSuccess"] = true;
        responseJson["accountNumber"] = loginQuery.value("AccountNumber").toLongLong();
        responseJson["isAdmin"] = loginQuery.value("Admin").toBool();
    }
    else
    {
        // Login failed
        logger.log("Login failed.");
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        logger.log("Failed to commit transaction for login.");
        dbConnection.rollback();
        return responseJson;
    }

    return responseJson;
}

QJsonObject DatabaseManager::getAccountNumber(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery accountNumberQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["userFound"] = false;

    // Extract the username from the request JSON
    QString username = requestJson["username"].toString();

    accountNumberQuery.prepare("SELECT AccountNumber FROM Accounts WHERE Username = :username");
    accountNumberQuery.bindValue(":username", username);

    if (!accountNumberQuery.exec())
    {
        logger.log("Failed execution for getAccountNumber query.");
        logger.log("Error: " + accountNumberQuery.lastError().text());
        return responseJson;
    }

    if (accountNumberQuery.next())
    {
        // User found
        responseJson["accountNumber"] = accountNumberQuery.value("AccountNumber").toLongLong();
        responseJson["userFound"] = true;
    }

    return responseJson;
}

QJsonObject DatabaseManager::getAccountBalance(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery balanceQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["accountFound"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    balanceQuery.prepare("SELECT Balance FROM Users_Personal_Data WHERE AccountNumber = :accountNumber");
    balanceQuery.bindValue(":accountNumber", accountNumber);

    if (!balanceQuery.exec())
    {
        logger.log("Failed execution for getAccountBalance query.");
        logger.log("Error: " + balanceQuery.lastError().text());
        return responseJson;
    }

    if (balanceQuery.next())
    {
        // Account found
        responseJson["balance"] = balanceQuery.value("Balance").toDouble();
        responseJson["accountFound"] = true;
    }

    return responseJson;
}

QJsonObject DatabaseManager::createNewAccount(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery checkQuery(dbConnection), insertAccountsQuery(dbConnection), insertPersonalDataQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["createAccountSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for createNewAccount.");
        return responseJson;
    }

    // Extract the necessary data from the request JSON
    bool isAdmin = requestJson["isAdmin"].toBool();
    QString username = requestJson["username"].toString();
    QString password = requestJson["password"].toString();
    QString name = requestJson["name"].toString();
    int age = requestJson["age"].toInt();
    double balance = 0.0;

    checkQuery.prepare("SELECT COUNT(*) FROM Accounts WHERE Username = :username");
    checkQuery.bindValue(":username", username);

    if (!checkQuery.exec() || !checkQuery.next() || checkQuery.value(0).toInt() > 0)
    {
        responseJson["errorMessage"] = "exists";
        logger.log("Username already exists.");
        dbConnection.rollback();
        return responseJson;
    }

    insertAccountsQuery.prepare("INSERT INTO Accounts (Username, Password, Admin) VALUES (:username, :password, :admin)");
    insertAccountsQuery.bindValue(":username", username);
    insertAccountsQuery.bindValue(":password", password);
    insertAccountsQuery.bindValue(":admin", isAdmin);

    if (!insertAccountsQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to insert new account.");
        logger.log("Error: " + insertAccountsQuery.lastError().text());
        return responseJson;
    }
    qint64 accountNumber = insertAccountsQuery.lastInsertId().toLongLong();

    insertPersonalDataQuery.prepare("INSERT INTO Users_Personal_Data (AccountNumber, Name, Age, Balance) "
                              "VALUES (:accountNumber, :name, :age, :balance)");
    insertPersonalDataQuery.bindValue(":accountNumber", accountNumber);
    insertPersonalDataQuery.bindValue(":name", name);
    insertPersonalDataQuery.bindValue(":age", age);
    insertPersonalDataQuery.bindValue(":balance", balance);

    if (!insertPersonalDataQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to insert personal data.");
        logger.log("Error: " + insertPersonalDataQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for createNewAccount.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["createAccountSuccess"] = true;
    responseJson["accountNumber"] = accountNumber;
    return responseJson;
}

QJsonObject DatabaseManager::deleteAccount(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery deleteQuery(dbConnection), deletePersonalDataQuery(dbConnection), deleteTransactionQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["deleteAccountSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for deleteAccount.");
        return responseJson;
    }

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    deleteQuery.prepare("DELETE FROM Accounts WHERE AccountNumber = :accountNumber");
    deleteQuery.bindValue(":accountNumber", accountNumber);

    if (!deleteQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to delete account from Accounts table.");
        logger.log("Error: " + deleteQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    deletePersonalDataQuery.prepare("DELETE FROM Users_Personal_Data WHERE AccountNumber = :accountNumber");
    deletePersonalDataQuery.bindValue(":accountNumber", accountNumber);

    if (!deletePersonalDataQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to delete account from Users_Personal_Data table.");
        logger.log("Error: " + deletePersonalDataQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    deleteTransactionQuery.prepare("DELETE FROM Transaction_History WHERE AccountNumber = :accountNumber");
    deleteTransactionQuery.bindValue(":accountNumber", accountNumber);

    if (!deleteTransactionQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to delete transaction history for the account.");
        logger.log("Error: " + deleteTransactionQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for deleteAccount.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["deleteAccountSuccess"] = true;
    return responseJson;
}

QJsonObject DatabaseManager::fetchAllUserData()
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery fetchAllUserDataQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["fetchUserDataSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for fetchAllUserData.");
        return responseJson;
    }

    fetchAllUserDataQuery.prepare("SELECT Accounts.AccountNumber, Accounts.Username, Users_Personal_Data.Name, "
                                  "Users_Personal_Data.Balance, Users_Personal_Data.Age "
                                  "FROM Accounts JOIN Users_Personal_Data "
                                  "ON Accounts.AccountNumber = Users_Personal_Data.AccountNumber");

    if (!fetchAllUserDataQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to fetch all user data.");
        logger.log("Error: " + fetchAllUserDataQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
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

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for fetchAllUserData.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["fetchUserDataSuccess"] = true;
    responseJson["userData"] = userDataArray;
    return responseJson;
}

QJsonObject DatabaseManager::viewTransactionHistory(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery transactionHistoryQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["viewTransactionHistorySuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for viewTransactionHistory.");
        return responseJson;
    }

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    transactionHistoryQuery.prepare("SELECT TransactionID, Date, Time, Amount FROM Transaction_History "
                                    "WHERE AccountNumber = :accountNumber ORDER BY Date DESC, Time DESC");
    transactionHistoryQuery.bindValue(":accountNumber", accountNumber);

    if (!transactionHistoryQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to fetch transaction history.");
        logger.log("Error: " + transactionHistoryQuery.lastError().text());
        dbConnection.rollback();
        return responseJson;
    }

    // Create a JSON array to store transaction history
    QJsonArray transactionHistoryArray;

    while (transactionHistoryQuery.next())
    {
        QJsonObject transactionObj;
        transactionObj["TransactionID"] = transactionHistoryQuery.value("TransactionID").toLongLong();
        transactionObj["Date"] = transactionHistoryQuery.value("Date").toString();
        transactionObj["Time"] = transactionHistoryQuery.value("Time").toString();
        transactionObj["Amount"] = transactionHistoryQuery.value("Amount").toDouble();

        transactionHistoryArray.append(transactionObj);
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for viewTransactionHistory.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["viewTransactionHistorySuccess"] = true;
    responseJson["transactionHistory"] = transactionHistoryArray;
    return responseJson;
}

QJsonObject DatabaseManager::updateUserData(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery checkQuery(dbConnection), updateQuery(dbConnection), updatePersonalDataQuery(dbConnection);

    QJsonObject responseJson;
    responseJson["updateSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for updateUserData.");
        return responseJson;
    }

    // Extract the necessary data from the request JSON
    QString username = requestJson["username"].toString();
    QString name = requestJson["name"].toString();
    QString password = requestJson["password"].toString();

    checkQuery.prepare("SELECT AccountNumber FROM Accounts WHERE Username = :username");
    checkQuery.bindValue(":username", username);

    if (!checkQuery.exec() || !checkQuery.next())
    {
        responseJson["errorMessage"] = "Account not found";
        logger.log("Account not found.");
        dbConnection.rollback();
        return responseJson;
    }
    qint64 accountNumber = checkQuery.value(0).toLongLong();

    if (!password.isEmpty())
    {
        updateQuery.prepare("UPDATE Accounts SET Password = :password WHERE Username = :username");
        updateQuery.bindValue(":username", username);
        updateQuery.bindValue(":password", password);

        if (!updateQuery.exec())
        {
            responseJson["errorMessage"] = "Failed to update password";
            logger.log("Failed to update password.");
            logger.log("Error: " + updateQuery.lastError().text());
            dbConnection.rollback();
            return responseJson;
        }
    }

    if (!name.isEmpty())
    {
        updatePersonalDataQuery.prepare("UPDATE Users_Personal_Data SET Name = :name WHERE AccountNumber = :accountNumber");
        updatePersonalDataQuery.bindValue(":accountNumber", accountNumber);
        updatePersonalDataQuery.bindValue(":name", name);

        if (!updatePersonalDataQuery.exec())
        {
            responseJson["errorMessage"] = "Failed to update name";
            logger.log("Failed to update name.");
            logger.log("Error: " + updatePersonalDataQuery.lastError().text());
            dbConnection.rollback();
            return responseJson;
        }
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for updateUserData.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["updateSuccess"] = true;
    return responseJson;
}

QJsonObject DatabaseManager::makeTransaction(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);

    QJsonObject responseJson;
    responseJson["transactionSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for makeTransaction.");
        return responseJson;
    }

    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();
    double amount = requestJson["amount"].toDouble();

    // Create a QJsonObject for getAccountBalance
    QJsonObject accountJson;
    accountJson["accountNumber"] = accountNumber;

    // Check if the balance is sufficient
    double currentBalance = getAccountBalance(accountJson)["balance"].toDouble();
    if (currentBalance < 0 || currentBalance + amount < 0)
    {
        responseJson["errorMessage"] = "Insufficient balance";
        logger.log("Insufficient balance.");
        dbConnection.rollback();
        return responseJson;
    }

    // Update the balance
    if (!updateAccountBalance(accountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to update balance";
        logger.log("Failed to update balance.");
        dbConnection.rollback();
        return responseJson;
    }

    // Log the transaction in the Transaction_History table
    if (!logTransaction(accountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log transaction";
        logger.log("Failed to log transaction.");
        dbConnection.rollback();
        return responseJson;
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for makeTransaction.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["transactionSuccess"] = true;
    responseJson["newBalance"] = getAccountBalance(accountJson)["balance"].toDouble();
    return responseJson;
}

QJsonObject DatabaseManager::makeTransfer(QJsonObject requestJson)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);

    QJsonObject responseJson;
    responseJson["transferSuccess"] = false;

    // Begin transaction
    if (!dbConnection.transaction())
    {
        logger.log("Failed to start a transaction for makeTransfer.");
        return responseJson;
    }

    qint64 fromAccountNumber = requestJson["fromAccountNumber"].toVariant().toLongLong();
    qint64 toAccountNumber = requestJson["toAccountNumber"].toVariant().toLongLong();
    double amount = requestJson["amount"].toDouble();

    // Check if the 'from' and 'to' account numbers are the same
    if (fromAccountNumber == toAccountNumber)
    {
        responseJson["errorMessage"] = "Cannot transfer money to yourself!";
        logger.log("Cannot transfer money to yourself.");
        dbConnection.rollback();
        return responseJson;
    }

    // Check if the 'from' account has sufficient balance
    QJsonObject fromAccountJson;
    fromAccountJson["accountNumber"] = QJsonValue::fromVariant(fromAccountNumber);
    double fromAccountBalance = getAccountBalance(fromAccountJson)["balance"].toDouble();
    if (fromAccountBalance < amount)
    {
        responseJson["errorMessage"] = "Insufficient balance for the transfer";
        logger.log("Insufficient balance for the transfer.");
        dbConnection.rollback();
        return responseJson;
    }

    // Update the 'from' and 'to' account balances
    if (!updateAccountBalance(fromAccountNumber, -amount) || !updateAccountBalance(toAccountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to update account balance";
        logger.log("Failed to update account balance.");
        dbConnection.rollback();
        return responseJson;
    }

    // Log the transfer in the Transaction_History table for both 'from' and 'to' accounts
    if (!logTransaction(fromAccountNumber, -amount) || !logTransaction(toAccountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log transaction";
        logger.log("Failed to log transaction.");
        dbConnection.rollback();
        return responseJson;
    }

    // Commit transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to commit transaction for makeTransfer.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["transferSuccess"] = true;
    QJsonObject toAccountJson;
    toAccountJson["accountNumber"] = QJsonValue::fromVariant(toAccountNumber);
    responseJson["newFromBalance"] = getAccountBalance(fromAccountJson)["balance"].toDouble();
    responseJson["newToBalance"] = getAccountBalance(toAccountJson)["balance"].toDouble();
    return responseJson;
}

bool DatabaseManager::updateAccountBalance(qint64 accountNumber, double amount)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery updateBalanceQuery(dbConnection);

    // Get the current balance
    QJsonObject accountJson;
    accountJson["accountNumber"] = QJsonValue::fromVariant(accountNumber);
    double currentBalance = getAccountBalance(accountJson)["balance"].toDouble();

    // Calculate the new balance
    double newBalance = currentBalance + amount;

    // Update the balance in the database
    updateBalanceQuery.prepare("UPDATE Users_Personal_Data SET Balance = :balance WHERE AccountNumber = :accountNumber");
    updateBalanceQuery.bindValue(":balance", newBalance);
    updateBalanceQuery.bindValue(":accountNumber", accountNumber);

    bool success = updateBalanceQuery.exec();
    return success;
}

bool DatabaseManager::logTransaction(qint64 accountNumber, double amount)
{
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery logTransactionQuery(dbConnection);

    // Get the current date and time
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDate = currentDateTime.toString("dd-MM-yyyy");
    QString formattedTime = currentDateTime.toString("hh:mm:ss");

    // Log the transaction in the database
    logTransactionQuery.prepare("INSERT INTO Transaction_History (AccountNumber, Date, Time, Amount) "
                                "VALUES (:accountNumber, :date, :time, :amount)");
    logTransactionQuery.bindValue(":accountNumber", accountNumber);
    logTransactionQuery.bindValue(":date", formattedDate);
    logTransactionQuery.bindValue(":time", formattedTime);
    logTransactionQuery.bindValue(":amount", amount);

    bool success = logTransactionQuery.exec();
    return success;
}
