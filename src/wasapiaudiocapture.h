#ifndef WASAPIAUDIOCAPTURE_H
#define WASAPIAUDIOCAPTURE_H

#include <QObject>
#include <QByteArray>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

class WasapiAudioCapture : public QObject
{
    Q_OBJECT
public:
    explicit WasapiAudioCapture(QObject *parent = nullptr);
    ~WasapiAudioCapture();

    bool startCapture();
    void stopCapture();
    bool isCapturing() const { return m_isCapturing; }

signals:
    void audioDataReceived(const QByteArray &data);
    void error(const QString &message);

private:
    bool initializeWASAPI();
    void cleanupWASAPI();
    static DWORD WINAPI captureThread(LPVOID context);
    void processAudioData(const BYTE* data, UINT32 numFrames);

    IMMDeviceEnumerator* m_deviceEnumerator;
    IMMDevice* m_audioDevice;
    IAudioClient* m_audioClient;
    IAudioCaptureClient* m_captureClient;
    HANDLE m_captureThread;
    bool m_isCapturing;
    WAVEFORMATEX* m_waveFormat;
    UINT32 m_bufferFrameCount;
    static const int SAMPLE_RATE = 16000;
    static const int CHANNELS = 1;
    static const int BITS_PER_SAMPLE = 16;
};

#endif // WASAPIAUDIOCAPTURE_H 