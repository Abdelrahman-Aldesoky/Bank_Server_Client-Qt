#include "transactionmanager.h"

TransactionManager::TransactionManager(DatabaseManager* databaseManager, QObject *parent)
    : QObject(parent), databaseManager(databaseManager), logger("TransactionManager")
{
    logger.log("TransactionManager Object Created.");
}

TransactionManager::~TransactionManager()
{
    logger.log("TransactionManager Object Destroyed");
}

QJsonObject TransactionManager::makeTransaction(QJsonObject requestJson)
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
    if (!databaseManager->startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "Failed to start transaction.";
        return responseJson;
    }

    // Fetch the current balance
    QVariant currentBalance = databaseManager->fetchData("Users_Personal_Data", "Balance",
                                        searchCriteria);

    // Check if the balance is sufficient
    if (currentBalance.toDouble() < 0 || currentBalance.toDouble() + amount < 0)
    {
        responseJson["errorMessage"] = "Insufficient balance";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Update the balance
    QJsonObject balanceData;
    balanceData["Balance"] = currentBalance.toDouble() + amount;

    if (!databaseManager->updateData("Users_Personal_Data", balanceData, searchCriteria))
    {
        responseJson["errorMessage"] = "Failed to update Balance";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Log the transaction
    if (!logTransaction(accountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log Transaction";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!databaseManager->commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for makeTransaction.");
        if (!databaseManager->rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for makeTransaction.");
        }
        responseJson["errorMessage"] = "Failed to commit Transaction";
        return responseJson;
    }

    responseJson["transactionSuccess"] = true;

    // Fetch the updated balance from the database
    responseJson["newBalance"] =
        databaseManager->fetchData("Users_Personal_Data", "Balance", searchCriteria).toDouble();
    return responseJson;
}

QJsonObject TransactionManager::makeTransfer(QJsonObject requestJson)
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
    if (!databaseManager->startDatabaseTransaction())
    {
        responseJson["errorMessage"] = "failed";
        return responseJson;
    }

    // Fetch the 'from' account balance
    QVariant fromBalance = databaseManager->fetchData("Users_Personal_Data", "Balance", fromSearchCriteria);

    // Check if the 'from' account has sufficient balance
    if (fromBalance.toDouble() < amount)
    {
        responseJson["errorMessage"] = "Insufficient balance";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Fetch the 'to' account balance
    QVariant toBalance = databaseManager->fetchData("Users_Personal_Data", "Balance", toSearchCriteria);

    // Check if the 'to' account exists
    if (!toBalance.isValid())
    {
        responseJson["errorMessage"] = "The 'to' account does not exist";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Update the 'from' and 'to' account balances
    QJsonObject fromBalanceData, toBalanceData;
    fromBalanceData["Balance"] = fromBalance.toDouble() - amount;
    toBalanceData["Balance"] = toBalance.toDouble() + amount;

    if (!databaseManager->updateData("Users_Personal_Data", fromBalanceData, fromSearchCriteria)
        || !databaseManager->updateData("Users_Personal_Data", toBalanceData, toSearchCriteria))
    {
        responseJson["errorMessage"] = "Failed to update transaction rolling back";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Log the transfer in the Transaction_History table for both 'from' and 'to' accounts
    if (!logTransaction(fromAccountNumber, -amount) ||
        !logTransaction(toAccountNumber, amount))
    {
        responseJson["errorMessage"] = "Failed to log transaction rolling back";
        databaseManager->rollbackDatabaseTransaction();
        return responseJson;
    }

    // Commit the transaction
    if (!databaseManager->commitDatabaseTransaction())
    {
        logger.log("Failed to commit transaction for makeTransfer.");
        if (!databaseManager->rollbackDatabaseTransaction())
        {
            logger.log("Failed to rollback transaction for makeTransfer.");
        }
        responseJson["errorMessage"] = "Failed to rollback transaction for makeTransfer.";
        return responseJson;
    }

    responseJson["transferSuccess"] = true;

    // Fetch the updated 'from' and 'to' account balances from the database
    responseJson["newFromBalance"] =
        databaseManager->fetchData("Users_Personal_Data", "Balance", fromSearchCriteria).toDouble();
    responseJson["newToBalance"] =
        databaseManager->fetchData("Users_Personal_Data", "Balance", toSearchCriteria).toDouble();
    return responseJson;

}

QJsonObject TransactionManager::viewTransactionHistory(QJsonObject requestJson)
{
    QJsonObject responseJson;
    responseJson["viewTransactionHistorySuccess"] = false;

    // Extract the account number from the request JSON
    qint64 accountNumber = requestJson["accountNumber"].toVariant().toLongLong();

    // Use the DatabaseManager pointer to get the QSqlDatabase object
    QSqlDatabase dbConnection = databaseManager->getDatabase();
    QSqlQuery transactionHistoryQuery(dbConnection);
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

bool TransactionManager::logTransaction(qint64 accountNumber, double amount)
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
    bool success = databaseManager->insertData("Transaction_History", transactionData);
    return success;
}
