#include "accountmanager.h"

AccountManager::AccountManager(DatabaseManager* databaseManager, QObject *parent)
    : QObject(parent), databaseManager(databaseManager), logger("AccountManager")
{
    logger.log("AccountManager Object Created.");
}

AccountManager::~AccountManager()
{
    logger.log("AccountManager Object Destroyed");
}

QJsonObject AccountManager::login(QJsonObject requestJson)
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
    QVariant accountNumberData = databaseManager->fetchData("Accounts", "AccountNumber",
                                           searchCriteria);
    QVariant adminData = databaseManager->fetchData("Accounts", "Admin", searchCriteria);

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

QJsonObject AccountManager::getAccountNumber(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["userFound"] = false;

    // Extract the username from the request JSON
    QString username = requestJson["username"].toString();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["Username"] = username;

    // Fetch the account number
    QVariant accountNumber = databaseManager->fetchData("Accounts", "AccountNumber",
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

QJsonObject AccountManager::getAccountBalance(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["accountFound"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["AccountNumber"] = accountNumber;

    // Fetch the account balance
    QVariant balance = databaseManager->fetchData("Users_Personal_Data", "Balance", searchCriteria);

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

QJsonObject AccountManager::createNewAccount(QJsonObject requestJson)
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
    QVariant usernameExists = databaseManager->fetchData("Accounts", "COUNT(*)", searchCriteria);

    if (!usernameExists.isNull() && usernameExists.toInt() > 0)
    {
        responseJson["errorMessage"] = "Username already exists.";
        logger.log("Username already exists.");
        return responseJson;
    }

    // Start a transaction
    if (!databaseManager->startDatabaseTransaction())
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

    qint64 accountNumber = databaseManager->insertData("Accounts", accountData);
    if (!accountNumber)
    {
        responseJson["errorMessage"] = "Failed to insert new account";
        logger.log("Failed to insert new account.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Insert the personal data into the Users_Personal_Data table
    QJsonObject personalData;
    personalData["AccountNumber"] = accountNumber;
    personalData["Name"] = name;
    personalData["Age"] = age;
    personalData["Balance"] = balance;

    if (!databaseManager->insertData("Users_Personal_Data", personalData))
    {
        responseJson["errorMessage"] = "Failed to insert personal data.";
        logger.log("Failed to insert personal data.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!databaseManager->commitDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to commit transaction for createNewAccount.";
        logger.log("Failed to commit transaction for createNewAccount.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    responseJson["createAccountSuccess"] = true;
    responseJson["accountNumber"] = static_cast<qint64>(accountNumber);
    return responseJson;
}

QJsonObject AccountManager::deleteAccount(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["deleteAccountSuccess"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    // Prepare the search criteria
    QJsonObject searchCriteria;
    searchCriteria["AccountNumber"] = accountNumber;

    // Start a transaction
    if (!databaseManager->startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start a transaction for deleteAccount.";
        logger.log("Failed to start a transaction for deleteAccount.");
        return responseJson;
    }

    // Delete the account from the Accounts table
    if (!databaseManager->removeData("Accounts", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete account from Accounts table.";
        logger.log("Failed to delete account from Accounts table.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Delete the personal data from the Users_Personal_Data table
    if (!databaseManager->removeData("Users_Personal_Data", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete account from Users_Personal_Data table.";
        logger.log("Failed to delete account from Users_Personal_Data table.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Delete the transaction history from the Transaction_History table
    if (!databaseManager->removeData("Transaction_History", searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to delete transaction history for the account.";
        logger.log("Failed to delete transaction history for the account.");
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!databaseManager->commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for deleteAccount.");
        if (!databaseManager->rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for deleteAccount.");
        }
        responseJson["errorMessage"] = "failed";
        return responseJson;
    }

    responseJson["deleteAccountSuccess"] = true;
    return responseJson;
}

QJsonObject AccountManager::updateUserData(QJsonObject requestJson)
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
    if (!databaseManager->startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start a transaction for updateUserData.";
        logger.log("Failed to start a transaction for updateUserData.");
        return responseJson;
    }

    // Fetch the account number
    QVariant accountNumberVariant = databaseManager->fetchData("Accounts", "AccountNumber", searchCriteria);
    if (!accountNumberVariant.isValid())
    {
        responseJson["errorMessage"] = "Account not found";
        logger.log("Account not found.");
        databaseManager->rollbackDatabaseTransaction();
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

        if (!databaseManager->updateData("Accounts", passwordData, searchCriteria))
        {
            responseJson["errorMessage"] = "failed to update password";
            logger.log("Failed to update password.");
            databaseManager->rollbackDatabaseTransaction();
            return responseJson;
        }
    }

    // Update the name in the Users_Personal_Data table
    if (!name.isEmpty())
    {
        QJsonObject nameData;
        nameData["Name"] = name;

        if (!databaseManager->updateData("Users_Personal_Data", nameData, updateSearchCriteria))
        {
            responseJson["errorMessage"] = "failed to update name";
            logger.log("Failed to update name.");
            databaseManager->rollbackDatabaseTransaction();
            return responseJson;
        }
    }

    // Commit the transaction
    if (!databaseManager->commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for updateUserData.");
        if (!databaseManager->rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for updateUserData.");
        }
        responseJson["errorMessage"] = "Failed to commit transaction.";
        return responseJson;
    }

    responseJson["updateSuccess"] = true;
    return responseJson;
}

QJsonObject AccountManager::viewDatabase()
{
    QJsonObject responseJson;
    responseJson["fetchUserDataSuccess"] = false;

    // Using the DatabaseManager pointer to get the QSqlDatabase object
    QSqlDatabase dbConnection = databaseManager->getDatabase();
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
