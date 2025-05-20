#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QAudioSource>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMediaDevices>
#include <QString>
#include <QIODevice>

class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();

    bool startRecording();
    void stopRecording();
    void setAppId(const QString &appId);
    void setApiKey(const QString &apiKey);

signals:
    void audioDataReceived(const QByteArray &data);
    void newTranslation(const QString &text);
    void statusChanged(const QString &status);
    void error(const QString &message);

private slots:
    void handleStateChanged(QAudio::State newState);
    void handleDataReceived();
    void handleTranslationResponse(QNetworkReply *reply);

private:
    QAudioSource *audioSource;
    QBuffer *buffer;
    QNetworkAccessManager *networkManager;
    QAudioFormat format;
    bool isRecording;
    QString appId;
    QString apiKey;

    void setupAudioFormat();
    void translateAudio(const QByteArray &audioData);
    QString getAccessToken();
    void sendAudioToAPI(const QByteArray &audioData, const QString &accessToken);
};

#endif // AUDIOPROCESSOR_H 