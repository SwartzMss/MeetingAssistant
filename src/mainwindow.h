#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QByteArray>
#include "audioprocessor.h"
#include "baiduapi.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopClicked();
    void onClearClicked();
    void onSaveConfigClicked();
    void onTestApiClicked();
    void onAudioDataReceived(const QByteArray &data);
    void onRecognitionResult(const QString &text);
    void onTranslationResult(const QString &text);
    void onApiError(const QString &message);
    void onApiTestResult(bool success, const QString &message);

private:
    void setupUI();
    void loadConfig();
    void saveConfig();
    void updateButtonState();
    void appendSubtitle(const QString &text);
    QString getConfigPath() const;

    QLineEdit *appIdInput;
    QLineEdit *apiKeyInput;
    QPushButton *saveConfigButton;
    QPushButton *testApiButton;
    QPushButton *startStopButton;
    QPushButton *clearButton;
    QTextEdit *subtitleDisplay;
    QStatusBar *statusBar;
    AudioProcessor *audioProcessor;
    BaiduAPI *baiduApi;
    bool isRecording;
};

#endif // MAINWINDOW_H 