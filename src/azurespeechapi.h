#ifndef AZURESPEECHAPI_H
#define AZURESPEECHAPI_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <speechapi_cxx.h>
#include <speechapi_cxx_translation_recognizer.h>
#include "logger.h"

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Translation;
using namespace Microsoft::CognitiveServices::Speech::Audio;

class AzureSpeechAPI : public QObject
{
    Q_OBJECT

public:
    explicit AzureSpeechAPI(QObject *parent = nullptr);
    ~AzureSpeechAPI();

    // 初始化Azure Speech服务
    void initialize(const QString &subscriptionKey, const QString &region);
    
    // 开始语音识别和翻译
    void startRecognitionAndTranslation(const QString &sourceLanguage, const QString &targetLanguage);
    
    // 停止语音识别和翻译
    void stopRecognitionAndTranslation();
    
    // 处理音频数据
    void processAudioData(const QByteArray &audioData);

    void testConnection(const QString &key, const QString &region);

signals:
    void recognitionResult(const QString &text);
    void translationResult(const QString &text);
    void error(const QString &message);
    void statusChanged(const QString &status);

private:
    std::shared_ptr<SpeechConfig> speechConfig;
    std::shared_ptr<SpeechTranslationConfig> translationConfig;
    std::shared_ptr<TranslationRecognizer> recognizer;
    std::shared_ptr<PushAudioInputStream> audioStream;
    
    bool isInitialized;
    QString currentSourceLanguage;
    QString currentTargetLanguage;
    std::unique_ptr<Logger> logger;
};

#endif // AZURESPEECHAPI_H 