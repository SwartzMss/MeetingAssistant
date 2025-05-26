#include "azurespeechapi.h"

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Translation;
using namespace Microsoft::CognitiveServices::Speech::Audio;

AzureSpeechAPI::AzureSpeechAPI(QObject *parent)
    : QObject(parent)
    , isInitialized(false)
    , logger(std::make_unique<Logger>())
{
    LOG_INFO("AzureSpeechAPI 初始化");
}

AzureSpeechAPI::~AzureSpeechAPI()
{
    LOG_INFO("AzureSpeechAPI 析构");
    stopRecognitionAndTranslation();
}

void AzureSpeechAPI::initialize(const QString &subscriptionKey, const QString &region)
{
    try {
        LOG_INFO(QString("开始初始化 Azure Speech 服务，区域: %1").arg(region));
        
        // 创建语音配置
        speechConfig = SpeechConfig::FromSubscription(subscriptionKey.toStdString(), region.toStdString());
        
        // 设置语音识别和翻译的默认参数
        speechConfig->SetSpeechRecognitionLanguage("zh-CN");
        speechConfig->SetProperty(PropertyId::SpeechServiceConnection_InitialSilenceTimeoutMs, "5000");
        speechConfig->SetProperty(PropertyId::SpeechServiceConnection_EndSilenceTimeoutMs, "1000");
        
        isInitialized = true;
        LOG_INFO("Azure Speech 服务初始化成功");
        emit statusChanged("Azure Speech服务初始化成功");
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("初始化失败: %1").arg(e.what());
        LOG_ERROR(errorMsg);
        emit error(errorMsg);
    }
}

void AzureSpeechAPI::startRecognitionAndTranslation(const QString &sourceLanguage, const QString &targetLanguage)
{
    if (!isInitialized) {
        LOG_ERROR("请先初始化Azure Speech服务");
        emit error("请先初始化Azure Speech服务");
        return;
    }

    try {
        LOG_INFO(QString("开始语音识别和翻译，源语言: %1, 目标语言: %2")
                   .arg(sourceLanguage)
                   .arg(targetLanguage));

        currentSourceLanguage = sourceLanguage;
        currentTargetLanguage = targetLanguage;

        // 创建翻译配置
        translationConfig = SpeechTranslationConfig::FromSubscription(speechConfig->GetSubscriptionKey(), speechConfig->GetRegion());
        if (!translationConfig) {
            LOG_ERROR("创建翻译配置失败");
            emit error("创建翻译配置失败");
            return;
        }

        translationConfig->SetSpeechRecognitionLanguage(sourceLanguage.toStdString());
        translationConfig->AddTargetLanguage(targetLanguage.toStdString());
        
        // 创建音频流
        audioStream = PushAudioInputStream::Create();
        if (!audioStream) {
            LOG_ERROR("创建音频流失败");
            emit error("创建音频流失败");
            return;
        }
        
        // 创建音频配置
        auto audioConfig = AudioConfig::FromStreamInput(audioStream);
        if (!audioConfig) {
            LOG_ERROR("创建音频配置失败");
            emit error("创建音频配置失败");
            return;
        }
        
        // 创建识别器
        recognizer = TranslationRecognizer::FromConfig(translationConfig, audioConfig);
        if (!recognizer) {
            LOG_ERROR("创建识别器失败");
            emit error("创建识别器失败");
            return;
        }
        
        // 设置事件处理
        recognizer->Recognized.Connect([this](const TranslationRecognitionEventArgs& e) {
            try {
                if (e.Result->Reason == ResultReason::TranslatedSpeech) {
                    // 最终英文
                    QString text = QString::fromStdString(e.Result->Text);
                    emit recognitionResult(text);
                    // 最终中文
                    auto translations = e.Result->Translations;
                    if (translations.find(currentTargetLanguage.toStdString()) != translations.end()) {
                        QString translatedText = QString::fromStdString(translations[currentTargetLanguage.toStdString()]);
                        emit translationResult(translatedText);
                        emit finalTranslationResult(translatedText);
                    }
                } else if (e.Result->Reason == ResultReason::NoMatch) {
                    LOG_INFO("未检测到语音");
                } else if (e.Result->Reason == ResultReason::Canceled) {
                    LOG_ERROR("识别被取消");
                }
            } catch (const std::exception& ex) {
                LOG_ERROR(QString("处理识别结果时发生异常: %1").arg(ex.what()));
                emit error(QString("处理识别结果时发生异常: %1").arg(ex.what()));
            }
        });

        // Recognizing 事件
        recognizer->Recognizing.Connect([this](const TranslationRecognitionEventArgs& e) {
            if (e.Result->Reason == ResultReason::TranslatingSpeech) {
                // 实时英文
                QString partialText = QString::fromStdString(e.Result->Text);
                emit recognitionResult(partialText);
                // 实时中文
                auto translations = e.Result->Translations;
                if (translations.find(currentTargetLanguage.toStdString()) != translations.end()) {
                    QString translatedText = QString::fromStdString(translations[currentTargetLanguage.toStdString()]);
                    emit translationResult(translatedText);
                }
            }
        });

        recognizer->Canceled.Connect([this](const TranslationRecognitionCanceledEventArgs& e) {
            LOG_ERROR(QString("识别取消: %1").arg(QString::fromStdString(e.ErrorDetails)));
            emit error(QString("识别取消: %1").arg(QString::fromStdString(e.ErrorDetails)));
        });

        recognizer->SessionStarted.Connect([this](const SessionEventArgs&) {
            LOG_INFO("识别会话开始");
        });

        recognizer->SessionStopped.Connect([this](const SessionEventArgs&) {
            LOG_INFO("识别会话结束");
        });

        // 开始连续识别
        try {
            recognizer->StartContinuousRecognitionAsync().wait();
            LOG_INFO("开始语音识别和翻译");
            emit statusChanged("开始语音识别和翻译");
        } catch (const std::exception& e) {
            LOG_ERROR(QString("启动连续识别失败: %1").arg(e.what()));
            emit error(QString("启动连续识别失败: %1").arg(e.what()));
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("启动识别失败: %1").arg(e.what()));
        emit error(QString("启动识别失败: %1").arg(e.what()));
    }
}

void AzureSpeechAPI::stopRecognitionAndTranslation()
{
    if (recognizer) {
        try {
            LOG_INFO("停止语音识别和翻译");
            recognizer->StopContinuousRecognitionAsync().wait();
            recognizer.reset();
            audioStream.reset();
            emit statusChanged("停止语音识别和翻译");
        }
        catch (const std::exception& e) {
            QString errorMsg = QString("停止识别失败: %1").arg(e.what());
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
        }
    }
}

void AzureSpeechAPI::processAudioData(const QByteArray &audioData)
{
    if (!audioStream) {
        // 停止后收到的音频数据，直接忽略，不报错
        return;
    }

    try {
        // 将QByteArray转换为std::vector<uint8_t>
        std::vector<uint8_t> audioBuffer(audioData.begin(), audioData.end());
        
        // 写入音频数据，确保大小不超过uint32_t的最大值
        if (audioBuffer.size() > UINT32_MAX) {
            LOG_ERROR("音频数据块太大");
            emit error("音频数据块太大");
            return;
        }
        
        // 使用静态计数器来减少日志输出频率
        static int logCounter = 0;
        if (++logCounter % 10 == 0) {  // 每10个数据块记录一次
            LOG_INFO(QString("处理音频数据，大小: %1 字节").arg(audioBuffer.size()));
        }
        
        // 写入音频数据
        try {
            audioStream->Write(audioBuffer.data(), static_cast<uint32_t>(audioBuffer.size()));
            if (logCounter % 10 == 0) {
                LOG_INFO("音频数据写入成功");
            }
        } catch (const std::exception& e) {
            LOG_ERROR(QString("写入音频数据失败: %1").arg(e.what()));
            emit error(QString("写入音频数据失败: %1").arg(e.what()));
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("处理音频数据失败: %1").arg(e.what()));
        emit error(QString("处理音频数据失败: %1").arg(e.what()));
    }
}

void AzureSpeechAPI::testConnection(const QString &key, const QString &region)
{
    try {
        LOG_INFO(QString("开始测试连接，区域: %1").arg(region));
        
        // 1. 构造配置
        auto config = SpeechConfig::FromSubscription(
            key.toStdString(),
            region.toStdString()
        );

        if (!config) {
            QString errorMsg = "Failed to create speech config";
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
            return;
        }
        LOG_INFO("Speech config created successfully");

        // 创建音频流
        auto audioStream = PushAudioInputStream::Create();
        if (!audioStream) {
            QString errorMsg = "Failed to create audio stream";
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
            return;
        }
        LOG_INFO("Audio stream created successfully");

        // 创建音频配置
        auto audioConfig = AudioConfig::FromStreamInput(audioStream);
        if (!audioConfig) {
            QString errorMsg = "Failed to create audio config";
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
            return;
        }

        // 生成100ms的静音数据
        std::vector<uint8_t> silenceData(16000 * 2 * 0.1); // 16kHz, 16-bit, 100ms
        audioStream->Write(silenceData.data(), static_cast<uint32_t>(silenceData.size()));
        audioStream->Close();
        LOG_INFO("Silence data written to stream");

        // 创建识别器
        auto recognizer = SpeechRecognizer::FromConfig(config, audioConfig);
        if (!recognizer) {
            QString errorMsg = "Failed to create speech recognizer";
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
            return;
        }
        LOG_INFO("Speech recognizer created successfully");

        // 进行识别
        auto result = recognizer->RecognizeOnceAsync().get();
        if (result->Reason == ResultReason::RecognizedSpeech || result->Reason == ResultReason::NoMatch) {
            LOG_INFO("Connection test successful");
            emit statusChanged("连接测试成功");
        } else {
            QString reasonStr;
            switch (result->Reason) {
                case ResultReason::RecognizedSpeech: reasonStr = "RecognizedSpeech"; break;
                case ResultReason::NoMatch: reasonStr = "NoMatch"; break;
                case ResultReason::Canceled: reasonStr = "Canceled"; break;
                default: reasonStr = QString::number(static_cast<int>(result->Reason)); break;
            }
            QString errorMsg = QString("Connection test failed: %1, Text: %2")
                .arg(reasonStr)
                .arg(QString::fromStdString(result->Text));
            LOG_ERROR(errorMsg);
            emit error(errorMsg);
        }
    }
    catch (const std::exception &e) {
        QString msg = QString("连接测试异常: %1").arg(e.what());
        LOG_ERROR(msg);
        emit statusChanged(msg);
        emit error(msg);
    }
} 