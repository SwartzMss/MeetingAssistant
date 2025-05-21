#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audioProcessor(new AudioProcessor(this))
    , azureSpeechApi(new AzureSpeechAPI(this))
{
    ui->setupUi(this);

    // 连接信号和槽
    connect(audioProcessor, &AudioProcessor::audioDataReceived,
            this, &MainWindow::onAudioDataReceived);
    
    connect(azureSpeechApi, &AzureSpeechAPI::recognitionResult,
            this, &MainWindow::onRecognitionResult);
    connect(azureSpeechApi, &AzureSpeechAPI::translationResult,
            this, &MainWindow::onTranslationResult);
    connect(azureSpeechApi, &AzureSpeechAPI::error,
            this, &MainWindow::onError);
    connect(azureSpeechApi, &AzureSpeechAPI::statusChanged,
            this, &MainWindow::onStatusChanged);

    // 连接按钮信号
    connect(ui->startButton, &QPushButton::clicked,
            this, &MainWindow::onStartButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopButtonClicked);
    connect(ui->testConnectionButton, &QPushButton::clicked,
            this, &MainWindow::onTestConnectionButtonClicked);

    // 初始化UI状态
    ui->stopButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete audioProcessor;
    delete azureSpeechApi;
}

void MainWindow::onStartButtonClicked()
{
    currentAppId = ui->appIdInput->text();
    currentApiKey = ui->appIdInput->text();
    currentRegion = ui->regionInput->text();

    if (currentAppId.isEmpty() || currentApiKey.isEmpty() || currentRegion.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写完整的Azure Speech服务配置信息");
        return;
    }

    // 初始化Azure Speech服务
    azureSpeechApi->initialize(currentApiKey, currentRegion);
    
    // 开始语音识别和翻译
    azureSpeechApi->startRecognitionAndTranslation("zh-CN", "en-US");
    
    // 开始音频处理
    audioProcessor->startRecording();
    
    // 更新UI状态
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->testConnectionButton->setEnabled(false);
}

void MainWindow::onStopButtonClicked()
{
    // 停止音频处理
    audioProcessor->stopRecording();
    
    // 停止语音识别和翻译
    azureSpeechApi->stopRecognitionAndTranslation();
    
    // 更新UI状态
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->testConnectionButton->setEnabled(true);
}

void MainWindow::onTestConnectionButtonClicked()
{
    currentAppId = ui->appIdInput->text();
    currentApiKey = ui->appIdInput->text();
    currentRegion = ui->regionInput->text();

    if (currentAppId.isEmpty() || currentApiKey.isEmpty() || currentRegion.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写完整的Azure Speech服务配置信息");
        return;
    }

    // 初始化Azure Speech服务
    azureSpeechApi->initialize(currentApiKey, currentRegion);
}

void MainWindow::onAudioDataReceived(const QByteArray &data)
{
    azureSpeechApi->processAudioData(data);
}

void MainWindow::onRecognitionResult(const QString &text)
{
    ui->recognitionText->append(text);
}

void MainWindow::onTranslationResult(const QString &text)
{
    ui->translationText->append(text);
}

void MainWindow::onError(const QString &message)
{
    QMessageBox::warning(this, "错误", message);
    ui->statusBar->showMessage(message);
}

void MainWindow::onStatusChanged(const QString &status)
{
    ui->statusBar->showMessage(status);
} 