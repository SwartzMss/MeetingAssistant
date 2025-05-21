#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audioProcessor(new AudioProcessor(this))
    , azureSpeechAPI(new AzureSpeechAPI(this))
    , logger(new Logger(this))
{
    ui->setupUi(this);
    
    // 设置配置文件路径
    configFilePath = QCoreApplication::applicationDirPath() + "/config.ini";
    
    // 加载配置
    loadConfig();
    
    // 连接信号和槽
    connect(audioProcessor, &AudioProcessor::audioDataReceived,
            this, &MainWindow::onAudioDataReceived);
    
    connect(azureSpeechAPI, &AzureSpeechAPI::recognitionResult,
            this, &MainWindow::onRecognitionResult);
    connect(azureSpeechAPI, &AzureSpeechAPI::translationResult,
            this, &MainWindow::onTranslationResult);
    connect(azureSpeechAPI, &AzureSpeechAPI::error,
            this, &MainWindow::onError);
    connect(azureSpeechAPI, &AzureSpeechAPI::statusChanged,
            this, &MainWindow::onStatusChanged);

    // 连接按钮信号
    connect(ui->startButton, &QPushButton::clicked,
            this, &MainWindow::onStartButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopButtonClicked);
    connect(ui->testButton, &QPushButton::clicked, this, &MainWindow::onTestButtonClicked);
    connect(ui->saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfigClicked);

    // 初始化UI状态
    ui->stopButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete audioProcessor;
    delete azureSpeechAPI;
    delete logger;
}

void MainWindow::onStartButtonClicked()
{
    QString key = ui->keyEdit->text();
    QString region = ui->regionEdit->text();

    if (key.isEmpty() || region.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写完整的Azure Speech服务配置信息");
        return;
    }

    // 初始化Azure Speech服务
    azureSpeechAPI->initialize(key, region);
    
    // 开始语音识别和翻译
    azureSpeechAPI->startRecognitionAndTranslation("en-US", "zh-CN");
    
    // 开始音频处理
    audioProcessor->startRecording();
    
    // 更新UI状态
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->testButton->setEnabled(false);
    ui->saveConfigButton->setEnabled(false);
}

void MainWindow::onStopButtonClicked()
{
    // 停止音频处理
    audioProcessor->stopRecording();
    
    // 停止语音识别和翻译
    azureSpeechAPI->stopRecognitionAndTranslation();
    
    // 更新UI状态
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->testButton->setEnabled(true);
    ui->saveConfigButton->setEnabled(true);
}

void MainWindow::onAudioDataReceived(const QByteArray &data)
{
    azureSpeechAPI->processAudioData(data);
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

void MainWindow::onSaveConfigClicked()
{
    QString region = ui->regionEdit->text();
    QString key = ui->keyEdit->text();
    
    if (region.isEmpty() || key.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写区域和密钥");
        return;
    }
    
    QSettings settings(configFilePath, QSettings::IniFormat);
    settings.setValue("Azure/Region", region);
    settings.setValue("Azure/Key", key);
    
    QMessageBox::information(this, "保存成功", "配置已保存");
}

void MainWindow::loadConfig()
{
    QSettings settings(configFilePath, QSettings::IniFormat);
    QString region = settings.value("Azure/Region").toString();
    QString key = settings.value("Azure/Key").toString();
    
    ui->regionEdit->setText(region);
    ui->keyEdit->setText(key);
    
    // 如果配置已存在，启用开始按钮
    if (!region.isEmpty() && !key.isEmpty()) {
        ui->startButton->setEnabled(true);
    }
}

void MainWindow::onTestButtonClicked()
{
    QString region = ui->regionEdit->text();
    QString key = ui->keyEdit->text();
    
    if (region.isEmpty() || key.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写区域和密钥");
        return;
    }
    
    // 测试连接
    azureSpeechAPI->testConnection(key, region);
} 