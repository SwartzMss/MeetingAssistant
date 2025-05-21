#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QDateTime>
#include <windows.h>
#include <dbghelp.h>
#include <QDebug>
#include "mainwindow.h"
#include "logger.h"

// 设置崩溃转储文件的保存路径
QString getDumpFilePath() {
    QString dumpDir = QCoreApplication::applicationDirPath() + "/dumps";
    QDir().mkpath(dumpDir);
    return dumpDir + "/crash_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".dmp";
}

// 崩溃处理函数
LONG WINAPI TopLevelExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    static bool isHandling = false;
    if (isHandling) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    isHandling = true;

    try {
        QString dumpPath = getDumpFilePath();
        HANDLE hFile = CreateFileW(
            dumpPath.toStdWString().c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION exInfo;
            exInfo.ExceptionPointers = pExceptionInfo;
            exInfo.ThreadId = GetCurrentThreadId();
            exInfo.ClientPointers = TRUE;

            // 创建完整的内存转储
            MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithFullMemory | MiniDumpWithHandleData),
                &exInfo,
                NULL,
                NULL
            );

            CloseHandle(hFile);

            // 记录崩溃信息到日志
            Logger logger;
            logger.logError(QString("程序崩溃，转储文件已保存到: %1").arg(dumpPath));
            logger.logError(QString("异常代码: 0x%1").arg(pExceptionInfo->ExceptionRecord->ExceptionCode, 8, 16, QChar('0')));
            logger.logError(QString("异常地址: 0x%1").arg((quintptr)pExceptionInfo->ExceptionRecord->ExceptionAddress, 8, 16, QChar('0')));
        }
    }
    catch (...) {
        // 如果转储过程中发生异常，至少记录一下
        Logger logger;
        logger.logError("程序崩溃，但无法创建转储文件");
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char *argv[])
{
    // 设置异常处理
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);
    
    // 设置应用程序信息
    QCoreApplication::setOrganizationName("MeetingAssistant");
    QCoreApplication::setApplicationName("MeetingAssistant");
    
    // 创建日志目录
    QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    QDir().mkpath(logDir);
    
    MainWindow window;
    window.show();
    
    return app.exec();
} 