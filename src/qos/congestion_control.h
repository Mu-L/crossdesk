#ifndef _CONGESTION_CONTROL_H_
#define _CONGESTION_CONTROL_H_

#include <deque>
#include <memory>

#include "acknowledged_bitrate_estimator.h"
#include "alr_detector.h"
#include "congestion_window_pushback_controller.h"
#include "network_types.h"
#include "send_side_bandwidth_estimation.h"

class CongestionControl {
 public:
  CongestionControl();
  ~CongestionControl();

 public:
  NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback report);

 private:
  const std::unique_ptr<CongestionWindowPushbackController>
      congestion_window_pushback_controller_;

 private:
  std::deque<int64_t> feedback_max_rtts_;
  int expected_packets_since_last_loss_update_ = 0;
  int lost_packets_since_last_loss_update_ = 0;
  int64_t next_loss_update_ = std::numeric_limits<int64_t>::min();
  const bool packet_feedback_only_ = false;
  bool previously_in_alr_ = false;
  const bool limit_probes_lower_than_throughput_estimate_ = false;
  std::optional<int64_t> current_data_window_;

 private:
  std::unique_ptr<SendSideBandwidthEstimation> bandwidth_estimation_;
  std::unique_ptr<AlrDetector> alr_detector_;
  std::unique_ptr<AcknowledgedBitrateEstimator> acknowledged_bitrate_estimator_;
  std::unique_ptr<DelayBasedBwe> delay_based_bwe_;
};

#endif