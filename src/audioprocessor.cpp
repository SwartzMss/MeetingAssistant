#include "audioprocessor.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QEventLoop>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , audioCapture(new WasapiAudioCapture(this))
    , isRecording(false)
{
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AudioProcessor::handleTranslationResponse);
    connect(audioCapture, &WasapiAudioCapture::audioDataReceived,
            this, &AudioProcessor::handleAudioData);
    connect(audioCapture, &WasapiAudioCapture::error,
            this, &AudioProcessor::error);
}

AudioProcessor::~AudioProcessor()
{
    if (isRecording) {
        stopRecording();
    }
    delete networkManager;
    delete audioCapture;
}

void AudioProcessor::setAppId(const QString &id)
{
    appId = id;
}

void AudioProcessor::setApiKey(const QString &key)
{
    apiKey = key;
}

bool AudioProcessor::startRecording()
{
    if (isRecording) {
        return true;
    }

    if (!audioCapture->startCapture()) {
        return false;
    }

    isRecording = true;
    return true;
}

void AudioProcessor::stopRecording()
{
    if (!isRecording) {
        return;
    }

    audioCapture->stopCapture();
    isRecording = false;
}

void AudioProcessor::handleAudioData(const QByteArray &data)
{
    if (!data.isEmpty()) {
        emit audioDataReceived(data);
        QString accessToken = getAccessToken();
        if (!accessToken.isEmpty()) {
            sendAudioToAPI(data, accessToken);
        }
    }
}

QString AudioProcessor::getAccessToken()
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
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        
        if (obj.contains("access_token")) {
            return obj["access_token"].toString();
        }
    }
    
    reply->deleteLater();
    return QString();
}

void AudioProcessor::sendAudioToAPI(const QByteArray &audioData, const QString &accessToken)
{
    QUrl url(QString("https://aip.baidubce.com/rpc/2.0/asr/v1/wst?dev_pid=1537&cuid=MeetingAssistant&token=%1")
             .arg(accessToken));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/pcm;rate=16000");
    
    QNetworkReply *reply = networkManager->post(request, audioData);
}

void AudioProcessor::handleTranslationResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        
        if (obj.contains("result")) {
            QString text = obj["result"].toArray().first().toString();
            emit newTranslation(text);
        } else if (obj.contains("error_msg")) {
            emit error(obj["error_msg"].toString());
        }
    } else {
        emit error(reply->errorString());
    }
    
    reply->deleteLater();
} 