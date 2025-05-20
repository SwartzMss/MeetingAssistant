#ifndef BAIDUAPI_H
#define BAIDUAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QString>
#include <QFile>
#include <QCryptographicHash>
#include <QDateTime>

class BaiduAPI : public QObject
{
    Q_OBJECT

public:
    explicit BaiduAPI(QObject *parent = nullptr);
    ~BaiduAPI();

    // 测试API连接
    void testConnection(const QString &appId, const QString &apiKey);
    
    // 语音识别
    void recognizeSpeech(const QByteArray &audioData);
    
    // 翻译文本
    void translateText(const QString &text, const QString &from = "auto", const QString &to = "zh");

signals:
    void testResult(bool success, const QString &message);
    void recognitionResult(const QString &text);
    void translationResult(const QString &text);
    void error(const QString &message);

private:
    QNetworkAccessManager *networkManager;
    QString accessToken;
    QString currentAppId;
    QString currentApiKey;

    void getAccessToken(const QString &appId, const QString &apiKey);
    QString generateSign(const QString &appId, const QString &apiKey);
    void handleNetworkError(QNetworkReply *reply);
};

#endif // BAIDUAPI_H 