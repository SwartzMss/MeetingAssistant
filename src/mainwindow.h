#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioInput>
#include <QAudioOutput>
#include <QBuffer>
#include <QTimer>
#include "audioprocessor.h"
#include "azurespeechapi.h"

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
    void onTestConnectionButtonClicked();
    void onAudioDataReceived(const QByteArray &data);
    void onRecognitionResult(const QString &text);
    void onTranslationResult(const QString &text);
    void onError(const QString &message);
    void onStatusChanged(const QString &status);

private:
    Ui::MainWindow *ui;
    AudioProcessor *audioProcessor;
    AzureSpeechAPI *azureSpeechApi;
    QString currentAppId;
    QString currentApiKey;
    QString currentRegion;
};

#endif // MAINWINDOW_H 