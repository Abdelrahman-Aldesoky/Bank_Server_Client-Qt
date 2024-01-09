#include "bank_client.h"
#include "ui_bank_client.h"
#include "adminwindow.h"
#include "userwindow.h"

// Regular expressions for username and password validation
const QRegularExpression bank_client::usernameRegex("^[a-zA-Z0-9_]*$");
const QRegularExpression bank_client::passwordRegex("\\s");

// Constructor
bank_client::bank_client(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::bank_client),
    socket(new QTcpSocket(this))
{
    // Setup the UI
    ui->setupUi(this);
    // Set the window title
    setWindowTitle("Login Window");

    // Connect to the server
    socket->connectToHost("localhost", 54321);
    // Connect the readyRead signal to the readyRead slot
    connect(socket, &QTcpSocket::readyRead, this, &bank_client::readyRead);
}

// Destructor
bank_client::~bank_client()
{
    // Flush the socket
    socket->flush();
    // Delete the UI
    delete ui;
}

// Slot for handling incoming data from the server
void bank_client::readyRead()
{
    // Read the response from the server
    QString responseText = socket->readAll();
    // Display the response in a label
    ui->label_view_server->setText(responseText);

    // Split the response into parts
    QStringList responseParts = responseText.split('|');

    // Check the response parts
    if (responseParts.size() == 3)
    {
        // Handle the login response
        handleLoginResponse(responseParts);
    }
    // Handle invalid response size or format if needed
}

// Slot for handling login button click
void bank_client::on_pushButton_login_clicked()
{
    // Request ID for login
    quint8 request_id = 1;
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

    // Construct the request text
    QString requestText = QStringLiteral("%1|%2|%3").arg(request_id).arg(username).arg(password);

    // Send the request to the server
    socket->write(requestText.toUtf8());
}

// Function to handle login response from the server
void bank_client::handleLoginResponse(const QStringList &responseParts)
{
    // Extract information from the response
    bool loginSuccess = responseParts[0].toInt();
    qint64 accountNumber = responseParts[1].toLongLong();
    bool isAdmin = responseParts[2].toInt();

    // Check if login is successful
    if (loginSuccess)
    {
        // Create and show the appropriate window based on user type
        if (isAdmin)
        {
            AdminWindow *adminWindow = new AdminWindow(this, accountNumber);
            // Connect the finished signal in the AdminWindow object to showOldWindow in the bank_client object
            connect(adminWindow, &AdminWindow::finished, this, &bank_client::showOldWindow);
            qDebug() << adminWindow;
            adminWindow->show();
        }
        else
        {
            UserWindow *userWindow = new UserWindow(this, accountNumber);
            // Connect the finished signal in the UserWindow object to showOldWindow in the bank_client object
            connect(userWindow, &UserWindow::finished, this, &bank_client::showOldWindow);
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

void bank_client::showOldWindow()
{
    // Clear the login window
    ui->lineEdit_Username->clear();
    ui->lineEdit_Password->clear();
    ui->label_view_server->clear();

    // Get the sender object
    QObject *senderObject = sender();
    qDebug() << senderObject;

    // Show the login window
    this->show();
}
