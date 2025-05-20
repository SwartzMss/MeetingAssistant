#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QMutex>

class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger& instance();
    void log(const QString& message, const QString& level = "INFO");

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QFile logFile;
    QTextStream logStream;
    QMutex mutex;
    QString getLogPath() const;
};

#endif // LOGGER_H 