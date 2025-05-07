/*
 * @Author: DI JUNKUN
 * @Date: 2025-05-07
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SCREEN_CAPTURER_X11_H_
#define _SCREEN_CAPTURER_X11_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

class ScreenCapturer {
 public:
  typedef std::function<void(unsigned char*, int width, int height, int stride)>
      cb_desktop_data;

  virtual ~ScreenCapturer() {}
  virtual int Init(const int fps, cb_desktop_data cb) = 0;
  virtual int Destroy() = 0;
  virtual int Start() = 0;
  virtual int Stop() = 0;
};

class ScreenCapturerX11 : public ScreenCapturer {
 public:
  ScreenCapturerX11();
  ~ScreenCapturerX11();

 public:
  int Init(const int fps, cb_desktop_data cb) override;
  int Destroy() override;
  int Start() override;
  int Stop() override;

  int Pause();
  int Resume();

  void OnFrame();

 protected:
  void CleanUp();

 private:
  Display* display_ = nullptr;
  Window root_ = 0;
  int width_ = 0;
  int height_ = 0;
  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> paused_{false};
  int fps_ = 30;
  cb_desktop_data callback_;

  // 缓冲区
  std::vector<uint8_t> y_plane_;
  std::vector<uint8_t> uv_plane_;
};

#endif