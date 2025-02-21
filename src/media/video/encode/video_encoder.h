#ifndef _VIDEO_ENCODER_H_
#define _VIDEO_ENCODER_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>

#include "video_frame_wrapper.h"
#include "x.h"

class VideoEncoder {
 public:
  virtual int Init() = 0;

  virtual int Encode(
      const XVideoFrame* video_frame,
      std::function<int(std::shared_ptr<VideoFrameWrapper> encoded_frame)>
          on_encoded_image) = 0;

  virtual int ForceIdr() = 0;

  virtual int SetTargetBitrate(int bitrate) = 0;

  virtual std::string GetEncoderName() = 0;

  VideoEncoder() = default;
  virtual ~VideoEncoder() {}
};

#endif