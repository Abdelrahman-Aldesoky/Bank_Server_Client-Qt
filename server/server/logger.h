#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class Logger : public QObject
{
    Q_OBJECT

public:
    Logger(const QString &tag, QObject *parent = nullptr);
    ~Logger();

    void log(const QString &message);

private:
    QFile logFile;
    QTextStream textStream;
    QString logTag;
};

#endif // LOGGER_H
