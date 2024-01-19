#include "client.h"
#include "ui_client.h"
#include "adminwindow.h"
#include "userwindow.h"

// Regular expressions for username and password validation
const QRegularExpression client::usernameRegex("^[a-zA-Z0-9_]*$");
const QRegularExpression client::passwordRegex("\\s");

client::client(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::client),
      socket(new QSslSocket(this))
{
    // Setup the UI
    ui->setupUi(this);
    // Set the window elements
    setWindowTitle("Login Window");
    this->setWindowIcon(QIcon("bank.jpg"));

    // Set the protocol to TLS 1.2
    socket->setProtocol(QSsl::TlsV1_2OrLater);

    // Load the certificate from the file
    QFile certFile(QStringLiteral("server.crt"));
    certFile.open(QIODevice::ReadOnly);
    QSslCertificate cert(&certFile, QSsl::Pem);
    certFile.close();

    // Add the certificate to the socket's CA certificates
    QList<QSslCertificate> caCerts = socket->sslConfiguration().caCertificates();
    caCerts.append(cert);
    QSslConfiguration sslConfig = socket->sslConfiguration();
    sslConfig.setCaCertificates(caCerts);
    socket->setSslConfiguration(sslConfig);

    // handle state changed for connection
    connect(socket, &QSslSocket::stateChanged, this, &client::handleStateChanged);

    // Connect the readyRead signal to the readyRead slot
    connect(socket, &QSslSocket::readyRead, this, &client::readyRead);

    // Connect the sslErrors signal to the handleSslErrors slot
    connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &client::handleSslErrors);

    // attempt connecting to the server
    connectToServer();
}

// Destructor
client::~client()
{
    // Flush the socket
    socket->flush();
    socket->disconnect();
    // Delete socket
    delete socket;
    // Delete the UI
    delete ui;
}

void client::connectToServer()
{
    // Get the IP address from the line edit
    QString ipAddress = ui->lineEdit_ip->text();

    // Attempt to connect to the server encrypted
    socket->connectToHostEncrypted(ipAddress, 19908);
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

void client::handleSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qDebug() << "SSL error: " << error.errorString();
    }
}

void client::on_pbn_connect_clicked()
{
    connectToServer();
}

// Slot for handling incoming data from the server
void client::readyRead()
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
            disconnect(socket, &QSslSocket::readyRead, this, &client::readyRead);
            AdminWindow *adminWindow = new AdminWindow(nullptr, accountNumber, socket);
            // Connect the finished signal in the AdminWindow object to showOldWindow in the client object
            connect(adminWindow, &AdminWindow::finished, this, &client::showOldWindow);
            qDebug() << adminWindow;
            adminWindow->show();
        }
        else
        {
            // Disconnect the readyRead signal from the client slot
            disconnect(socket, &QSslSocket::readyRead, this, &client::readyRead);
            UserWindow *userWindow = new UserWindow(nullptr, accountNumber, socket);
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
    connect(socket, &QSslSocket::readyRead, this, &client::readyRead);
    // Clear the login window
    ui->lineEdit_Username->clear();
    ui->lineEdit_Password->clear();

    // Get the sender object
    QObject *senderObject = sender();
    qDebug() << senderObject;

    // Show the login window
    this->show();
}
