#include "Logger.h"

Logger::Logger(const QString &tag, QObject *parent)
    : QObject(parent), logTag(tag)
{
    QString logFilePath = "common_log.txt";

    logFile.setFileName(logFilePath);

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        textStream.setDevice(&logFile);
    }
    else
    {
        qDebug() << "Failed to open log file:" << logFilePath;
    }
}

Logger::~Logger()
{
    logFile.close();
}

void Logger::log(const QString &message)
{
    QString formattedMessage = QString("%1: %2").arg(logTag, message);
    textStream << formattedMessage << '\n';
    textStream.flush();

    // Also print the log message to the console for debugging
    qDebug() << formattedMessage;
}
