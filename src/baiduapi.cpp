#include "baiduapi.h"
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QRandomGenerator>

BaiduAPI::BaiduAPI(QObject *parent) : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
}

BaiduAPI::~BaiduAPI()
{
    delete networkManager;
}

void BaiduAPI::testConnection(const QString &appId, const QString &apiKey)
{
    currentAppId = appId;
    currentApiKey = apiKey;
    
    // 生成随机数
    QString salt = QString::number(QRandomGenerator::global()->bounded(1000000));
    // 生成签名
    QString signStr = appId + "test" + salt + apiKey;
    QByteArray signBytes = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5);
    QString sign = signBytes.toHex();

    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", "test");
    query.addQueryItem("from", "auto");
    query.addQueryItem("to", "zh");
    query.addQueryItem("appid", appId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "MeetingAssistant/1.0");

    QNetworkReply *reply = networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            
            if (obj.contains("trans_result")) {
                emit testResult(true, "API连接测试成功");
            } else if (obj.contains("error_code")) {
                QString errorMsg = QString("API连接测试失败：%1 - %2")
                    .arg(obj["error_code"].toString())
                    .arg(obj["error_msg"].toString());
                emit testResult(false, errorMsg);
            } else {
                emit testResult(false, "API连接测试失败：未知错误");
            }
        } else {
            emit testResult(false, "网络错误: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void BaiduAPI::getAccessToken(const QString &appId, const QString &apiKey)
{
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", appId);
    query.addQueryItem("client_secret", apiKey);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QByteArray());
    
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            
            if (obj.contains("access_token")) {
                accessToken = obj["access_token"].toString();
                emit testResult(true, "API连接测试成功");
            } else {
                emit testResult(false, "获取访问令牌失败: " + obj["error_description"].toString());
            }
        } else {
            emit testResult(false, "网络错误: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void BaiduAPI::recognizeSpeech(const QByteArray &audioData)
{
    if (accessToken.isEmpty()) {
        emit error("未获取访问令牌，请先测试API连接");
        return;
    }

    QUrl url("https://vop.baidu.com/server_api");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["format"] = "pcm";
    json["rate"] = 16000;
    json["channel"] = 1;
    json["token"] = accessToken;
    json["cuid"] = "MeetingAssistant";
    json["len"] = audioData.size();
    json["speech"] = QString(audioData.toBase64());

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            
            if (obj.contains("result")) {
                QJsonArray results = obj["result"].toArray();
                if (!results.isEmpty()) {
                    emit recognitionResult(results[0].toString());
                }
            } else {
                emit error("语音识别失败: " + obj["err_msg"].toString());
            }
        } else {
            handleNetworkError(reply);
        }
        reply->deleteLater();
    });
}

void BaiduAPI::translateText(const QString &text, const QString &from, const QString &to)
{
    if (currentAppId.isEmpty() || currentApiKey.isEmpty()) {
        emit error("未设置APP ID或API Key");
        return;
    }

    // 生成随机数
    QString salt = QString::number(QRandomGenerator::global()->bounded(1000000));
    // 生成签名
    QString signStr = currentAppId + text + salt + currentApiKey;
    QByteArray signBytes = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5);
    QString sign = signBytes.toHex();

    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", from);
    query.addQueryItem("to", to);
    query.addQueryItem("appid", currentAppId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "MeetingAssistant/1.0");

    QNetworkReply *reply = networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            
            if (obj.contains("trans_result")) {
                QJsonArray results = obj["trans_result"].toArray();
                if (!results.isEmpty()) {
                    QJsonObject result = results[0].toObject();
                    emit translationResult(result["dst"].toString());
                }
            } else if (obj.contains("error_code")) {
                QString errorMsg = QString("翻译失败：%1 - %2")
                    .arg(obj["error_code"].toString())
                    .arg(obj["error_msg"].toString());
                emit error(errorMsg);
            }
        } else {
            handleNetworkError(reply);
        }
        reply->deleteLater();
    });
}

void BaiduAPI::handleNetworkError(QNetworkReply *reply)
{
    emit error("网络错误: " + reply->errorString());
} 