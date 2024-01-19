# Banking Client-Server Application

## Overview
This application is a banking system implemented as a client-server model using Qt. The server handles incoming connections, processes client requests, and interacts with a database. The client provides interfaces for both regular users and administrators.

## Key Features
- **Multithreading**: Each client gets its own thread and connection to the database. This allows the server to handle multiple clients simultaneously, making it more efficient and faster.
- **Communication and Secure Connection**: The application uses JSON format for the communication between the server and client. Each request and response pair has a unique ID. The application also uses SSL/TLS to encrypt the connection between the client and server. This ensures that the data exchanged is protected from eavesdropping, tampering, and forgery.
- **Server**: The server handles various banking operations such as login, account creation, balance viewing, transaction history, etc. It uses the `RequestHandler` class to process different types of requests from clients and the `DatabaseManager` class to manage all database-related operations.
- **Client**: The client provides separate interfaces for administrators and regular users. The `AdminWindow` class allows administrators to view account numbers, balances, transaction history, and database information, and also create and delete accounts. The `UserWindow` class allows regular users to view their account number, balance, and transaction history, and also make transactions and transfers.
- **Database**: The application uses SQLite for database management. The database consists of three separate tables: `Accounts`, `Users_Personal_Data`, and `Transaction_History`.
- **Backup Feature**: Implements routine backups every 6 hours, each uniquely timestamped. Ensures efficient storage management by automatically deleting backups older than a month. Prioritizes data integrity by initiating backup before server closure on SIGTERM, SIGINT, and SIGHUP on (Linux) can be enabled. 
This feature enhances data safety while maintaining an efficient storage system.

## Database Structure
- **Accounts table**: Stores account number (primary key), username, password, and admin status. A default admin account is created during table creation.
- **Users_Personal_Data table**: Stores account number (primary key and foreign key referencing `Accounts` account number), name, age, and balance.
- **Transaction_History table**: Stores transaction ID (primary key), account number (foreign key referencing `Accounts` account number), date, time, and amount.

## Usage
To use this application, start the server application first. Then, start the client application and connect to the server on localhost. There is a default account with username `admin` and password `admin`.

## Installation
To install this application, follow these steps:

1. Extract the files located in the "Executables" folder on a Windows machine.
2. Install OpenSSL on your machine. You can download it from [here](https://www.openssl.org/source/).
3. Generate your own private key and self-signed certificate using OpenSSL. This only applies to the secure version.

## Setting Up your key and certificate Instructions

1. Open your command prompt and run the following commands:

    ```bash
    openssl genrsa -out server.key 2048
    openssl req -new -x509 -key server.key -out server.crt -days 365
    ```

2. After running these commands:
    - Double-click the certificate file (`server.crt`) and install it on your machine. Follow these steps to trust the certificate authority:
        - Click on the **Install Certificate** button.
        - Select **Local Machine** as the Store Location and click **Next**.
        - Select **Place all certificates in the following store** and click **Browse**.
        - Check the **Show physical stores** box and expand **Trusted Root Certification Authorities**. Select **Local Computer** under it and click **OK**.
        - Click **Next** and then **Finish** to complete the wizard. You may see a security warning asking you to confirm that you want to install the certificate. Click **Yes** to proceed.
        - You should see a message saying that the import was successful. Click **OK** to close the dialog box.
    - Rename the private key and certificate files as `server.key` and `server.crt` respectively and place them in the same folder as the server executable file.
    - Run the `server.exe` file and the `client.exe` file.

## Future Improvements

- **Encrypted Database Operations**: Use encrypted database operations to protect sensitive data.

