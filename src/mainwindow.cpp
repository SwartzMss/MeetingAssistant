#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QByteArray>
#include <QCoreApplication>
#include "logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isRecording(false)
{
    setupUI();
    loadConfig();
    updateButtonState();

    audioProcessor = new AudioProcessor(this);
    baiduApi = new BaiduAPI(this);

    connect(audioProcessor, &AudioProcessor::audioDataReceived,
            this, &MainWindow::onAudioDataReceived);
    connect(baiduApi, &BaiduAPI::recognitionResult,
            this, &MainWindow::onRecognitionResult);
    connect(baiduApi, &BaiduAPI::translationResult,
            this, &MainWindow::onTranslationResult);
    connect(baiduApi, &BaiduAPI::error,
            this, &MainWindow::onApiError);
    connect(baiduApi, &BaiduAPI::testResult,
            this, &MainWindow::onApiTestResult);
}

MainWindow::~MainWindow()
{
    delete audioProcessor;
    delete baiduApi;
}

void MainWindow::setupUI()
{
    resize(800, 600);
    setWindowTitle("Meeting Assistant");

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 配置区域
    QHBoxLayout *configLayout = new QHBoxLayout();
    configLayout->setSpacing(10);

    QLabel *appIdLabel = new QLabel("APP ID:", this);
    appIdInput = new QLineEdit(this);
    appIdInput->setMinimumWidth(150);

    QLabel *apiKeyLabel = new QLabel("API Key:", this);
    apiKeyInput = new QLineEdit(this);
    apiKeyInput->setMinimumWidth(150);
    apiKeyInput->setEchoMode(QLineEdit::Password);

    saveConfigButton = new QPushButton("保存配置", this);
    testApiButton = new QPushButton("测试API", this);

    configLayout->addWidget(appIdLabel);
    configLayout->addWidget(appIdInput);
    configLayout->addWidget(apiKeyLabel);
    configLayout->addWidget(apiKeyInput);
    configLayout->addWidget(saveConfigButton);
    configLayout->addWidget(testApiButton);
    configLayout->addStretch();

    mainLayout->addLayout(configLayout);

    // 字幕显示区域
    subtitleDisplay = new QTextEdit(this);
    subtitleDisplay->setMinimumHeight(400);
    subtitleDisplay->setReadOnly(true);
    subtitleDisplay->setFont(QFont("Arial", 16));
    mainLayout->addWidget(subtitleDisplay);

    // 控制按钮区域
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(10);

    startStopButton = new QPushButton("开始", this);
    startStopButton->setMinimumWidth(80);
    clearButton = new QPushButton("清除", this);
    clearButton->setMinimumWidth(80);

    controlLayout->addWidget(startStopButton);
    controlLayout->addWidget(clearButton);
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout);

    // 状态栏
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // 连接信号和槽
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfigClicked);
    connect(testApiButton, &QPushButton::clicked, this, &MainWindow::onTestApiClicked);
}

void MainWindow::onStartStopClicked()
{
    if (!isRecording) {
        if (audioProcessor->startRecording()) {
            isRecording = true;
            startStopButton->setText("停止");
            statusBar->showMessage("正在录音...");
            Logger::instance().log("开始录音");
        } else {
            QMessageBox::critical(this, "错误", "无法启动录音设备");
            Logger::instance().log("无法启动录音设备", "ERROR");
        }
    } else {
        audioProcessor->stopRecording();
        isRecording = false;
        startStopButton->setText("开始");
        statusBar->showMessage("就绪");
        Logger::instance().log("停止录音");
    }
}

void MainWindow::onClearClicked()
{
    subtitleDisplay->clear();
}

void MainWindow::onSaveConfigClicked()
{
    saveConfig();
    QMessageBox::information(this, "成功", "配置已保存");
    Logger::instance().log("配置已保存");
}

void MainWindow::onTestApiClicked()
{
    QString appId = appIdInput->text().trimmed();
    QString apiKey = apiKeyInput->text().trimmed();

    if (appId.isEmpty() || apiKey.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入APP ID和API Key");
        Logger::instance().log("API测试失败：未输入APP ID或API Key", "WARNING");
        return;
    }

    statusBar->showMessage("正在测试API连接...");
    Logger::instance().log("开始测试API连接");
    baiduApi->testConnection(appId, apiKey);
}

void MainWindow::onAudioDataReceived(const QByteArray &data)
{
    baiduApi->recognizeSpeech(data);
}

void MainWindow::onRecognitionResult(const QString &text)
{
    appendSubtitle(text);
    baiduApi->translateText(text);
}

void MainWindow::onTranslationResult(const QString &text)
{
    appendSubtitle(text);
}

void MainWindow::onApiError(const QString &message)
{
    statusBar->showMessage(message);
    QMessageBox::warning(this, "错误", message);
    Logger::instance().log("API错误：" + message, "ERROR");
}

void MainWindow::onApiTestResult(bool success, const QString &message)
{
    if (success) {
        statusBar->showMessage("API连接测试成功");
        QMessageBox::information(this, "成功", message);
        Logger::instance().log("API连接测试成功：" + message);
    } else {
        statusBar->showMessage("API连接测试失败");
        QMessageBox::warning(this, "错误", message);
        Logger::instance().log("API连接测试失败：" + message, "ERROR");
    }
}

void MainWindow::loadConfig()
{
    QFile file(getConfigPath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        
        if (obj.contains("appId")) {
            appIdInput->setText(obj["appId"].toString());
        }
        if (obj.contains("apiKey")) {
            apiKeyInput->setText(obj["apiKey"].toString());
        }
        
        file.close();
        Logger::instance().log("成功加载配置文件");
    } else {
        Logger::instance().log("无法打开配置文件", "WARNING");
    }
}

void MainWindow::saveConfig()
{
    QJsonObject obj;
    obj["appId"] = appIdInput->text();
    obj["apiKey"] = apiKeyInput->text();

    QFile file(getConfigPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
        Logger::instance().log("成功保存配置文件");
    } else {
        QMessageBox::warning(this, "错误", "无法保存配置文件");
        Logger::instance().log("无法保存配置文件", "ERROR");
    }
}

void MainWindow::updateButtonState()
{
    bool hasConfig = !appIdInput->text().isEmpty() && !apiKeyInput->text().isEmpty();
    startStopButton->setEnabled(hasConfig);
}

void MainWindow::appendSubtitle(const QString &text)
{
    subtitleDisplay->append(text);
}

QString MainWindow::getConfigPath() const
{
    // 获取应用程序可执行文件所在目录
    QString exePath = QCoreApplication::applicationDirPath();
    return exePath + "/MeetingAssistant.json";
} 