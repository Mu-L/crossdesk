/*
 * @Author: DI JUNKUN
 * @Date: 2024-07-22
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SPEAKER_CAPTURER_WASAPI_H_
#define _SPEAKER_CAPTURER_WASAPI_H_

#include <Audioclient.h>
#include <Devicetopology.h>
#include <Endpointvolume.h>
#include <Mmdeviceapi.h>

#include <thread>
#include <vector>

#include "speaker_capturer.h"

class SpeakerCapturerWasapi : public SpeakerCapturer {
 public:
  SpeakerCapturerWasapi();
  ~SpeakerCapturerWasapi();

 public:
  virtual int Init(speaker_data_cb cb);
  virtual int Destroy();
  virtual int Start();
  virtual int Stop();

  int Pause();
  int Resume();

 private:
  speaker_data_cb cb_ = nullptr;

 private:
  REFERENCE_TIME hnsActualDuration;
  UINT32 bufferFrameCount;
  UINT32 numFramesAvailable;
  BYTE *pData;
  // std::vector<BYTE> pData_dst;
  BYTE pData_dst[960];
  DWORD flags;

  // REFERENCE_TIME hnsRequestedDuration = 10000000;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  IAudioClient *pAudioClient = NULL;
  IAudioCaptureClient *pCaptureClient = NULL;
  WAVEFORMATEX *pwfx = NULL;
  UINT32 packetLength = 0;
  UINT64 pos, ts;
  FILE *fp;

  bool inited_ = false;
  // thread
  std::unique_ptr<std::thread> capture_thread_ = nullptr;
};

#endif