#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>

namespace Ui
{
class UserWindow;
}

class UserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UserWindow(QWidget *parent = nullptr, qint64 accountNumber = 0);
    ~UserWindow();

signals:
    void finished();

private:
    Ui::UserWindow *ui;
    qint64 accountNumber;
};

#endif // USERWINDOW_H
