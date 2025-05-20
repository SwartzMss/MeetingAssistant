#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScreen>
#include <QDir>
#include <QStandardPaths>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isRecording(false)
{
    ui->setupUi(this);
    
    // 创建音频处理器
    audioProcessor = new AudioProcessor(this);
    
    setupConnections();
    loadConfig();
    updateButtonState();
}

MainWindow::~MainWindow()
{
    if (isRecording) {
        audioProcessor->stopRecording();
    }
    delete ui;
}

void MainWindow::setupConnections()
{
    connect(audioProcessor, &AudioProcessor::newTranslation,
            this, &MainWindow::onNewTranslation);
    connect(audioProcessor, &AudioProcessor::statusChanged,
            this, &MainWindow::onStatusChanged);
    connect(audioProcessor, &AudioProcessor::error,
            this, &MainWindow::onError);
    connect(ui->startStopButton, &QPushButton::clicked,
            this, &MainWindow::onStartStopClicked);
    connect(ui->clearButton, &QPushButton::clicked,
            this, &MainWindow::onClearText);
    connect(ui->saveConfigButton, &QPushButton::clicked,
            this, &MainWindow::onSaveConfig);
    connect(ui->appIdInput, &QLineEdit::textChanged,
            this, &MainWindow::onConfigChanged);
    connect(ui->apiKeyInput, &QLineEdit::textChanged,
            this, &MainWindow::onConfigChanged);
}

void MainWindow::updateButtonState()
{
    bool hasConfig = !ui->appIdInput->text().isEmpty() && 
                    !ui->apiKeyInput->text().isEmpty();
    ui->startStopButton->setEnabled(hasConfig);
}

void MainWindow::onStartStopClicked()
{
    if (!isRecording) {
        audioProcessor->setAppId(ui->appIdInput->text());
        audioProcessor->setApiKey(ui->apiKeyInput->text());
        audioProcessor->startRecording();
        ui->startStopButton->setText("停止");
        isRecording = true;
    } else {
        audioProcessor->stopRecording();
        ui->startStopButton->setText("开始");
        isRecording = false;
    }
}

void MainWindow::onNewTranslation(const QString &text)
{
    ui->subtitleDisplay->append(text);
}

void MainWindow::onClearText()
{
    ui->subtitleDisplay->clear();
}

void MainWindow::onSaveConfig()
{
    saveConfig();
}

void MainWindow::onConfigChanged()
{
    updateButtonState();
}

void MainWindow::onStatusChanged(const QString &status)
{
    ui->statusBar->showMessage(status);
}

void MainWindow::onError(const QString &message)
{
    QMessageBox::warning(this, "错误", message);
    if (isRecording) {
        audioProcessor->stopRecording();
        ui->startStopButton->setText("开始");
        isRecording = false;
    }
}

void MainWindow::loadConfig()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(configPath + "/config.json");
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        
        ui->appIdInput->setText(obj["app_id"].toString());
        
        // 解码Base64编码的API Key
        QString encodedKey = obj["api_key"].toString();
        if (!encodedKey.isEmpty()) {
            QByteArray decodedKey = QByteArray::fromBase64(encodedKey.toUtf8());
            ui->apiKeyInput->setText(QString::fromUtf8(decodedKey));
        }
        
        file.close();
    }
}

void MainWindow::saveConfig()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    QFile file(configPath + "/config.json");
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["app_id"] = ui->appIdInput->text();
        
        // 对API Key进行Base64编码
        QString apiKey = ui->apiKeyInput->text();
        QByteArray encodedKey = apiKey.toUtf8().toBase64();
        obj["api_key"] = QString::fromUtf8(encodedKey);
        
        QJsonDocument doc(obj);
        file.write(doc.toJson());
        file.close();
        
        ui->statusBar->showMessage("配置已保存");
    } else {
        QMessageBox::warning(this, "错误", "无法保存配置文件");
    }
} 