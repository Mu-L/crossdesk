// MyAudioSink.cpp : 定义控制台应用程序的入口点。
//

// #define  _CRT_SECURE_NO_WARNINGS

#include <Audioclient.h>
#include <Devicetopology.h>
#include <Endpointvolume.h>
#include <Mmdeviceapi.h>
#include <tchar.h>

#include <iostream>
//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define EXIT_ON_ERROR(hres) \
  if (FAILED(hres)) {       \
    goto Exit;              \
  }

#define SAFE_RELEASE(punk) \
  if ((punk) != NULL) {    \
    (punk)->Release();     \
    (punk) = NULL;         \
  }

#define IS_INPUT_DEVICE 0  // 切换输入和输出音频设备

#define BUFFER_TIME_100NS (5 * 10000000)

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

const IID IID_IDeviceTopology = __uuidof(IDeviceTopology);
const IID IID_IAudioVolumeLevel = __uuidof(IAudioVolumeLevel);
const IID IID_IPart = __uuidof(IPart);
const IID IID_IConnector = __uuidof(IConnector);
const IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);

class MyAudioSink {
 public:
  // WAVEFORMATEX *pwfx = NULL;
  int SetFormat(WAVEFORMATEX *pwfx);

  int CopyData(SHORT *pData, UINT32 numFramesAvailable, BOOL *pbDone);
};

int MyAudioSink::SetFormat(WAVEFORMATEX *pwfx) {
  printf("wFormatTag is %x\n", pwfx->wFormatTag);
  printf("nChannels is %x\n", pwfx->nChannels);
  printf("nSamplesPerSec is %d\n", pwfx->nSamplesPerSec);
  printf("nAvgBytesPerSec is %d\n", pwfx->nAvgBytesPerSec);
  printf("wBitsPerSample is %d\n", pwfx->wBitsPerSample);

  return 0;
}

FILE *fp;

int MyAudioSink::CopyData(SHORT *pData, UINT32 numFramesAvailable,
                          BOOL *pbDone) {
  if (pData != NULL) {
    size_t t = sizeof(SHORT);
    for (int i = 0; i < numFramesAvailable / t; i++) {
      double dbVal = pData[i];
      pData[i] = dbVal;  // 可以通过不同的分母来控制声音大小
    }
    fwrite(pData, numFramesAvailable, 1, fp);
  }

  return 0;
}

/// pwfx->nSamplesPerSec    = 44100;
/// 不支持修改采样率， 看来只能等得到数据之后再 swr 转换了
BOOL AdjustFormatTo16Bits(WAVEFORMATEX *pwfx) {
  BOOL bRet(FALSE);

  if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = 16;
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

    bRet = TRUE;
  } else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
    if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
      pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
      pEx->Samples.wValidBitsPerSample = 16;
      pwfx->wBitsPerSample = 16;
      pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
      pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

      bRet = TRUE;
    }
  }

  return bRet;
}

typedef unsigned long long uint64_t;
static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;
static inline uint64_t get_clockfreq(void) {
  if (!have_clockfreq) QueryPerformanceFrequency(&clock_freq);
  return clock_freq.QuadPart;
}
uint64_t os_gettime_ns(void) {
  LARGE_INTEGER current_time;
  double time_val;

  QueryPerformanceCounter(&current_time);
  time_val = (double)current_time.QuadPart;
  time_val *= 1000000000.0;
  time_val /= (double)get_clockfreq();

  return (uint64_t)time_val;
}

HRESULT RecordAudioStream(MyAudioSink *pMySink) {
  HRESULT hr;
  REFERENCE_TIME hnsActualDuration;
  UINT32 bufferFrameCount;
  UINT32 numFramesAvailable;
  BYTE *pData;
  DWORD flags;
  REFERENCE_TIME hnsDefaultDevicePeriod(0);

  REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  IAudioClient *pAudioClient = NULL;
  IAudioCaptureClient *pCaptureClient = NULL;
  WAVEFORMATEX *pwfx = NULL;
  UINT32 packetLength = 0;
  BOOL bDone = FALSE;
  HANDLE hTimerWakeUp = NULL;
  UINT64 pos, ts;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                        IID_IMMDeviceEnumerator, (void **)&pEnumerator);

  EXIT_ON_ERROR(hr)

  if (IS_INPUT_DEVICE)
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications,
                                              &pDevice);  // 输入
  else
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole,
                                              &pDevice);  // 输出

  // wchar_t *w_id;
  // os_utf8_to_wcs_ptr(device_id.c_str(), device_id.size(), &w_id);
  // hr = pEnumerator->GetDevice(w_id, &pDevice);
  // bfree(w_id);

  EXIT_ON_ERROR(hr)

  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                         (void **)&pAudioClient);

  EXIT_ON_ERROR(hr)

  hr = pAudioClient->GetMixFormat(&pwfx);

  EXIT_ON_ERROR(hr)

  // The GetDevicePeriod method retrieves the length of the periodic interval
  // separating successive processing passes by the audio engine on the data in
  // the endpoint buffer.
  hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);

  EXIT_ON_ERROR(hr)

  AdjustFormatTo16Bits(pwfx);

  // 平时创建定时器使用的是WINAPI  SetTimer，不过该函数一般用于有界面的时候。
  // 无界面的情况下，可以选择微软提供的CreateWaitableTimer和SetWaitableTimer
  // API。
  hTimerWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);

  DWORD flag;
  if (IS_INPUT_DEVICE)
    flag = 0;
  else
    flag = AUDCLNT_STREAMFLAGS_LOOPBACK;

  if (IS_INPUT_DEVICE)
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flag /*0*/, 0, 0,
                                  pwfx, NULL);  // 输入
  else
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flag /*0*/, 0, 0,
                                  pwfx, NULL);  // 输出

  EXIT_ON_ERROR(hr)

  // Get the size of the allocated buffer.
  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  EXIT_ON_ERROR(hr)

  hr = pAudioClient->GetService(IID_IAudioCaptureClient,
                                (void **)&pCaptureClient);

  EXIT_ON_ERROR(hr)

  LARGE_INTEGER liFirstFire;
  liFirstFire.QuadPart =
      -hnsDefaultDevicePeriod / 2;  // negative means relative time
  LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 /
                           (10 * 1000);  // convert to milliseconds

  BOOL bOK = SetWaitableTimer(hTimerWakeUp, &liFirstFire, lTimeBetweenFires,
                              NULL, NULL, FALSE);

  // Notify the audio sink which format to use.
  hr = pMySink->SetFormat(pwfx);
  EXIT_ON_ERROR(hr)

  // Calculate the actual duration of the allocated buffer.
  hnsActualDuration =
      (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

  /*************************************************************/
  hr = pAudioClient->Start();  // Start recording.
  EXIT_ON_ERROR(hr)
  HANDLE waitArray[1] = {/*htemp hEventStop,*/ hTimerWakeUp};

  // Each loop fills about half of the shared buffer.
  while (bDone == FALSE) {
    // Sleep for half the buffer duration.
    // Sleep(hnsActualDuration/REFTIMES_PER_MILLISEC/2);//这句貌似不加也可以
    // WaitForSingleObject(hTimerWakeUp,INFINITE);
    int a = sizeof(waitArray);
    int aa = sizeof(waitArray[0]);
    WaitForMultipleObjects(sizeof(waitArray) / sizeof(waitArray[0]), waitArray,
                           FALSE, INFINITE);
    // WaitForMultipleObjects(sizeof(waitArray) / sizeof(waitArray[0]),
    // waitArray, FALSE, INFINITE);

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    EXIT_ON_ERROR(hr)

    while (packetLength != 0) {
      // Get the available data in the shared buffer.
      hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL,
                                     &ts);
      ts = ts * 100;
      uint64_t timestamp =
          os_gettime_ns();  // ts是设备时间，timestamp是系统时间
      EXIT_ON_ERROR(hr)

      // 位运算，flags的标志符为2（静音状态）时，将pData置为NULL
      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        pData = NULL;  // Tell CopyData to write silence.
      }

      // Copy the available capture data to the audio sink.
      hr = pMySink->CopyData((SHORT *)pData,
                             numFramesAvailable * pwfx->nBlockAlign, &bDone);
      EXIT_ON_ERROR(hr)

      hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
      EXIT_ON_ERROR(hr)

      hr = pCaptureClient->GetNextPacketSize(&packetLength);
      EXIT_ON_ERROR(hr)
    }
  }

  hr = pAudioClient->Stop();  // Stop recording.
  EXIT_ON_ERROR(hr)

Exit:
  CoTaskMemFree(pwfx);
  SAFE_RELEASE(pEnumerator)
  SAFE_RELEASE(pDevice)
  SAFE_RELEASE(pAudioClient)
  SAFE_RELEASE(pCaptureClient)

  return hr;
}

int _tmain(int argc, _TCHAR *argv[]) {
  fopen_s(&fp, "record.pcm", "wb");
  CoInitialize(NULL);
  MyAudioSink test;

  RecordAudioStream(&test);

  return 0;
}