#include "azurespeechapi.h"
#include <QDebug>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Translation;
using namespace Microsoft::CognitiveServices::Speech::Audio;

AzureSpeechAPI::AzureSpeechAPI(QObject *parent)
    : QObject(parent)
    , isInitialized(false)
{
}

AzureSpeechAPI::~AzureSpeechAPI()
{
    stopRecognitionAndTranslation();
}

void AzureSpeechAPI::initialize(const QString &subscriptionKey, const QString &region)
{
    try {
        // 创建语音配置
        speechConfig = SpeechConfig::FromSubscription(subscriptionKey.toStdString(), region.toStdString());
        
        // 设置语音识别和翻译的默认参数
        speechConfig->SetSpeechRecognitionLanguage("zh-CN");
        speechConfig->SetProperty(PropertyId::SpeechServiceConnection_InitialSilenceTimeoutMs, "5000");
        speechConfig->SetProperty(PropertyId::SpeechServiceConnection_EndSilenceTimeoutMs, "1000");
        
        isInitialized = true;
        emit statusChanged("Azure Speech服务初始化成功");
    }
    catch (const std::exception& e) {
        emit error(QString("初始化失败: %1").arg(e.what()));
    }
}

void AzureSpeechAPI::startRecognitionAndTranslation(const QString &sourceLanguage, const QString &targetLanguage)
{
    if (!isInitialized) {
        emit error("请先初始化Azure Speech服务");
        return;
    }

    try {
        currentSourceLanguage = sourceLanguage;
        currentTargetLanguage = targetLanguage;

        // 创建翻译配置
        translationConfig = SpeechTranslationConfig::FromSubscription(speechConfig->GetSubscriptionKey(), speechConfig->GetRegion());
        translationConfig->SetSpeechRecognitionLanguage(sourceLanguage.toStdString());
        translationConfig->AddTargetLanguage(targetLanguage.toStdString());
        
        // 创建音频流
        audioStream = PushAudioInputStream::Create();
        
        // 创建音频配置
        auto audioConfig = AudioConfig::FromStreamInput(audioStream);
        
        // 创建识别器
        recognizer = TranslationRecognizer::FromConfig(translationConfig, audioConfig);
        
        // 设置事件处理
        recognizer->Recognized.Connect([this](const TranslationRecognitionEventArgs& e) {
            if (e.Result->Reason == ResultReason::TranslatedSpeech) {
                emit recognitionResult(QString::fromStdString(e.Result->Text));
                
                // 获取翻译结果
                auto translations = e.Result->Translations;
                if (translations.find(currentTargetLanguage.toStdString()) != translations.end()) {
                    QString translatedText = QString::fromStdString(
                        translations[currentTargetLanguage.toStdString()]);
                    emit translationResult(translatedText);
                }
            }
        });

        recognizer->Canceled.Connect([this](const TranslationRecognitionCanceledEventArgs& e) {
            emit error(QString("识别取消: %1").arg(QString::fromStdString(e.ErrorDetails)));
        });

        // 开始连续识别
        recognizer->StartContinuousRecognitionAsync();
        emit statusChanged("开始语音识别和翻译");
    }
    catch (const std::exception& e) {
        emit error(QString("启动识别失败: %1").arg(e.what()));
    }
}

void AzureSpeechAPI::stopRecognitionAndTranslation()
{
    if (recognizer) {
        try {
            recognizer->StopContinuousRecognitionAsync();
            recognizer.reset();
            audioStream.reset();
            emit statusChanged("停止语音识别和翻译");
        }
        catch (const std::exception& e) {
            emit error(QString("停止识别失败: %1").arg(e.what()));
        }
    }
}

void AzureSpeechAPI::processAudioData(const QByteArray &audioData)
{
    if (!audioStream) {
        emit error("音频流未初始化");
        return;
    }

    try {
        // 将QByteArray转换为std::vector<uint8_t>
        std::vector<uint8_t> audioBuffer(audioData.begin(), audioData.end());
        
        // 写入音频数据
        audioStream->Write(audioBuffer.data(), audioBuffer.size());
    }
    catch (const std::exception& e) {
        emit error(QString("处理音频数据失败: %1").arg(e.what()));
    }
} 