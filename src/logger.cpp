#include "logger.h"

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger(QObject *parent) : QObject(parent)
{
    QString logPath = getLogPath();
    logFile.setFileName(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream.setDevice(&logFile);
    }
}

Logger::~Logger()
{
    if (logFile.isOpen()) {
        logFile.close();
    }
}

void Logger::log(const QString& message, const QString& level)
{
    QMutexLocker locker(&mutex);
    
    if (logFile.isOpen()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        logStream << QString("[%1] [%2] %3\n")
                     .arg(timestamp)
                     .arg(level)
                     .arg(message);
        logStream.flush();
    }
}

QString Logger::getLogPath() const
{
    QString exePath = QCoreApplication::applicationDirPath();
    return exePath + "/MeetingAssistant.log";
} 