#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QDebug>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    static void log(const QString &message, const char* file = nullptr, int line = 0);
    static void logError(const QString &message, const char* file = nullptr, int line = 0);
    static QString getLogPath();

private:
    static QString formatLogMessage(const QString &message, const char* file = nullptr, int line = 0);
    static QFile logFile;
    static QTextStream logStream;
    static bool isInitialized;
};

// 定义日志宏
#define LOG_INFO(msg) Logger::log(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::logError(msg, __FILE__, __LINE__)

#endif // LOGGER_H 