#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>

namespace Ui
{
class AdminWindow;
}

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminWindow(QWidget *parent = nullptr, qint64 accountNumber = 0);
    ~AdminWindow();

signals:
    void finished();

private:
    Ui::AdminWindow *ui;
    qint64 accountNumber;

};

#endif // ADMINWINDOW_H
