#include "speaker_capturer_wasapi.h"

#include <algorithm>
#include <climits>
#include <iostream>

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define SAVE_AUDIO_FILE 0

#define CHECK_HR(hres) \
  if (FAILED(hres)) {  \
    return -1;         \
  }

#define SAFE_RELEASE(punk) \
  if ((punk) != nullptr) { \
    (punk)->Release();     \
    (punk) = nullptr;      \
  }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

SpeakerCapturerWasapi::SpeakerCapturerWasapi() {}

SpeakerCapturerWasapi::~SpeakerCapturerWasapi() {
  if (inited_ && capture_thread_->joinable()) {
    capture_thread_->join();
    inited_ = false;
  }

  CoTaskMemFree(pwfx);
  SAFE_RELEASE(pEnumerator)
  SAFE_RELEASE(pDevice)
  SAFE_RELEASE(pAudioClient)
  SAFE_RELEASE(pCaptureClient)

  if (SAVE_AUDIO_FILE) {
    fclose(fp);
  }

  // if (pData_dst) delete pData_dst;
  // pData_dst = nullptr;
}

int SpeakerCapturerWasapi::Init(speaker_data_cb cb) {
  cb_ = cb;

  if (SAVE_AUDIO_FILE) {
    fopen_s(&fp, "system_audio.pcm", "wb");
  }

  HRESULT hr;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                        IID_IMMDeviceEnumerator, (void **)&pEnumerator);
  CHECK_HR(hr)

  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole,
                                            &pDevice);  // 输出
  CHECK_HR(hr)

  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr,
                         (void **)&pAudioClient);
  CHECK_HR(hr)

  hr = pAudioClient->GetMixFormat(&pwfx);
  CHECK_HR(hr)

  // Change to 16bit
  if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = 16;
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
  } else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
    if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
      pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
      pEx->Samples.wValidBitsPerSample = 16;
      pwfx->wBitsPerSample = 16;
      pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
      pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
    }
  }

  hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx,
                                nullptr);
  CHECK_HR(hr)

  // Get the size of the allocated buffer.
  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  CHECK_HR(hr)

  hr = pAudioClient->GetService(IID_IAudioCaptureClient,
                                (void **)&pCaptureClient);
  CHECK_HR(hr)

  // Show audio info
  {
    printf("wFormatTag is %x\n", pwfx->wFormatTag);
    printf("nChannels is %x\n", pwfx->nChannels);
    printf("nSamplesPerSec is %d\n", pwfx->nSamplesPerSec);
    printf("nAvgBytesPerSec is %d\n", pwfx->nAvgBytesPerSec);
    printf("wBitsPerSample is %d\n", pwfx->wBitsPerSample);
  }

  hnsActualDuration =
      (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

  // pData_dst = new BYTE[960];

  inited_ = true;

  return 0;
}

int SpeakerCapturerWasapi::Start() {
  HRESULT hr;
  hr = pAudioClient->Start();
  CHECK_HR(hr)

  capture_thread_.reset(new std::thread([this]() {
    HRESULT hr;

    // Each loop fills about half of the shared buffer.
    while (1) {
      // Sleep for half the buffer duration.
      Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 4);

      hr = pCaptureClient->GetNextPacketSize(&packetLength);
      CHECK_HR(hr)

      while (packetLength != 0) {
        // Get the available data in the shared buffer.
        hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags,
                                       nullptr, &ts);
        CHECK_HR(hr)

        // flags equals to 2 means silence, set data to nullptr
        if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
          pData = nullptr;
        }

        if (pData != nullptr) {
          size_t size = numFramesAvailable * pwfx->nBlockAlign;

          for (int i = 0; i < size / 2; i++) {
            BYTE left = pData[i * 2];
            BYTE right = pData[i * 2 + 1];
            // Right channel only?
            BYTE monoSample = right;

            pData_dst[i] = static_cast<BYTE>(monoSample);
          }

          cb_(pData_dst, size / 2);

          if (SAVE_AUDIO_FILE) {
            fwrite(pData_dst, size / 2, 1, fp);
          }
        }

        hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
        CHECK_HR(hr)

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        CHECK_HR(hr)
      }
    }
  }));

  return 0;
}

int SpeakerCapturerWasapi::Stop() {
  HRESULT hr;
  hr = pAudioClient->Stop();
  CHECK_HR(hr)
  return 0;
}

int SpeakerCapturerWasapi::Destroy() { return 0; }

int SpeakerCapturerWasapi::Pause() { return 0; }
