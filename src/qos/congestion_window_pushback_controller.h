/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-13
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CONGESTION_WINDOW_PUSHBACK_CONTROLLER_H_
#define _CONGESTION_WINDOW_PUSHBACK_CONTROLLER_H_

#include <stdint.h>

#include <optional>

// This class enables pushback from congestion window directly to video encoder.
// When the congestion window is filling up, the video encoder target bitrate
// will be reduced accordingly to accommodate the network changes. To avoid
// pausing video too frequently, a minimum encoder target bitrate threshold is
// used to prevent video pause due to a full congestion window.
class CongestionWindowPushbackController {
 public:
  explicit CongestionWindowPushbackController();
  void UpdateOutstandingData(int64_t outstanding_bytes);
  void UpdatePacingQueue(int64_t pacing_bytes);
  uint32_t UpdateTargetBitrate(uint32_t bitrate_bps);
  void SetDataWindow(int64_t data_window);

 private:
  const bool add_pacing_ = true;
  const uint32_t min_pushback_target_bitrate_bps_ = 10000;
  std::optional<int64_t> current_data_window_ = std::nullopt;
  int64_t outstanding_bytes_ = 0;
  int64_t pacing_bytes_ = 0;
  double encoding_rate_ratio_ = 1.0;
};

#endif