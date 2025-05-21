#include "wasapiaudiocapture.h"
#include <QDebug>

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
{
}

WasapiAudioCapture::~WasapiAudioCapture()
{
    stopCapture();
    cleanupWASAPI();
}

bool WasapiAudioCapture::initializeWASAPI()
{
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                 nullptr,
                                 CLSCTX_ALL,
                                 __uuidof(IMMDeviceEnumerator),
                                 (void**)&m_deviceEnumerator);
    if (FAILED(hr)) {
        emit error("无法创建设备枚举器");
        return false;
    }

    // 获取默认音频输出设备
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_audioDevice);
    if (FAILED(hr)) {
        emit error("无法获取默认音频输出设备");
        return false;
    }

    // 获取音频客户端
    hr = m_audioDevice->Activate(__uuidof(IAudioClient),
                                CLSCTX_ALL,
                                nullptr,
                                (void**)&m_audioClient);
    if (FAILED(hr)) {
        emit error("无法激活音频客户端");
        return false;
    }

    // 设置音频格式
    m_waveFormat = new WAVEFORMATEX();
    m_waveFormat->wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat->nChannels = CHANNELS;
    m_waveFormat->nSamplesPerSec = SAMPLE_RATE;
    m_waveFormat->wBitsPerSample = BITS_PER_SAMPLE;
    m_waveFormat->nBlockAlign = (m_waveFormat->nChannels * m_waveFormat->wBitsPerSample) / 8;
    m_waveFormat->nAvgBytesPerSec = m_waveFormat->nSamplesPerSec * m_waveFormat->nBlockAlign;
    m_waveFormat->cbSize = 0;

    // 初始化音频客户端
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK,
                                 0,
                                 0,
                                 m_waveFormat,
                                 nullptr);
    if (FAILED(hr)) {
        emit error("无法初始化音频客户端");
        return false;
    }

    // 获取缓冲区大小
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) {
        emit error("无法获取缓冲区大小");
        return false;
    }

    // 获取捕获客户端
    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient),
                                  (void**)&m_captureClient);
    if (FAILED(hr)) {
        emit error("无法获取捕获客户端");
        return false;
    }

    return true;
}

void WasapiAudioCapture::cleanupWASAPI()
{
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
        delete m_waveFormat;
        m_waveFormat = nullptr;
    }
}

bool WasapiAudioCapture::startCapture()
{
    if (m_isCapturing) {
        return true;
    }

    if (!initializeWASAPI()) {
        return false;
    }

    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr)) {
        emit error("无法启动音频捕获");
        return false;
    }

    m_isCapturing = true;
    m_captureThread = CreateThread(nullptr, 0, captureThread, this, 0, nullptr);
    if (!m_captureThread) {
        emit error("无法创建捕获线程");
        stopCapture();
        return false;
    }

    return true;
}

void WasapiAudioCapture::stopCapture()
{
    if (!m_isCapturing) {
        return;
    }

    m_isCapturing = false;
    if (m_captureThread) {
        WaitForSingleObject(m_captureThread, INFINITE);
        CloseHandle(m_captureThread);
        m_captureThread = nullptr;
    }

    if (m_audioClient) {
        m_audioClient->Stop();
    }
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

    UINT32 dataSize = numFrames * m_waveFormat->nBlockAlign;
    QByteArray audioData(reinterpret_cast<const char*>(data), dataSize);
    emit audioDataReceived(audioData);
} 