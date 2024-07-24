/*
 * @Author: DI JUNKUN
 * @Date: 2024-07-22
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SPEAKER_CAPTURER_FACTORY_H_
#define _SPEAKER_CAPTURER_FACTORY_H_

#ifdef _WIN32
#include "speaker_capturer_wasapi.h"
#elif __linux__
#elif __APPLE__
#endif

class SpeakerCapturerFactory {
 public:
  virtual ~SpeakerCapturerFactory() {}

 public:
  SpeakerCapturer* Create() {
#ifdef _WIN32
    return new SpeakerCapturerWasapi();
#elif __linux__
#elif __APPLE__
#else
    return nullptr;
#endif
  }
};

#endif