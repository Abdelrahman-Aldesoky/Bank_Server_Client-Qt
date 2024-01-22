#include "userwindow.h"
#include "ui_userwindow.h"

UserWindow::UserWindow(QWidget *parent, qint64 accountNumber, QSslSocket *socket)
    : QMainWindow(parent), ui(new Ui::UserWindow), socket(socket), accountNumber(accountNumber)
{
    ui->setupUi(this);
    setWindowTitle("User Window - Account Number: " + QString::number(accountNumber));
    this->setWindowIcon(QIcon("bank.jpg"));
    this->setWindowIconText("Admin");
    setAttribute(Qt::WA_DeleteOnClose);
    connect(socket, &QSslSocket::readyRead, this, &UserWindow::readyRead);
    qDebug() << "Constructed User Window.";
}

UserWindow::~UserWindow()
{
    delete ui;
    emit finished();
    qDebug() << "Destroyed User Window.";
}

void UserWindow::readyRead()
{
    // Read data from socket
    QByteArray responseData = socket->readAll();

    // Uncompress the response data
    responseData = qUncompress(responseData);

    // Try to parse the JSON document
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

    // Check if the response is a valid JSON object
    if (!jsonResponse.isObject())
    {
        qDebug() << "Invalid JSON response from the server.";
        return;
    }

    QJsonObject responseObject = jsonResponse.object();
    int responseId = responseObject["responseId"].toInt();

    switch (responseId)
    {
    case 1:
        break;
    case 2:
        handleViewAccountBalanceResponse(responseObject);
        break;
    case 6:
        handleMakeTransactionResponse(responseObject);
        break;
    case 7:
        handleMakeTransferResponse(responseObject);
        break;
    case 8:
        handleViewTransactionHistoryResponse(responseObject);
        break;
    default:
        qDebug() << "Unknown responseId ID: " << responseId;
        break;
    }
}

void UserWindow::on_pushButton_get_account_number_clicked()
{
    ui->label_account_number->setText("Account Number: " + QString::number(accountNumber));
}

void UserWindow::on_pbn_view_balance_clicked()
{
    // To ensure that iam connected to the server
    emit reconnectNeeded();

    ui->pbn_view_balance->setDisabled(true);
    // Request ID for View Account Balance
    quint8 requestId = 2;

    // Construct the request JSON object
    QJsonObject requestObject;
    requestObject["requestId"] = static_cast<int>(requestId);
    requestObject["accountNumber"] = accountNumber;

    // Convert the JSON object to a JSON document
    QJsonDocument jsonRequest(requestObject);

    // Send the request to the server
    socket->write(jsonRequest.toJson());
}

void UserWindow::handleViewAccountBalanceResponse(const QJsonObject &responseObject)
{
    ui->label_view_balance->setText("Balance: ");
    double accountBalance = responseObject["balance"].toDouble();
    bool viewBalanceSuccess = responseObject["accountFound"].toBool();

    if (viewBalanceSuccess)
    {
        ui->label_view_balance->setText("Balance: $" + QString::number(accountBalance));
        qDebug() << "View Account Balance successful. Balance: $" << accountBalance;
    }
    else
    {
        ui->label_view_balance->setText("Failed to view Account Balance!");
        qDebug() << "Failed to view Account Balance.";
    }
    ui->pbn_view_balance->setEnabled(true);
}

void UserWindow::on_pbn_make_trasnaction_clicked()
{
    // To ensure that iam connected to the server
    emit reconnectNeeded();

    ui->pbn_make_trasnaction->setDisabled(true);
    QString amountString = ui->lnedit_amount->text();
    bool conversionOk;
    double amount = amountString.toDouble(&conversionOk);

    // Validate the amount
    if (!conversionOk || amountString.isEmpty() || amount == 0)
    {
        // Display an error message in lbl_transaction_error
        ui->lbl_transaction_error->setText("Invalid amount entered. Please enter a valid number.");
        ui->pbn_make_trasnaction->setEnabled(true);
        return;
    }

    // Clear any previous error messages
    ui->lbl_transaction_error->clear();

    // Request ID for Make Transaction
    quint8 requestId = 6;

    // Create a JSON object for the transaction request
    QJsonObject transactionRequest;
    transactionRequest["requestId"] = static_cast<int>(requestId);
    transactionRequest["accountNumber"] = accountNumber;
    transactionRequest["amount"] = amount;

    // Convert the JSON object to a JSON document
    QJsonDocument jsonRequest(transactionRequest);

    // Send the request to the server
    socket->write(jsonRequest.toJson());
    socket->flush();
}

void UserWindow::handleMakeTransactionResponse(const QJsonObject &responseObject)
{
    ui->lbl_transaction_error->clear();
    double newBalance = responseObject["newBalance"].toDouble();
    bool transactionSuccess = responseObject["transactionSuccess"].toBool();

    if (transactionSuccess)
    {
        ui->lbl_transaction_error->setText("Transaction successful :)");
        qDebug() << "Transaction successful. New balance: $" << newBalance;
    }
    else
    {
        QString errorMessage = responseObject["errorMessage"].toString();
        ui->lbl_transaction_error->setText("Transaction failed: " + errorMessage);
        qDebug() << "Transaction failed: " << errorMessage;
    }
    ui->pbn_make_trasnaction->setEnabled(true);
}

void UserWindow::on_pbn_mk_transfer_clicked()
{
    // To ensure that iam connected to the server
    emit reconnectNeeded();

    ui->pbn_mk_transfer->setDisabled(true);
    // Get the account number to transfer to
    qint64 toAccountNumber = ui->lnedit_to_accountnumber->text().toLongLong();

    // Validate the toAccountNumber
    if (toAccountNumber <= 0)
    {
        ui->lbl_mk_trnsf_err->setText("Invalid 'To Account Number'. Please enter a valid number.");
        ui->pbn_mk_transfer->setEnabled(true);
        return;
    }

    // Check if the 'from' and 'to' account numbers are the same
    if (accountNumber == toAccountNumber)
    {
        ui->lbl_mk_trnsf_err->setText("You cannot transfer money to yourself!");
        ui->pbn_mk_transfer->setEnabled(true);
        return;
    }

    // Get the transfer amount
    QString amountString = ui->lnedit_trnsfr_amount->text();
    bool conversionOk;
    double amount = amountString.toDouble(&conversionOk);

    // Validate the transfer amount
    if (!conversionOk || amountString.isEmpty() || amount <= 0)
    {
        ui->lbl_mk_trnsf_err->setText("Invalid transfer amount. Please enter a valid positive number greater than zero.");
        ui->pbn_mk_transfer->setEnabled(true);
        return;
    }

    // Clear any previous error messages
    ui->lbl_mk_trnsf_err->clear();

    // Request ID for Make Transfer
    quint8 requestId = 7;

    // Create a JSON object for the transfer request
    QJsonObject transferRequest;
    transferRequest["requestId"] = static_cast<int>(requestId);
    transferRequest["fromAccountNumber"] = accountNumber;
    transferRequest["toAccountNumber"] = toAccountNumber;
    transferRequest["amount"] = amount;

    // Convert the JSON object to a JSON document
    QJsonDocument jsonRequest(transferRequest);

    // Send the request to the server
    socket->write(jsonRequest.toJson());
    socket->flush();
}

void UserWindow::handleMakeTransferResponse(const QJsonObject &responseObject)
{
    ui->lbl_mk_trnsf_err->clear();
    double newFromBalance = responseObject["newFromBalance"].toDouble();
    double newToBalance = responseObject["newToBalance"].toDouble();
    bool transferSuccess = responseObject["transferSuccess"].toBool();

    if (transferSuccess)
    {
        ui->lbl_mk_trnsf_err->setText("Transfer successful :)");
        qDebug() << "Transfer successful. New 'From' balance: $" << newFromBalance
                 << ". New 'To' balance: $" << newToBalance;
    }
    else
    {
        QString errorMessage = responseObject["errorMessage"].toString();
        ui->lbl_mk_trnsf_err->setText("Transfer failed: " + errorMessage);
        qDebug() << "Transfer failed: " << errorMessage;
    }
    ui->pbn_mk_transfer->setEnabled(true);
}

void UserWindow::on_pbn_view_transaction_histroy_clicked()
{
    // To ensure that iam connected to the server
    emit reconnectNeeded();

    // Disable the button upon clicking
    ui->pbn_view_transaction_histroy->setDisabled(true);

    // Request ID for View Transaction History
    quint8 requestId = 8;

    // Construct the request JSON object
    QJsonObject requestObject;
    requestObject["requestId"] = static_cast<int>(requestId);
    requestObject["accountNumber"] = accountNumber;

    // Convert the JSON object to a JSON document
    QJsonDocument jsonRequest(requestObject);

    // Send the request to the server
    socket->write(jsonRequest.toJson());
}

void UserWindow::handleViewTransactionHistoryResponse(const QJsonObject &responseObject)
{
    ui->lbl_err_transaction_history->clear();

    bool viewTransactionHistorySuccess = responseObject["viewTransactionHistorySuccess"].toBool();

    if (viewTransactionHistorySuccess)
    {
        // Clear existing data in tbl_transactionhistory
        ui->tbl_view_histroy_transaction->clearContents();
        ui->tbl_view_histroy_transaction->setRowCount(0);

        // Get the transaction history array from the response
        QJsonArray transactionHistoryArray = responseObject["transactionHistory"].toArray();

        qint32 maxRows = ui->lnedit_count_user->text().isEmpty() ? transactionHistoryArray.size() : ui->lnedit_count_user->text().toInt();

        // Populate tbl_transactionhistory with transaction history data
        int row = 0;
        for (const auto &transactionDataValue : transactionHistoryArray)
        {
            // Stop the loop if reached the maximum number of rows
            if (row >= maxRows)
                break;
            QJsonObject transactionData = transactionDataValue.toObject();

            ui->tbl_view_histroy_transaction->insertRow(row);
            ui->tbl_view_histroy_transaction->setItem(row, 0, new QTableWidgetItem(QString::number(transactionData["TransactionID"].toVariant().toLongLong())));
            ui->tbl_view_histroy_transaction->setItem(row, 1, new QTableWidgetItem(QString::number(transactionData["Amount"].toDouble())));
            ui->tbl_view_histroy_transaction->setItem(row, 2, new QTableWidgetItem(transactionData["Date"].toString()));
            ui->tbl_view_histroy_transaction->setItem(row, 3, new QTableWidgetItem(transactionData["Time"].toString()));

            row++;
        }

        qDebug() << "View Transaction History successful.";
    }
    else
    {
        ui->lbl_err_transaction_history->setText("Failed to view transaction history.");
        qDebug() << "Failed to view Transaction History.";
    }
    // enable the button
    ui->pbn_view_transaction_histroy->setEnabled(true);
}
