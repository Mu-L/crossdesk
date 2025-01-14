/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "send_side_bandwidth_estimation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "log.h"

namespace {
constexpr int64_t kBweIncreaseInterval = 1000;
constexpr int64_t kBweDecreaseInterval = 300;
constexpr int64_t kStartPhase = 2000;
constexpr int64_t kBweConverganceTime = 20000;
constexpr int kLimitNumPackets = 20;
constexpr int64_t kDefaultMaxBitrate = 1000000000;
constexpr int64_t kLowBitrateLogPeriod = 10000;
constexpr int64_t kRtcEventLogPeriod = 5000;
// Expecting that RTCP feedback is sent uniformly within [0.5, 1.5]s intervals.
constexpr int64_t kMaxRtcpFeedbackInterval = 5000;

constexpr float kDefaultLowLossThreshold = 0.02f;
constexpr float kDefaultHighLossThreshold = 0.1f;
constexpr int64_t kDefaultBitrateThreshold = 0;

constexpr int64_t kCongestionControllerMinBitrate = 5000;

struct UmaRampUpMetric {
  const char* metric_name;
  int bitrate_kbps;
};

const UmaRampUpMetric kUmaRampupMetrics[] = {
    {"WebRTC.BWE.RampUpTimeTo500kbpsInMs", 500},
    {"WebRTC.BWE.RampUpTimeTo1000kbpsInMs", 1000},
    {"WebRTC.BWE.RampUpTimeTo2000kbpsInMs", 2000}};
const size_t kNumUmaRampupMetrics =
    sizeof(kUmaRampupMetrics) / sizeof(kUmaRampupMetrics[0]);
}  // namespace

void LinkCapacityTracker::UpdateDelayBasedEstimate(
    int64_t at_time, int64_t delay_based_bitrate) {
  if (delay_based_bitrate < last_delay_based_estimate_) {
    capacity_estimate_bps_ = std::min(capacity_estimate_bps_,
                                      static_cast<double>(delay_based_bitrate));
    last_link_capacity_update_ = at_time;
  }
  last_delay_based_estimate_ = delay_based_bitrate;
}

void LinkCapacityTracker::OnStartingRate(int64_t start_rate) {
  if (IsInfinite(last_link_capacity_update_)) {
    capacity_estimate_bps_ = start_rate;
  }
}

void LinkCapacityTracker::OnRateUpdate(std::optional<int64_t> acknowledged,
                                       int64_t target, int64_t at_time) {
  if (!acknowledged) return;
  int64_t acknowledged_target = std::min(*acknowledged, target);
  if (acknowledged_target > capacity_estimate_bps_) {
    int64_t delta = at_time - last_link_capacity_update_;
    double alpha = IsFinite(delta) ? exp(-(delta / 10)) : 0;
    capacity_estimate_bps_ =
        alpha * capacity_estimate_bps_ + (1 - alpha) * acknowledged_target;
  }
  last_link_capacity_update_ = at_time;
}

void LinkCapacityTracker::OnRttBackoff(int64_t backoff_rate, int64_t at_time) {
  capacity_estimate_bps_ =
      std::min(capacity_estimate_bps_, static_cast<double>(backoff_rate));
  last_link_capacity_update_ = at_time;
}

int64_t LinkCapacityTracker::estimate() const { return capacity_estimate_bps_; }

RttBasedBackoff::RttBasedBackoff()
    : disabled_(true),
      configured_limit_(3),
      drop_fraction_(0.8),
      drop_interval_(1),
      bandwidth_floor_(5),
      rtt_limit_(INT64_T_MAX),
      // By initializing this to plus infinity, we make sure that we never
      // trigger rtt backoff unless packet feedback is enabled.
      last_propagation_rtt_update_(INT64_T_MAX),
      last_propagation_rtt_(0),
      last_packet_sent_(INT64_T_MIN) {
  if (!disabled_) {
    rtt_limit_ = configured_limit_;
  }
}

void RttBasedBackoff::UpdatePropagationRtt(int64_t at_time,
                                           int64_t propagation_rtt) {
  last_propagation_rtt_update_ = at_time;
  last_propagation_rtt_ = propagation_rtt;
}

bool RttBasedBackoff::IsRttAboveLimit() const {
  return CorrectedRtt() > rtt_limit_;
}

int64_t RttBasedBackoff::CorrectedRtt() const {
  // Avoid timeout when no packets are being sent.
  int64_t timeout_correction =
      std::max(last_packet_sent_ - last_propagation_rtt_update_,
               static_cast<int64_t>(0));
  return timeout_correction + last_propagation_rtt_;
}

RttBasedBackoff::~RttBasedBackoff() = default;

SendSideBandwidthEstimation::SendSideBandwidthEstimation()
    : lost_packets_since_last_loss_update_(0),
      expected_packets_since_last_loss_update_(0),
      current_target_(0),
      last_logged_target_(0),
      min_bitrate_configured_(kCongestionControllerMinBitrate),
      max_bitrate_configured_(kDefaultMaxBitrate),
      last_low_bitrate_log_(INT64_T_MIN),
      has_decreased_since_last_fraction_loss_(false),
      last_loss_feedback_(INT64_T_MIN),
      last_loss_packet_report_(INT64_T_MIN),
      last_fraction_loss_(0),
      last_logged_fraction_loss_(0),
      last_round_trip_time_(0),
      receiver_limit_(INT64_T_MAX),
      delay_based_limit_(INT64_T_MAX),
      time_last_decrease_(INT64_T_MIN),
      first_report_time_(INT64_T_MIN),
      initially_lost_packets_(0),
      bitrate_at_2_seconds_(0),
      uma_update_state_(kNoUpdate),
      uma_rtt_state_(kNoUpdate),
      rampup_uma_stats_updated_(kNumUmaRampupMetrics, false),
      last_rtc_event_log_(INT64_T_MIN),
      low_loss_threshold_(kDefaultLowLossThreshold),
      high_loss_threshold_(kDefaultHighLossThreshold),
      bitrate_threshold_(kDefaultBitrateThreshold),
      disable_receiver_limit_caps_only_(true) {}

SendSideBandwidthEstimation::~SendSideBandwidthEstimation() {}

void SendSideBandwidthEstimation::OnRouteChange() {
  lost_packets_since_last_loss_update_ = 0;
  expected_packets_since_last_loss_update_ = 0;
  current_target_ = 0;
  min_bitrate_configured_ = kCongestionControllerMinBitrate;
  max_bitrate_configured_ = kDefaultMaxBitrate;
  last_low_bitrate_log_ = INT64_T_MIN;
  has_decreased_since_last_fraction_loss_ = false;
  last_loss_feedback_ = INT64_T_MIN;
  last_loss_packet_report_ = INT64_T_MIN;
  last_fraction_loss_ = 0;
  last_logged_fraction_loss_ = 0;
  last_round_trip_time_ = 0;
  receiver_limit_ = INT64_T_MAX;
  delay_based_limit_ = INT64_T_MAX;
  time_last_decrease_ = INT64_T_MIN;
  first_report_time_ = INT64_T_MIN;
  initially_lost_packets_ = 0;
  bitrate_at_2_seconds_ = 0;
  uma_update_state_ = kNoUpdate;
  uma_rtt_state_ = kNoUpdate;
  last_rtc_event_log_ = INT64_T_MIN;
}

void SendSideBandwidthEstimation::SetBitrates(
    std::optional<int64_t> send_bitrate, int64_t min_bitrate,
    int64_t max_bitrate, int64_t at_time) {
  SetMinMaxBitrate(min_bitrate, max_bitrate);
  if (send_bitrate) {
    link_capacity_.OnStartingRate(*send_bitrate);
    SetSendBitrate(*send_bitrate, at_time);
  }
}

void SendSideBandwidthEstimation::SetSendBitrate(int64_t bitrate,
                                                 int64_t at_time) {
  // Reset to avoid being capped by the estimate.
  delay_based_limit_ = INT64_T_MAX;
  UpdateTargetBitrate(bitrate, at_time);
  // Clear last sent bitrate history so the new value can be used directly
  // and not capped.
  min_bitrate_history_.clear();
}

void SendSideBandwidthEstimation::SetMinMaxBitrate(int64_t min_bitrate,
                                                   int64_t max_bitrate) {
  min_bitrate_configured_ =
      std::max(min_bitrate, kCongestionControllerMinBitrate);
  if (max_bitrate > 0 && IsFinite(max_bitrate)) {
    max_bitrate_configured_ = std::max(min_bitrate_configured_, max_bitrate);
  } else {
    max_bitrate_configured_ = kDefaultMaxBitrate;
  }
}

int SendSideBandwidthEstimation::GetMinBitrate() const {
  return min_bitrate_configured_;
}

int64_t SendSideBandwidthEstimation::target_rate() const {
  int64_t target = current_target_;
  if (!disable_receiver_limit_caps_only_)
    target = std::min(target, receiver_limit_);
  return std::max(min_bitrate_configured_, target);
}

bool SendSideBandwidthEstimation::IsRttAboveLimit() const {
  return rtt_backoff_.IsRttAboveLimit();
}

int64_t SendSideBandwidthEstimation::GetEstimatedLinkCapacity() const {
  return link_capacity_.estimate();
}

void SendSideBandwidthEstimation::UpdateReceiverEstimate(int64_t at_time,
                                                         int64_t bandwidth) {
  // TODO(srte): Ensure caller passes PlusInfinity, not zero, to represent no
  // limitation.
  receiver_limit_ = !bandwidth ? INT64_T_MAX : bandwidth;
  ApplyTargetLimits(at_time);
}

void SendSideBandwidthEstimation::UpdateDelayBasedEstimate(int64_t at_time,
                                                           int64_t bitrate) {
  link_capacity_.UpdateDelayBasedEstimate(at_time, bitrate);
  // TODO(srte): Ensure caller passes PlusInfinity, not zero, to represent no
  // limitation.
  delay_based_limit_ = !bitrate ? INT64_T_MAX : bitrate;
  ApplyTargetLimits(at_time);
}

void SendSideBandwidthEstimation::SetAcknowledgedRate(
    std::optional<int64_t> acknowledged_rate, int64_t at_time) {
  acknowledged_rate_ = acknowledged_rate;
  if (!acknowledged_rate.has_value()) {
    return;
  }
}

void SendSideBandwidthEstimation::UpdatePacketsLost(int64_t packets_lost,
                                                    int64_t number_of_packets,
                                                    int64_t at_time) {
  last_loss_feedback_ = at_time;
  if (IsInfinite(first_report_time_)) {
    first_report_time_ = at_time;
  }

  // Check sequence number diff and weight loss report
  if (number_of_packets > 0) {
    int64_t expected =
        expected_packets_since_last_loss_update_ + number_of_packets;

    // Don't generate a loss rate until it can be based on enough packets.
    if (expected < kLimitNumPackets) {
      // Accumulate reports.
      expected_packets_since_last_loss_update_ = expected;
      lost_packets_since_last_loss_update_ += packets_lost;
      return;
    }

    has_decreased_since_last_fraction_loss_ = false;
    int64_t lost_q8 =
        std::max<int64_t>(lost_packets_since_last_loss_update_ + packets_lost,
                          static_cast<int64_t>(0))
        << 8;
    last_fraction_loss_ = std::min<int>(lost_q8 / expected, 255);

    // Reset accumulators.
    lost_packets_since_last_loss_update_ = 0;
    expected_packets_since_last_loss_update_ = 0;
    last_loss_packet_report_ = at_time;
    UpdateEstimate(at_time);
  }

  UpdateUmaStatsPacketsLost(at_time, packets_lost);
}

void SendSideBandwidthEstimation::UpdateUmaStatsPacketsLost(int64_t at_time,
                                                            int packets_lost) {
  int64_t bitrate_kbps = (current_target_ + 500) / 1000;
  for (size_t i = 0; i < kNumUmaRampupMetrics; ++i) {
    if (!rampup_uma_stats_updated_[i] &&
        bitrate_kbps >= kUmaRampupMetrics[i].bitrate_kbps) {
      rampup_uma_stats_updated_[i] = true;
    }
  }
  if (IsInStartPhase(at_time)) {
    initially_lost_packets_ += packets_lost;
  } else if (uma_update_state_ == kNoUpdate) {
    uma_update_state_ = kFirstDone;
    bitrate_at_2_seconds_ = bitrate_kbps;
  } else if (uma_update_state_ == kFirstDone &&
             at_time - first_report_time_ >= kBweConverganceTime) {
    uma_update_state_ = kDone;
    int bitrate_diff_kbps =
        std::max(bitrate_at_2_seconds_ - bitrate_kbps, static_cast<int64_t>(0));
  }
}

void SendSideBandwidthEstimation::UpdateRtt(int64_t rtt, int64_t at_time) {
  // Update RTT if we were able to compute an RTT based on this RTCP.
  // FlexFEC doesn't send RTCP SR, which means we won't be able to compute RTT.
  if (rtt > 0) last_round_trip_time_ = rtt;

  if (!IsInStartPhase(at_time) && uma_rtt_state_ == kNoUpdate) {
    uma_rtt_state_ = kDone;
  }
}

void SendSideBandwidthEstimation::UpdateEstimate(int64_t at_time) {
  if (rtt_backoff_.IsRttAboveLimit()) {
    if (at_time - time_last_decrease_ >= rtt_backoff_.drop_interval_ &&
        current_target_ > rtt_backoff_.bandwidth_floor_) {
      time_last_decrease_ = at_time;
      int64_t new_bitrate =
          std::max(current_target_ * rtt_backoff_.drop_fraction_,
                   static_cast<double>(rtt_backoff_.bandwidth_floor_));
      link_capacity_.OnRttBackoff(new_bitrate, at_time);
      UpdateTargetBitrate(new_bitrate, at_time);
      return;
    }
    // TODO(srte): This is likely redundant in most cases.
    ApplyTargetLimits(at_time);
    return;
  }

  // We trust the REMB and/or delay-based estimate during the first 2 seconds if
  // we haven't had any packet loss reported, to allow startup bitrate probing.
  if (last_fraction_loss_ == 0 && IsInStartPhase(at_time)) {
    int64_t new_bitrate = current_target_;
    // TODO(srte): We should not allow the new_bitrate to be larger than the
    // receiver limit here.
    if (IsFinite(receiver_limit_)) {
      new_bitrate = std::max(receiver_limit_, new_bitrate);
    }
    if (IsFinite(delay_based_limit_)) {
      new_bitrate = std::max(delay_based_limit_, new_bitrate);
    }
    if (new_bitrate != current_target_) {
      min_bitrate_history_.clear();
      min_bitrate_history_.push_back(std::make_pair(at_time, current_target_));
      UpdateTargetBitrate(new_bitrate, at_time);
      return;
    }
  }
  UpdateMinHistory(at_time);
  if (IsInfinite(last_loss_packet_report_)) {
    // No feedback received.
    // TODO(srte): This is likely redundant in most cases.
    ApplyTargetLimits(at_time);
    return;
  }

  int64_t time_since_loss_packet_report = at_time - last_loss_packet_report_;
  if (time_since_loss_packet_report < 1.2 * kMaxRtcpFeedbackInterval) {
    // We only care about loss above a given bitrate threshold.
    float loss = last_fraction_loss_ / 256.0f;
    // We only make decisions based on loss when the bitrate is above a
    // threshold. This is a crude way of handling loss which is uncorrelated
    // to congestion.
    if (current_target_ < bitrate_threshold_ || loss <= low_loss_threshold_) {
      // Loss < 2%: Increase rate by 8% of the min bitrate in the last
      // kBweIncreaseInterval.
      // Note that by remembering the bitrate over the last second one can
      // rampup up one second faster than if only allowed to start ramping
      // at 8% per second rate now. E.g.:
      //   If sending a constant 100kbps it can rampup immediately to 108kbps
      //   whenever a receiver report is received with lower packet loss.
      //   If instead one would do: current_bitrate_ *= 1.08^(delta time),
      //   it would take over one second since the lower packet loss to achieve
      //   108kbps.
      int64_t new_bitrate = min_bitrate_history_.front().second * 1.08 + 0.5;

      // Add 1 kbps extra, just to make sure that we do not get stuck
      // (gives a little extra increase at low rates, negligible at higher
      // rates).
      new_bitrate += 1000;
      UpdateTargetBitrate(new_bitrate, at_time);
      return;
    } else if (current_target_ > bitrate_threshold_) {
      if (loss <= high_loss_threshold_) {
        // Loss between 2% - 10%: Do nothing.
      } else {
        // Loss > 10%: Limit the rate decreases to once a kBweDecreaseInterval
        // + rtt.
        if (!has_decreased_since_last_fraction_loss_ &&
            (at_time - time_last_decrease_) >=
                (kBweDecreaseInterval + last_round_trip_time_)) {
          time_last_decrease_ = at_time;

          // Reduce rate:
          //   newRate = rate * (1 - 0.5*lossRate);
          //   where packetLoss = 256*lossRate;
          int64_t new_bitrate =
              (current_target_ *
               static_cast<double>(512 - last_fraction_loss_)) /
              512.0;
          has_decreased_since_last_fraction_loss_ = true;
          UpdateTargetBitrate(new_bitrate, at_time);
          return;
        }
      }
    }
  }
  // TODO(srte): This is likely redundant in most cases.
  ApplyTargetLimits(at_time);
}

void SendSideBandwidthEstimation::UpdatePropagationRtt(
    int64_t at_time, int64_t propagation_rtt) {
  rtt_backoff_.UpdatePropagationRtt(at_time, propagation_rtt);
}

void SendSideBandwidthEstimation::OnSentPacket(const SentPacket& sent_packet) {
  // Only feedback-triggering packets will be reported here.
  rtt_backoff_.last_packet_sent_ = sent_packet.send_time;
}

bool SendSideBandwidthEstimation::IsInStartPhase(int64_t at_time) const {
  return (IsInfinite(first_report_time_)) ||
         at_time - first_report_time_ < kStartPhase;
}

void SendSideBandwidthEstimation::UpdateMinHistory(int64_t at_time) {
  // Remove old data points from history.
  // Since history precision is in ms, add one so it is able to increase
  // bitrate if it is off by as little as 0.5ms.
  while (!min_bitrate_history_.empty() &&
         at_time - min_bitrate_history_.front().first + 1 >
             kBweIncreaseInterval) {
    min_bitrate_history_.pop_front();
  }

  // Typical minimum sliding-window algorithm: Pop values higher than current
  // bitrate before pushing it.
  while (!min_bitrate_history_.empty() &&
         current_target_ <= min_bitrate_history_.back().second) {
    min_bitrate_history_.pop_back();
  }

  min_bitrate_history_.push_back(std::make_pair(at_time, current_target_));
}

int64_t SendSideBandwidthEstimation::GetUpperLimit() const {
  int64_t upper_limit = delay_based_limit_;
  if (disable_receiver_limit_caps_only_)
    upper_limit = std::min(upper_limit, receiver_limit_);
  return std::min(upper_limit, max_bitrate_configured_);
}

void SendSideBandwidthEstimation::MaybeLogLowBitrateWarning(int64_t bitrate,
                                                            int64_t at_time) {
  if (at_time - last_low_bitrate_log_ > kLowBitrateLogPeriod) {
    LOG_WARN(
        "Estimated available bandwidth {} is below configured min bitrate {}",
        bitrate, min_bitrate_configured_);
    last_low_bitrate_log_ = at_time;
  }
}

void SendSideBandwidthEstimation::MaybeLogLossBasedEvent(int64_t at_time) {
  if (current_target_ != last_logged_target_ ||
      last_fraction_loss_ != last_logged_fraction_loss_ ||
      at_time - last_rtc_event_log_ > kRtcEventLogPeriod) {
    last_logged_fraction_loss_ = last_fraction_loss_;
    last_logged_target_ = current_target_;
    last_rtc_event_log_ = at_time;
  }
}

void SendSideBandwidthEstimation::UpdateTargetBitrate(int64_t new_bitrate,
                                                      int64_t at_time) {
  new_bitrate = std::min(new_bitrate, GetUpperLimit());
  if (new_bitrate < min_bitrate_configured_) {
    MaybeLogLowBitrateWarning(new_bitrate, at_time);
    new_bitrate = min_bitrate_configured_;
  }
  current_target_ = new_bitrate;
  MaybeLogLossBasedEvent(at_time);
  link_capacity_.OnRateUpdate(acknowledged_rate_, current_target_, at_time);
}

void SendSideBandwidthEstimation::ApplyTargetLimits(int64_t at_time) {
  UpdateTargetBitrate(current_target_, at_time);
}
