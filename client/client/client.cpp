#include "client.h"
#include "ui_client.h"
#include "adminwindow.h"
#include "userwindow.h"

// Regular expressions for username and password validation
const QRegularExpression client::usernameRegex("^[a-zA-Z0-9_]*$");
const QRegularExpression client::passwordRegex("\\s");

// Constructor
client::client(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::client),
    socket(new QTcpSocket(this))
{
    // Setup the UI
    ui->setupUi(this);
    // Set the window elements
    setWindowTitle("Login Window");

    //handle state changed for connection
    connect(socket, &QTcpSocket::stateChanged, this, &client::handleStateChanged);

    //attempt connecting to the server
    connectToServer();

    // Connect the readyRead signal to the readyRead slot
    connect(socket, &QTcpSocket::readyRead, this, &client::readyRead);
}

// Destructor
client::~client()
{
    // Flush the socket
    socket->flush();
    // Delete the UI
    delete ui;
}

void client::connectToServer()
{
    // Attempt to connect to the server
    socket->connectToHost("localhost", 54321);
}

void client::handleStateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState == QAbstractSocket::ConnectedState)
    {
        // The socket is connected
        ui->chkbox->setText("Connected");
        ui->chkbox->setChecked(true);
        ui->pbn_connect->setEnabled(false);
        ui->chkbox->setEnabled(false);
    }
    else
    {
        // The socket is not connected
        ui->chkbox->setText("Not connected");
        ui->chkbox->setChecked(false);
        ui->pbn_connect->setEnabled(true);
        ui->chkbox->setEnabled(false);
    }
}

void client::on_pbn_connect_clicked()
{
    connectToServer();
}


// Slot for handling incoming data from the server
void client::readyRead()
{
    // Read the response from the server
    QByteArray responseData = socket->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

    // Check if the response is a valid JSON object
    if (!jsonResponse.isObject())
    {
        qDebug() << "Invalid JSON response from the server.";
        return;
    }

    // Handle the response based on the request ID
    QJsonObject responseObject = jsonResponse.object();
    int responseId = responseObject["responseId"].toInt();

    switch (responseId)
    {
    case 0:
        handleLoginResponse(responseObject);
        break;
    default:
        qDebug() << "Unknown responseId ID: " << responseId;
        break;
    }
   // qDebug() << responseData;
}

// Slot for handling login button click
void client::on_pushButton_login_clicked()
{
    // Request ID for login
    quint8 requestId = 0;
    // Get the username and password from the UI
    QString username = ui->lineEdit_Username->text();
    QString password = ui->lineEdit_Password->text();

    // Check if Username and Password fields are not empty
    if (username.isEmpty() || password.isEmpty())
    {
        qDebug() << "Warning: Please fill in the username and password.";
        QMessageBox::warning(nullptr, "Warning", "Please fill in the username and password.", QMessageBox::Ok);
        return;
    }

    // Check if the Username field contains only alphanumeric characters and underscores
    if (!username.contains(usernameRegex))
    {
        qDebug() << "Warning: Username must only contain alphanumeric characters.";
        QMessageBox::warning(nullptr, "Warning", "Username must only contain alphanumeric characters.", QMessageBox::Ok);
        return;
    }

    // Check if the password contains spaces
    if (password.contains(passwordRegex))
    {
        qDebug() << "Warning: Password must not contain spaces.";
        QMessageBox::warning(nullptr, "Warning", "Password must not contain spaces.", QMessageBox::Ok);
        return;
    }

    // Construct the request JSON object
    QJsonObject requestObject;
    requestObject["requestId"] = static_cast<int>(requestId);
    requestObject["username"] = username;
    requestObject["password"] = password;

    // Convert the JSON object to a JSON document
    QJsonDocument jsonRequest(requestObject);

    // Send the request to the server
    socket->write(jsonRequest.toJson());
}

// Function to handle login response from the server
void client::handleLoginResponse(const QJsonObject &responseObject)
{
    // Extract information from the response
    bool loginSuccess = responseObject["loginSuccess"].toBool();
    qint64 accountNumber = responseObject["accountNumber"].toVariant().toLongLong();
    bool isAdmin = responseObject["isAdmin"].toBool();

    // Check if login is successful
    if (loginSuccess)
    {
        // Create and show the appropriate window based on user type
        if (isAdmin)
        {
            // Disconnect the readyRead signal from the client slot
            disconnect(socket, &QTcpSocket::readyRead, this, &client::readyRead);
            AdminWindow *adminWindow = new AdminWindow(this, accountNumber, socket);
            // Connect the finished signal in the AdminWindow object to showOldWindow in the client object
            connect(adminWindow, &AdminWindow::finished, this, &client::showOldWindow);
            qDebug() << adminWindow;
            adminWindow->show();
        }
        else
        {
            // Disconnect the readyRead signal from the client slot
            disconnect(socket, &QTcpSocket::readyRead, this, &client::readyRead);
            UserWindow *userWindow = new UserWindow(this, accountNumber, socket);
            // Connect the finished signal in the UserWindow object to showOldWindow in the client object
            connect(userWindow, &UserWindow::finished, this, &client::showOldWindow);
            qDebug() << userWindow;
            userWindow->show();
        }
        this->hide();
    }
    else
    {
        // Display a login failed message
        QMessageBox::warning(this, "Login Failed", "Invalid username or password. Please try again.", QMessageBox::Ok);
    }
}

void client::showOldWindow()
{
    connect(socket, &QTcpSocket::readyRead, this, &client::readyRead);
    // Clear the login window
    ui->lineEdit_Username->clear();
    ui->lineEdit_Password->clear();

    // Get the sender object
    QObject *senderObject = sender();
    qDebug() << senderObject;

    // Show the login window
    this->show();
}
