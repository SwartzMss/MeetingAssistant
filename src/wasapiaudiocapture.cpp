#include "wasapiaudiocapture.h"
#include "logger.h"
#include <comdef.h>
#include <vector>
#include <cmath>

WasapiAudioCapture::WasapiAudioCapture(QObject *parent)
    : QObject(parent)
    , m_deviceEnumerator(nullptr)
    , m_audioDevice(nullptr)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    , m_captureThread(nullptr)
    , m_isCapturing(false)
    , m_waveFormat(nullptr)
    , m_bufferFrameCount(0)
    , logger(std::make_unique<Logger>())
{
    LOG_INFO("WasapiAudioCapture 初始化");
}

WasapiAudioCapture::~WasapiAudioCapture()
{
    LOG_INFO("WasapiAudioCapture 析构");
    stopCapture();
    cleanupWASAPI();
}

bool WasapiAudioCapture::initializeWASAPI()
{
    LOG_INFO("开始初始化 WASAPI");
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                 nullptr,
                                 CLSCTX_ALL,
                                 __uuidof(IMMDeviceEnumerator),
                                 (void**)&m_deviceEnumerator);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法创建设备枚举器，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法创建设备枚举器");
        return false;
    }

    // 获取默认音频输出设备
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_audioDevice);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法获取默认音频输出设备，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法获取默认音频输出设备");
        return false;
    }

    // 获取音频客户端
    hr = m_audioDevice->Activate(__uuidof(IAudioClient),
                                CLSCTX_ALL,
                                nullptr,
                                (void**)&m_audioClient);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法激活音频客户端，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法激活音频客户端");
        return false;
    }

    // 获取系统混音格式（可能是 WAVEFORMATEXTENSIBLE）
    WAVEFORMATEX* pMixFormat = nullptr;
    hr = m_audioClient->GetMixFormat(&pMixFormat);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法获取设备混音格式，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法获取设备混音格式");
        return false;
    }

    // 检查是否支持 16kHz/16bit/单声道格式
    WAVEFORMATEX desiredFormat = {0};
    desiredFormat.wFormatTag = WAVE_FORMAT_PCM;
    desiredFormat.nChannels = 1;
    desiredFormat.nSamplesPerSec = 16000;
    desiredFormat.wBitsPerSample = 16;
    desiredFormat.nBlockAlign = desiredFormat.nChannels * desiredFormat.wBitsPerSample / 8;
    desiredFormat.nAvgBytesPerSec = desiredFormat.nSamplesPerSec * desiredFormat.nBlockAlign;

    BOOL supported = FALSE;
    hr = m_audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &desiredFormat, nullptr);
    if (SUCCEEDED(hr)) {
        LOG_INFO("设备支持 16kHz/16bit/单声道格式，将使用此格式");
        // 分配足够的内存来存储 WAVEFORMATEXTENSIBLE
        m_waveFormat = (WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        if (!m_waveFormat) {
            LOG_ERROR("内存分配失败");
            CoTaskMemFree(pMixFormat);
            emit error("内存分配失败");
            return false;
        }
        // 复制基本格式信息
        memcpy(&m_waveFormat->Format, &desiredFormat, sizeof(WAVEFORMATEX));
        // 设置扩展信息
        m_waveFormat->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        m_waveFormat->Samples.wValidBitsPerSample = desiredFormat.wBitsPerSample;
        m_waveFormat->dwChannelMask = SPEAKER_FRONT_CENTER;
        m_waveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    } else {
        LOG_INFO(QString("设备不支持 16kHz/16bit/单声道格式，将使用系统混音格式：%1 通道, %2 Hz, %3 bit")
                 .arg(pMixFormat->nChannels)
                 .arg(pMixFormat->nSamplesPerSec)
                 .arg(pMixFormat->wBitsPerSample));
        // 直接使用系统混音格式
        m_waveFormat = (WAVEFORMATEXTENSIBLE*)pMixFormat;
        pMixFormat = nullptr;  // 防止在后面的 CoTaskMemFree 中重复释放
    }

    // 使用选定的格式初始化
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK,
                                 0,    // 默认缓冲区时长
                                 0,    // event-driven 模式
                                 (WAVEFORMATEX*)m_waveFormat,
                                 nullptr);

    if (pMixFormat) {
        CoTaskMemFree(pMixFormat);  // 如果之前没有使用系统混音格式，释放它
    }

    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法初始化音频客户端，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法初始化音频客户端");
        return false;
    }

    // 获取缓冲区大小
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法获取缓冲区大小，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法获取缓冲区大小");
        return false;
    }

    LOG_INFO(QString("音频缓冲区大小：%1 帧").arg(m_bufferFrameCount));

    // 获取捕获客户端
    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient),
                                  (void**)&m_captureClient);
    if (FAILED(hr)) {
        _com_error err(hr);
        LOG_ERROR(QString("无法获取捕获客户端，错误代码: 0x%1, 描述: %2")
                 .arg(hr, 8, 16, QChar('0'))
                 .arg(QString::fromWCharArray(err.Description())));
        emit error("无法获取捕获客户端");
        return false;
    }

    LOG_INFO("WASAPI 初始化完成");
    return true;
}

void WasapiAudioCapture::cleanupWASAPI()
{
    LOG_INFO("开始清理 WASAPI 资源");
    if (m_captureClient) {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }
    if (m_audioClient) {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }
    if (m_audioDevice) {
        m_audioDevice->Release();
        m_audioDevice = nullptr;
    }
    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }
    if (m_waveFormat) {
        CoTaskMemFree(m_waveFormat);  // 使用 CoTaskMemFree 释放内存
        m_waveFormat = nullptr;
    }
    LOG_INFO("WASAPI 资源清理完成");
}

bool WasapiAudioCapture::startCapture()
{
    if (m_isCapturing) {
        LOG_INFO("音频捕获已经在进行中");
        return true;
    }

    LOG_INFO("开始音频捕获");
    if (!initializeWASAPI()) {
        LOG_ERROR("初始化 WASAPI 失败");
        return false;
    }

    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr)) {
        LOG_ERROR("无法启动音频捕获");
        emit error("无法启动音频捕获");
        return false;
    }

    m_isCapturing = true;
    m_captureThread = CreateThread(nullptr, 0, captureThread, this, 0, nullptr);
    if (!m_captureThread) {
        LOG_ERROR("无法创建捕获线程");
        emit error("无法创建捕获线程");
        stopCapture();
        return false;
    }

    LOG_INFO("音频捕获已启动");
    return true;
}

void WasapiAudioCapture::stopCapture()
{
    if (!m_isCapturing) {
        return;
    }

    LOG_INFO("停止音频捕获");
    m_isCapturing = false;
    if (m_captureThread) {
        WaitForSingleObject(m_captureThread, INFINITE);
        CloseHandle(m_captureThread);
        m_captureThread = nullptr;
    }

    if (m_audioClient) {
        m_audioClient->Stop();
    }
    LOG_INFO("音频捕获已停止");
}

DWORD WINAPI WasapiAudioCapture::captureThread(LPVOID context)
{
    WasapiAudioCapture* capture = static_cast<WasapiAudioCapture*>(context);
    UINT32 packetLength = 0;
    BYTE* data = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD flags = 0;

    while (capture->m_isCapturing) {
        Sleep(10); // 避免过度占用CPU

        HRESULT hr = capture->m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            continue;
        }

        while (packetLength > 0) {
            hr = capture->m_captureClient->GetBuffer(&data,
                                                   &numFramesAvailable,
                                                   &flags,
                                                   nullptr,
                                                   nullptr);
            if (FAILED(hr)) {
                break;
            }

            if (data && numFramesAvailable > 0) {
                capture->processAudioData(data, numFramesAvailable);
            }

            capture->m_captureClient->ReleaseBuffer(numFramesAvailable);
            hr = capture->m_captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) {
                break;
            }
        }
    }

    return 0;
}

void WasapiAudioCapture::processAudioData(const BYTE* data, UINT32 numFrames)
{
    if (!data || numFrames == 0) {
        return;
    }

    // 将 BYTE* 转换为 float* 数组
    auto floatBuf = reinterpret_cast<const float*>(data);
    
    // 创建 16-bit PCM 缓冲区
    std::vector<int16_t> pcm16;
    pcm16.reserve(numFrames * m_waveFormat->Format.nChannels);

    // 转换每个采样点
    for (UINT32 i = 0; i < numFrames * m_waveFormat->Format.nChannels; ++i) {
        float f = floatBuf[i];
        // 裁剪到 [-1.0, 1.0] 范围
        if (f > 1.f)  f = 1.f;
        if (f < -1.f) f = -1.f;
        // 转换为 16-bit PCM
        pcm16.push_back(static_cast<int16_t>(f * 32767));
    }

    // 如果当前不是 16kHz/16bit/单声道，需要进行转换
    std::vector<int16_t> finalBuffer;
    if (m_waveFormat->Format.nSamplesPerSec != 16000 || m_waveFormat->Format.nChannels != 1) {
        // 先进行声道混音（如果需要）
        std::vector<int16_t> monoBuffer;
        if (m_waveFormat->Format.nChannels > 1) {
            monoBuffer.reserve(numFrames);
            for (UINT32 i = 0; i < numFrames; ++i) {
                int32_t sum = 0;
                for (UINT32 ch = 0; ch < m_waveFormat->Format.nChannels; ++ch) {
                    sum += pcm16[i * m_waveFormat->Format.nChannels + ch];
                }
                monoBuffer.push_back(static_cast<int16_t>(sum / m_waveFormat->Format.nChannels));
            }
        } else {
            monoBuffer = std::move(pcm16);
        }

        // 然后进行重采样（如果需要）
        if (m_waveFormat->Format.nSamplesPerSec != 16000) {
            const double ratio = 16000.0 / m_waveFormat->Format.nSamplesPerSec;
            const size_t newSize = static_cast<size_t>(std::ceil(monoBuffer.size() * ratio));
            finalBuffer.reserve(newSize);

            // 线性插值重采样
            for (size_t i = 0; i < newSize; ++i) {
                double pos = i / ratio;
                size_t pos1 = static_cast<size_t>(std::floor(pos));
                size_t pos2 = pos1 + 1;
                double frac = pos - pos1;

                // 边界检查
                if (pos2 >= monoBuffer.size()) {
                    pos2 = monoBuffer.size() - 1;
                }

                // 线性插值
                double sample = monoBuffer[pos1] * (1.0 - frac) + monoBuffer[pos2] * frac;
                finalBuffer.push_back(static_cast<int16_t>(std::round(sample)));
            }
        } else {
            finalBuffer = std::move(monoBuffer);
        }
    } else {
        finalBuffer = std::move(pcm16);
    }

    // 创建输出数据
    QByteArray out(reinterpret_cast<const char*>(finalBuffer.data()), finalBuffer.size() * sizeof(int16_t));
    
    // 使用静态计数器来减少日志输出频率
    static int logCounter = 0;
    if (++logCounter % 100 == 0) {  // 每100个数据块记录一次
        LOG_INFO(QString("处理音频数据：输入 %1 帧，%2 通道，%3 Hz，输出 %4 帧，16kHz/16bit/单声道，输出大小：%5 字节")
                .arg(numFrames)
                .arg(m_waveFormat->Format.nChannels)
                .arg(m_waveFormat->Format.nSamplesPerSec)
                .arg(finalBuffer.size())
                .arg(out.size()));
    }

    emit audioDataReceived(out);
} 