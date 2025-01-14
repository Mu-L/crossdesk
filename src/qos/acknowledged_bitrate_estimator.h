/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _ACKNOWLEDGED_BITRATE_ESTIMATOR_H_
#define _ACKNOWLEDGED_BITRATE_ESTIMATOR_H_

#include <memory>
#include <optional>
#include <vector>

#include "bitrate_estimator.h"
#include "network_types.h"

class AcknowledgedBitrateEstimator {
 public:
  AcknowledgedBitrateEstimator(
      std::unique_ptr<BitrateEstimator> bitrate_estimator);

  explicit AcknowledgedBitrateEstimator();
  ~AcknowledgedBitrateEstimator();

  void IncomingPacketFeedbackVector(
      const std::vector<PacketResult>& packet_feedback_vector);
  std::optional<int64_t> bitrate() const;
  std::optional<int64_t> PeekRate() const;
  void SetAlr(bool in_alr);
  void SetAlrEndedTime(int64_t alr_ended_time);

 private:
  std::optional<int64_t> alr_ended_time_;
  bool in_alr_;
  std::unique_ptr<BitrateEstimator> bitrate_estimator_;
};

#endif