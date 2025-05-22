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
    }
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