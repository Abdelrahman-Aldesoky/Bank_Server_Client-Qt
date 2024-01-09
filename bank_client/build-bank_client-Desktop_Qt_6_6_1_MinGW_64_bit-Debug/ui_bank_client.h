/********************************************************************************
** Form generated from reading UI file 'bank_client.ui'
**
** Created by: Qt User Interface Compiler version 6.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BANK_CLIENT_H
#define UI_BANK_CLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_bank_client
{
public:
    QWidget *centralwidget;
    QWidget *widget;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label_Username;
    QLineEdit *lineEdit_Username;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_Password;
    QLineEdit *lineEdit_Password;
    QPushButton *pushButton_login;
    QFormLayout *formLayout;
    QLabel *label_response;
    QLabel *label_view_server;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *bank_client)
    {
        if (bank_client->objectName().isEmpty())
            bank_client->setObjectName("bank_client");
        bank_client->resize(295, 173);
        centralwidget = new QWidget(bank_client);
        centralwidget->setObjectName("centralwidget");
        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(3, 10, 281, 116));
        verticalLayout_2 = new QVBoxLayout(widget);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_Username = new QLabel(widget);
        label_Username->setObjectName("label_Username");

        horizontalLayout->addWidget(label_Username);

        lineEdit_Username = new QLineEdit(widget);
        lineEdit_Username->setObjectName("lineEdit_Username");

        horizontalLayout->addWidget(lineEdit_Username);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_Password = new QLabel(widget);
        label_Password->setObjectName("label_Password");

        horizontalLayout_2->addWidget(label_Password);

        lineEdit_Password = new QLineEdit(widget);
        lineEdit_Password->setObjectName("lineEdit_Password");

        horizontalLayout_2->addWidget(lineEdit_Password);


        verticalLayout->addLayout(horizontalLayout_2);

        pushButton_login = new QPushButton(widget);
        pushButton_login->setObjectName("pushButton_login");

        verticalLayout->addWidget(pushButton_login);


        verticalLayout_2->addLayout(verticalLayout);

        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        label_response = new QLabel(widget);
        label_response->setObjectName("label_response");

        formLayout->setWidget(0, QFormLayout::LabelRole, label_response);

        label_view_server = new QLabel(widget);
        label_view_server->setObjectName("label_view_server");

        formLayout->setWidget(0, QFormLayout::FieldRole, label_view_server);


        verticalLayout_2->addLayout(formLayout);

        bank_client->setCentralWidget(centralwidget);
        menubar = new QMenuBar(bank_client);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 295, 21));
        bank_client->setMenuBar(menubar);
        statusbar = new QStatusBar(bank_client);
        statusbar->setObjectName("statusbar");
        bank_client->setStatusBar(statusbar);

        retranslateUi(bank_client);

        QMetaObject::connectSlotsByName(bank_client);
    } // setupUi

    void retranslateUi(QMainWindow *bank_client)
    {
        bank_client->setWindowTitle(QCoreApplication::translate("bank_client", "bank_client", nullptr));
        label_Username->setText(QCoreApplication::translate("bank_client", "Username:", nullptr));
        label_Password->setText(QCoreApplication::translate("bank_client", "Password:", nullptr));
        pushButton_login->setText(QCoreApplication::translate("bank_client", "Login", nullptr));
        label_response->setText(QCoreApplication::translate("bank_client", "Response:", nullptr));
        label_view_server->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class bank_client: public Ui_bank_client {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BANK_CLIENT_H
