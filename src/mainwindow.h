#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioInput>
#include <QAudioOutput>
#include <QBuffer>
#include <QTimer>
#include <QLabel>
#include <QTextEdit>
#include "audioprocessor.h"
#include "azurespeechapi.h"
#include "logger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onAudioDataReceived(const QByteArray &data);
    void onRecognitionResult(const QString &text);
    void onTranslationResult(const QString &text);
    void onError(const QString &message);
    void onStatusChanged(const QString &status);
    void onTestButtonClicked();
    void onSaveConfigClicked();
    void loadConfig();
    void onFinalRecognitionResult(const QString &text);
    void onFinalTranslationResult(const QString &text);
    void onClearButtonClicked();

private:
    Ui::MainWindow *ui;
    AudioProcessor *audioProcessor;
    AzureSpeechAPI *azureSpeechAPI;
    Logger *logger;
    QString configFilePath;
    QString recognitionHistory;
    QString translationHistory;
    QTextEdit *historyChineseText;
};

#endif // MAINWINDOW_H 