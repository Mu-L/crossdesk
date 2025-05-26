#include "screen_capturer_x11.h"

#include <chrono>
#include <thread>

#include "libyuv.h"
#include "rd_log.h"

ScreenCapturerX11::ScreenCapturerX11() {}

ScreenCapturerX11::~ScreenCapturerX11() { Destroy(); }

int ScreenCapturerX11::Init(const int fps, cb_desktop_data cb) {
  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    LOG_ERROR("Cannot connect to X server");
    return -1;
  }

  root_ = DefaultRootWindow(display_);
  XWindowAttributes attr;
  XGetWindowAttributes(display_, root_, &attr);

  width_ = attr.width;
  height_ = attr.height;

  if (width_ % 2 != 0 || height_ % 2 != 0) {
    LOG_ERROR("Width and height must be even numbers");
    return -2;
  }

  fps_ = fps;
  callback_ = cb;

  y_plane_.resize(width_ * height_);
  uv_plane_.resize((width_ / 2) * (height_ / 2) * 2);

  return 0;
}

int ScreenCapturerX11::Destroy() {
  Stop();
  CleanUp();
  return 0;
}

int ScreenCapturerX11::Start() {
  if (running_) return 0;
  running_ = true;
  paused_ = false;
  thread_ = std::thread([this]() {
    while (running_) {
      if (!paused_) OnFrame();
    }
  });
  return 0;
}

int ScreenCapturerX11::Stop() {
  if (!running_) return 0;
  running_ = false;
  if (thread_.joinable()) thread_.join();
  return 0;
}

int ScreenCapturerX11::Pause() {
  paused_ = true;
  return 0;
}

int ScreenCapturerX11::Resume() {
  paused_ = false;
  return 0;
}

void ScreenCapturerX11::OnFrame() {
  if (!display_) return;

  XImage* image =
      XGetImage(display_, root_, 0, 0, width_, height_, AllPlanes, ZPixmap);
  if (!image) return;

  bool needs_copy = image->bytes_per_line != width_ * 4;
  std::vector<uint8_t> argb_buf;
  uint8_t* src_argb = nullptr;

  if (needs_copy) {
    argb_buf.resize(width_ * height_ * 4);
    for (int y = 0; y < height_; ++y) {
      memcpy(&argb_buf[y * width_ * 4], image->data + y * image->bytes_per_line,
             width_ * 4);
    }
    src_argb = argb_buf.data();
  } else {
    src_argb = reinterpret_cast<uint8_t*>(image->data);
  }

  libyuv::ARGBToNV12(src_argb, width_ * 4, y_plane_.data(), width_,
                     uv_plane_.data(), width_, width_, height_);

  std::vector<uint8_t> nv12;
  nv12.reserve(y_plane_.size() + uv_plane_.size());
  nv12.insert(nv12.end(), y_plane_.begin(), y_plane_.end());
  nv12.insert(nv12.end(), uv_plane_.begin(), uv_plane_.end());

  if (callback_) {
    callback_(nv12.data(), width_ * height_ * 3 / 2, width_, height_, "");
  }

  XDestroyImage(image);
}

void ScreenCapturerX11::CleanUp() {
  if (display_) {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}