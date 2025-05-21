#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include "wasapiaudiocapture.h"

class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();

    void setAppId(const QString &id);
    void setApiKey(const QString &key);
    bool startRecording();
    void stopRecording();

signals:
    void audioDataReceived(const QByteArray &data);
    void newTranslation(const QString &text);
    void error(const QString &message);

private slots:
    void handleTranslationResponse(QNetworkReply *reply);
    void handleAudioData(const QByteArray &data);

private:
    QString getAccessToken();
    void sendAudioToAPI(const QByteArray &audioData, const QString &accessToken);

    QString appId;
    QString apiKey;
    QNetworkAccessManager *networkManager;
    WasapiAudioCapture *audioCapture;
    bool isRecording;
};

#endif // AUDIOPROCESSOR_H 