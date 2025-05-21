#include "logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QThread>
#include <QDateTime>
#include <QDebug>

QFile Logger::logFile;
QTextStream Logger::logStream;
bool Logger::isInitialized = false;

Logger::Logger(QObject *parent)
    : QObject(parent)
{
    if (!isInitialized) {
        QString logPath = getLogPath();
        QDir().mkpath(QFileInfo(logPath).path());
        
        logFile.setFileName(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            logStream.setDevice(&logFile);
            isInitialized = true;
        }
    }
}

Logger::~Logger()
{
    if (logFile.isOpen()) {
        logStream.flush();
        logFile.close();
    }
}

QString Logger::getLogPath()
{
    return QCoreApplication::applicationDirPath() + "/logs/meeting_assistant.log";
}

QString Logger::formatLogMessage(const QString &message, const char* file, int line)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString threadId = QString::number((quintptr)QThread::currentThreadId());
    QString fileName = file ? QFileInfo(file).fileName() : "unknown";
    QString lineNumber = line > 0 ? QString::number(line) : "0";
    
    return QString("[%1][Thread-%2][%3:%4] %5")
            .arg(timestamp)
            .arg(threadId)
            .arg(fileName)
            .arg(lineNumber)
            .arg(message);
}

void Logger::log(const QString &message, const char* file, int line)
{
    if (!isInitialized) {
        return;
    }
    
    QString formattedMessage = formatLogMessage(message, file, line);
    logStream << formattedMessage << Qt::endl;
    logStream.flush();
    
    // 同时输出到控制台
    qDebug().noquote() << formattedMessage;
}

void Logger::logError(const QString &message, const char* file, int line)
{
    if (!isInitialized) {
        return;
    }
    
    QString formattedMessage = formatLogMessage("ERROR: " + message, file, line);
    logStream << formattedMessage << Qt::endl;
    logStream.flush();
    
    // 同时输出到控制台
    qDebug().noquote() << formattedMessage;
} 