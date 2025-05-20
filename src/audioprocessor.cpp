#include "audioprocessor.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QEventLoop>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
    , audioSource(nullptr)
    , buffer(nullptr)
    , networkManager(new QNetworkAccessManager(this))
    , isRecording(false)
{
    setupAudioFormat();
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AudioProcessor::handleTranslationResponse);
}

AudioProcessor::~AudioProcessor()
{
    if (isRecording) {
        stopRecording();
    }
    delete audioSource;
    delete buffer;
    delete networkManager;
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

bool AudioProcessor::startRecording()
{
    if (isRecording) {
        return true;
    }

    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (!inputDevice.isFormatSupported(format)) {
        format = inputDevice.preferredFormat();
    }

    buffer = new QBuffer(this);
    buffer->open(QIODevice::WriteOnly);

    audioSource = new QAudioSource(format, this);
    audioSource->setBufferSize(4096);

    connect(audioSource, &QAudioSource::stateChanged,
            this, &AudioProcessor::handleStateChanged);

    audioSource->start(buffer);
    isRecording = true;
    return true;
}

void AudioProcessor::stopRecording()
{
    if (!isRecording) {
        return;
    }

    if (audioSource) {
        audioSource->stop();
        buffer->close();
        
        // 发送缓冲区中的数据
        QByteArray audioData = buffer->data();
        if (!audioData.isEmpty()) {
            emit audioDataReceived(audioData);
        }
    }

    isRecording = false;
}

void AudioProcessor::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::StoppedState:
            if (audioSource->error() != QAudio::NoError) {
                QString errorMsg;
                switch (audioSource->error()) {
                    case QAudio::OpenError:
                        errorMsg = "无法打开音频设备";
                        break;
                    case QAudio::IOError:
                        errorMsg = "音频设备IO错误";
                        break;
                    case QAudio::UnderrunError:
                        errorMsg = "音频数据下溢";
                        break;
                    case QAudio::FatalError:
                        errorMsg = "音频设备致命错误";
                        break;
                    default:
                        errorMsg = "未知错误";
                        break;
                }
                emit error(errorMsg);
            }
            break;
        case QAudio::ActiveState:
            // 开始录音
            break;
        default:
            break;
    }
}

void AudioProcessor::handleDataReceived()
{
    if (buffer && buffer->size() > 0) {
        QByteArray audioData = buffer->data();
        buffer->buffer().clear();
        emit audioDataReceived(audioData);
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