/*
 * @Author: DI JUNKUN
 * @Date: 2025-05-07
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _MOUSE_CONTROLLER_H_
#define _MOUSE_CONTROLLER_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#include "device_controller.h"

class MouseController : public DeviceController {
 public:
  MouseController();
  virtual ~MouseController();

 public:
  virtual int Init(int screen_width, int screen_height);
  virtual int Destroy();
  virtual int SendMouseCommand(RemoteAction remote_action);

 private:
  void SimulateKeyDown(int kval);
  void SimulateKeyUp(int kval);
  void SetMousePosition(int x, int y);
  void SimulateMouseWheel(int direction_button, int count);

  Display* display_ = nullptr;
  Window root_ = 0;
  int screen_width_ = 0;
  int screen_height_ = 0;
};

#endif