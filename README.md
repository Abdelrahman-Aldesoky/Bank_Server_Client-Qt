# Banking Client-Server Application

## Overview
This application is a banking system implemented as a client-server model using Qt. The server handles incoming connections, processes client requests, and interacts with a database. The client provides interfaces for both regular users and administrators.

## Key Features
- **Multithreading**: Each client gets its own thread and connection to the database. This allows the server to handle multiple clients simultaneously, making it more efficient and faster.
- **Server**: The server handles various banking operations such as login, account creation, balance viewing, transaction history, etc. It uses the `RequestHandler` class to process different types of requests from clients and the `DatabaseManager` class to manage all database-related operations.
- **Client**: The client provides separate interfaces for administrators and regular users. The `AdminWindow` class allows administrators to view account numbers, balances, transaction history, and database information, and also create and delete accounts. The `UserWindow` class allows regular users to view their account number, balance, and transaction history, and also make transactions and transfers.
- **Communication**: The application uses JSON format for the communication between the server and client. Each request and response pair has a unique ID.
- **Database**: The application uses SQLite for database management. The database consists of three separate tables: `Accounts`, `Users_Personal_Data`, and `Transaction_History`.

## Database Structure
- **Accounts table**: Stores account number (primary key), username, password, and admin status. A default admin account is created during table creation.
- **Users_Personal_Data table**: Stores account number (primary key and foreign key referencing `Accounts` account number), name, age, and balance.
- **Transaction_History table**: Stores transaction ID (primary key), account number (foreign key referencing `Accounts` account number), date, time, and amount.

## Usage
To use this application, start the server application first. Then, start the client application and connect to the server on localhost. There is a default account with username `admin` and password `admin`.

## Installation
Extract the files on a Windows machine and run the .exe file. The application has been deployed using winqtdeploy, so it should run without needing any additional dependencies.

## Future Improvements
- **Secure Connection**: Implement encryption for the connection between the client and server to enhance security.
- **Encrypted Database Operations**: Use encrypted database operations to protect sensitive data.
