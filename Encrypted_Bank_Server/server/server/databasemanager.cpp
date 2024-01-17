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
        responseJson = viewDatabase();
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
}

QJsonObject DatabaseManager::login(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["loginSuccess"] = false;

    // Extract the username and password from the request JSON
    QString username = requestJson["username"].toString();
    QString password = requestJson["password"].toString();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["Username"] = username;
    searchCriteria["Password"] = password;

    // Fetch the account number and admin status
    QVariant accountNumberData = fetchData("Accounts", "AccountNumber",
                                           searchCriteria);
    QVariant adminData = fetchData("Accounts", "Admin", searchCriteria);

    if (!accountNumberData.isNull() && !adminData.isNull())
    {
        // Login successful
        responseJson["loginSuccess"] = true;
        responseJson["accountNumber"] = accountNumberData.toLongLong();
        responseJson["isAdmin"] = adminData.toBool();
    }
    else
    {
        // Login failed
        logger.log("Login failed.");
    }

    return responseJson;
}

QJsonObject DatabaseManager::getAccountNumber(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["userFound"] = false;

    // Extract the username from the request JSON
    QString username = requestJson["username"].toString();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["Username"] = username;

    // Fetch the account number
    QVariant accountNumber = fetchData("Accounts", "AccountNumber",
                                       searchCriteria);

    if (!accountNumber.isNull())
    {
        // User found
        responseJson["accountNumber"] = accountNumber.toLongLong();
        responseJson["userFound"] = true;
    }
    else
    {
        // User not found
        logger.log("User not found.");
    }

    return responseJson;
}

QJsonObject DatabaseManager::getAccountBalance(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["accountFound"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["AccountNumber"] = accountNumber;

    // Fetch the account balance
    QVariant balance = fetchData("Users_Personal_Data", "Balance", searchCriteria);

    if (!balance.isNull())
    {
        // Account found
        responseJson["balance"] = balance.toDouble();
        responseJson["accountFound"] = true;
    }
    else
    {
        // Account not found

        logger.log("Account not found.");
    }

    return responseJson;
}

QJsonObject DatabaseManager::createNewAccount(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["createAccountSuccess"] = false;

    // Extract the necessary data from the request JSON
    bool isAdmin = requestJson["isAdmin"].toBool();
    QString username = requestJson["username"].toString();
    QString password = requestJson["password"].toString();
    QString name = requestJson["name"].toString();
    int age = requestJson["age"].toInt();
    double balance = 0.0;

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["Username"] = username;

    // Check if the username already exists
    QVariant usernameExists = fetchData("Accounts", "COUNT(*)", searchCriteria);

    if (!usernameExists.isNull() && usernameExists.toInt() > 0)
    {
        responseJson["errorMessage"] = "Username already exists.";
        logger.log("Username already exists.");
        return responseJson;
    }

    // Start a transaction
    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    if (!dbConnection.transaction())
    {
        responseJson["errorMessage"] = "Failed to start a transaction for createNewAccount.";
        logger.log("Failed to start a transaction for createNewAccount.");
        return responseJson;
    }

    // Insert the new account into the Accounts table
    QJsonObject accountData;
    accountData["Username"] = username;
    accountData["Password"] = password;
    accountData["Admin"] = isAdmin;

    qint64 accountNumber = insertData("Accounts", accountData);
    if (!accountNumber)
    {
        responseJson["errorMessage"] = "Failed to insert new account";
        logger.log("Failed to insert new account.");
        dbConnection.rollback();
        return responseJson;
    }

    // Insert the personal data into the Users_Personal_Data table
    QJsonObject personalData;
    personalData["AccountNumber"] = accountNumber;
    personalData["Name"] = name;
    personalData["Age"] = age;
    personalData["Balance"] = balance;

    if (!insertData("Users_Personal_Data", personalData))
    {
        responseJson["errorMessage"] = "Failed to insert personal data.";
        logger.log("Failed to insert personal data.");
        dbConnection.rollback();
        return responseJson;
    }

    // Commit the transaction
    if (!dbConnection.commit())
    {
        responseJson["errorMessage"] = "Failed to commit transaction for createNewAccount.";
        logger.log("Failed to commit transaction for createNewAccount.");
        dbConnection.rollback();
        return responseJson;
    }

    responseJson["createAccountSuccess"] = true;
    responseJson["accountNumber"] = static_cast<qint64>(accountNumber);
    return responseJson;
}

QJsonObject DatabaseManager::deleteAccount(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["deleteAccountSuccess"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["AccountNumber"] = accountNumber;

    // Start a transaction
    if (!startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start a transaction for deleteAccount.";
        logger.log("Failed to start a transaction for deleteAccount.");
        return responseJson;
    }

    // Delete the account from the Accounts table
    if (!removeData("Accounts", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete account from Accounts table.";
        logger.log("Failed to delete account from Accounts table.");
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Delete the personal data from the Users_Personal_Data table
    if (!removeData("Users_Personal_Data", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete account from Users_Personal_Data table.";
        logger.log("Failed to delete account from Users_Personal_Data table.");
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Delete the transaction history from the Transaction_History table
    if (!removeData("Transaction_History", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete transaction history for the account.";
        logger.log("Failed to delete transaction history for the account.");
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for deleteAccount.");
        if (!rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for deleteAccount.");
        }
        responseJson["errorMessage"] = "failed";
        return responseJson;
    }

    responseJson["deleteAccountSuccess"] = true;
    return responseJson;
}

QJsonObject DatabaseManager::viewDatabase()
{
    QJsonObject responseJson;
    responseJson["fetchUserDataSuccess"] = false;

    QSqlDatabase dbConnection = QSqlDatabase::database(connectionName);
    QSqlQuery fetchAllUserRecordsQuery(dbConnection);
    fetchAllUserRecordsQuery.prepare
        ("SELECT Accounts.AccountNumber, Accounts.Username, Accounts.Admin,"
                                     " Users_Personal_Data.Name, "
                                     "Users_Personal_Data.Balance,"
                                     " Users_Personal_Data.Age "
                                     "FROM Accounts JOIN Users_Personal_Data "
                                     "ON Accounts.AccountNumber = Users_Personal_Data.AccountNumber");

    if (!fetchAllUserRecordsQuery.exec())
    {
        responseJson["errorMessage"] = "failed";
        logger.log("Failed to fetch all user data.");
        logger.log("Error: " + fetchAllUserRecordsQuery.lastError().text());
        return responseJson;
    }

    // Create a JSON array to store user data
    QJsonArray userDataArray;

    while (fetchAllUserRecordsQuery.next())
    {
        QJsonObject userDataJson;
        userDataJson["AccountNumber"] = fetchAllUserRecordsQuery.
                                        value("AccountNumber").toLongLong();
        userDataJson["Username"] = fetchAllUserRecordsQuery.
                                   value("Username").toString();
        userDataJson["isAdmin"] = fetchAllUserRecordsQuery.
                                  value("Admin").toBool();
        userDataJson["Name"] = fetchAllUserRecordsQuery.
                               value("Name").toString();
        userDataJson["Balance"] = fetchAllUserRecordsQuery.
                                  value("Balance").toDouble();
        userDataJson["Age"] = fetchAllUserRecordsQuery.
                              value("Age").toInt();

        userDataArray.append(userDataJson);
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

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    transactionHistoryQuery.prepare
        ("SELECT TransactionID, Date, Time, Amount FROM Transaction_History "
         "WHERE AccountNumber = :accountNumber ORDER BY Date DESC, Time DESC");
    transactionHistoryQuery.bindValue(":accountNumber", accountNumber);

    if (!transactionHistoryQuery.exec())
    {
        responseJson["errorMessage"] = "Database query execution failed.";
        logger.log("Failed to fetch transaction history.");
        logger.log("Error: " + transactionHistoryQuery.lastError().text());
        return responseJson;
    }

    // Create a JSON array to store transaction history
    QJsonArray transactionHistoryArray;

    while (transactionHistoryQuery.next())
    {
        QJsonObject transactionObj;
        transactionObj["TransactionID"] = transactionHistoryQuery.
                                          value("TransactionID").toLongLong();
        transactionObj["Date"] = transactionHistoryQuery.
                                 value("Date").toString();
        transactionObj["Time"] = transactionHistoryQuery.
                                 value("Time").toString();
        transactionObj["Amount"] = transactionHistoryQuery.
                                   value("Amount").toDouble();

        transactionHistoryArray.append(transactionObj);
    }

    // Check if any data was fetched
    if (transactionHistoryArray.isEmpty())
    {
        responseJson["errorMessage"] =
            "No transaction history found for the given account number.";
        logger.log("No transaction history found for account number: "
                   + QString::number(accountNumber));
        return responseJson;
    }

    responseJson["viewTransactionHistorySuccess"] = true;
    responseJson["transactionHistory"] = transactionHistoryArray;

    return responseJson;
}

QJsonObject DatabaseManager::updateUserData(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["updateSuccess"] = false;

    // Extract the necessary data from the request JSON
    QString username = requestJson["username"].toString();
    QString name = requestJson["name"].toString();
    QString password = requestJson["password"].toString();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["Username"] = username;

    // Start a transaction
    if (!startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start a transaction for updateUserData.";
        logger.log("Failed to start a transaction for updateUserData.");
        return responseJson;
    }

    // Fetch the account number
    QVariant accountNumberVariant = fetchData("Accounts", "AccountNumber", searchCriteria);
    if (!accountNumberVariant.isValid())
    {
        responseJson["errorMessage"] = "Account not found";
        logger.log("Account not found.");
        rollbackDatabaseTransaction();
        return responseJson;
    }
    qint64 accountNumber = accountNumberVariant.toLongLong();

    // Prepare the search criteria for updating data
    QJsonObject updateSearchCriteria;
    updateSearchCriteria["AccountNumber"] = accountNumber;

    // Update the password in the Accounts table
    if (!password.isEmpty())
    {
        QJsonObject passwordData;
        passwordData["Password"] = password;

        if (!updateData("Accounts", passwordData, searchCriteria))
        {
            responseJson["errorMessage"] = "failed to update password";
            logger.log("Failed to update password.");
            rollbackDatabaseTransaction();
            return responseJson;
        }
    }

    // Update the name in the Users_Personal_Data table
    if (!name.isEmpty())
    {
        QJsonObject nameData;
        nameData["Name"] = name;

        if (!updateData("Users_Personal_Data", nameData, updateSearchCriteria))
        {
            responseJson["errorMessage"] = "failed to update name";
            logger.log("Failed to update name.");
            rollbackDatabaseTransaction();
            return responseJson;
        }
    }

    // Commit the transaction
    if (!commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for updateUserData.");
        if (!rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for updateUserData.");
        }
        responseJson["errorMessage"] = "Failed to commit transaction.";
        return responseJson;
    }

    responseJson["updateSuccess"] = true;
    return responseJson;
}

QJsonObject DatabaseManager::makeTransaction(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["transactionSuccess"] = false;

    // Extract the necessary data from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();
    double amount = requestJson["amount"].toDouble();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["AccountNumber"] = accountNumber;

    // Start a transaction
    if (!startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start transaction.";
        return responseJson;
    }

    // Fetch the current balance
    QVariant currentBalance = fetchData("Users_Personal_Data", "Balance",
                                        searchCriteria);

    // Check if the balance is sufficient
    if (currentBalance.toDouble() < 0 || currentBalance.toDouble() + amount < 0)
    {
        responseJson["errorMessage"] = "Insufficient balance";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Update the balance
    QJsonObject balanceData;
    balanceData["Balance"] = currentBalance.toDouble() + amount;

    if (!updateData("Users_Personal_Data", balanceData, searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to update Balance";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Log the transaction
    if (!logTransaction(accountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log Transaction";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for makeTransaction.");
        if (!rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for makeTransaction.");
        }
        responseJson["errorMessage"] = "Failed to commit Transaction";
        return responseJson;
    }

    responseJson["transactionSuccess"] = true;

    // Fetch the updated balance from the database
    responseJson["newBalance"] =
        fetchData("Users_Personal_Data", "Balance", searchCriteria).toDouble();
    return responseJson;
}

QJsonObject DatabaseManager::makeTransfer(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["transferSuccess"] = false;

    // Extract the necessary data from the request JSON
    qint64 fromAccountNumber = requestJson["fromAccountNumber"].toVariant().toLongLong();
    qint64 toAccountNumber = requestJson["toAccountNumber"].toVariant().toLongLong();
    double amount = requestJson["amount"].toDouble();

    // Check if the 'from' and 'to' account numbers are the same
    if (fromAccountNumber == toAccountNumber)
    {
        responseJson["errorMessage"] = "Cannot transfer money to yourself!";
        return responseJson;
    }

    // Prepare the search criteria
    QJsonObject fromSearchCriteria;
    fromSearchCriteria["AccountNumber"] = fromAccountNumber;
    QJsonObject toSearchCriteria;
    toSearchCriteria["AccountNumber"] = toAccountNumber;

    // Start a transaction
    if (!startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "failed";
        return responseJson;
    }

    // Fetch the 'from' account balance
    QVariant fromBalance = fetchData("Users_Personal_Data", "Balance", fromSearchCriteria);

    // Check if the 'from' account has sufficient balance
    if (fromBalance.toDouble() < amount)
    {
        responseJson["errorMessage"] = "Insufficient balance";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Fetch the 'to' account balance
    QVariant toBalance = fetchData("Users_Personal_Data", "Balance", toSearchCriteria);

    // Check if the 'to' account exists
    if (!toBalance.isValid())
    {
        responseJson["errorMessage"] = "The 'to' account does not exist";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Update the 'from' and 'to' account balances
    QJsonObject fromBalanceData, toBalanceData;
    fromBalanceData["Balance"] = fromBalance.toDouble() - amount;
    toBalanceData["Balance"] = toBalance.toDouble() + amount;

    if (!updateData("Users_Personal_Data", fromBalanceData, fromSearchCriteria)
        || !updateData("Users_Personal_Data", toBalanceData, toSearchCriteria))
    {
        responseJson["errorMessage"] = "Failed to update transaction rolling back";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Log the transfer in the Transaction_History table for both 'from' and 'to' accounts
    if (!logTransaction(fromAccountNumber, -amount) ||
        !logTransaction(toAccountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log transaction rolling back";
        rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for makeTransfer.");
        if (!rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for makeTransfer.");
        }
        responseJson["errorMessage"] = "Failed to rollback transaction for makeTransfer.";
        return responseJson;
    }

    responseJson["transferSuccess"] = true;

    // Fetch the updated 'from' and 'to' account balances from the database
    responseJson["newFromBalance"] =
        fetchData("Users_Personal_Data", "Balance", fromSearchCriteria).toDouble();
    responseJson["newToBalance"] =
        fetchData("Users_Personal_Data", "Balance", toSearchCriteria).toDouble();
    return responseJson;
}

bool DatabaseManager::logTransaction(qint64 accountNumber, double amount)
{
    // Get the current date and time
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDate = currentDateTime.toString("dd-MM-yyyy");
    QString formattedTime = currentDateTime.toString("hh:mm:ss");

    // Prepare the transaction data
    QJsonObject transactionData;
    transactionData["AccountNumber"] = accountNumber;
    transactionData["Date"] = formattedDate;
    transactionData["Time"] = formattedTime;
    transactionData["Amount"] = amount;

    // Log the transaction in the database
    bool success = insertData("Transaction_History", transactionData);
    return success;
}
