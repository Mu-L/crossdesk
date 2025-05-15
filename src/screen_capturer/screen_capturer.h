/*
 * @Author: DI JUNKUN
 * @Date: 2023-12-15
 * Copyright (c) 2023 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SCREEN_CAPTURER_H_
#define _SCREEN_CAPTURER_H_

#include <functional>

class ScreenCapturer {
 public:
  typedef std::function<void(unsigned char*, int, int, int, int)>
      cb_desktop_data;

  class DisplayInfo {
   public:
    DisplayInfo(std::string name, int left, int top, int right, int bottom)
        : name(name), left(left), top(top), right(right), bottom(bottom) {}
    DisplayInfo(void* handle, std::string name, bool is_primary, int left,
                int top, int right, int bottom)
        : handle(handle),
          name(name),
          is_primary(is_primary),
          left(left),
          top(top),
          right(right),
          bottom(bottom) {}
    ~DisplayInfo() {}

    void* handle = nullptr;
    std::string name = "";
    bool is_primary = false;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
  };

 public:
  virtual ~ScreenCapturer() {}

 public:
  virtual int Init(const int fps, cb_desktop_data cb) = 0;
  virtual int Destroy() = 0;
  virtual int Start() = 0;
  virtual int Stop() = 0;
  virtual int Pause(int monitor_index) = 0;
  virtual int Resume(int monitor_index) = 0;

  virtual std::vector<DisplayInfo> GetDisplayInfoList() = 0;
  virtual int SwitchTo(int monitor_index) = 0;
};

#endif