#include "audioprocessor.h"
#include <QAudioDevice>
#include <QAudioSource>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QMediaDevices>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QDateTime>
#include <QEventLoop>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
    , isRecording(false)
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AudioProcessor::handleTranslationResponse);

    setupAudioFormat();
}

AudioProcessor::~AudioProcessor()
{
    if (isRecording) {
        stopRecording();
    }
}

void AudioProcessor::setAppId(const QString &id)
{
    appId = id;
}

void AudioProcessor::setApiKey(const QString &key)
{
    apiKey = key;
}

void AudioProcessor::setupAudioFormat()
{
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
}

void AudioProcessor::startRecording()
{
    if (isRecording) return;

    if (appId.isEmpty() || apiKey.isEmpty()) {
        emit error("请先设置APP ID和API Key");
        return;
    }

    audioBuffer = new QBuffer(this);
    audioBuffer->open(QIODevice::WriteOnly);

    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    audioInput = new QAudioSource(inputDevice, format, this);
    
    connect(audioInput, &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState) {
            handleAudioData();
        }
    });

    audioInput->start(audioBuffer);
    isRecording = true;
    emit statusChanged("正在录音...");
}

void AudioProcessor::stopRecording()
{
    if (!isRecording) return;

    audioInput->stop();
    audioBuffer->close();
    
    delete audioInput;
    delete audioBuffer;
    
    isRecording = false;
    emit statusChanged("就绪");
}

void AudioProcessor::handleAudioData()
{
    if (!audioBuffer) return;

    QByteArray audioData = audioBuffer->buffer();
    if (!audioData.isEmpty()) {
        translateAudio(audioData);
    }
}

void AudioProcessor::translateAudio(const QByteArray &audioData)
{
    QString accessToken = getAccessToken();
    if (accessToken.isEmpty()) {
        emit error("获取访问令牌失败");
        return;
    }

    sendAudioToAPI(audioData, accessToken);
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
    
    // 等待响应
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
    // 响应会在handleTranslationResponse中处理
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