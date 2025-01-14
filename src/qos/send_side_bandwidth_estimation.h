/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-13
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SEND_SIDE_BANDWIDTH_ESTIMATION_H_
#define _SEND_SIDE_BANDWIDTH_ESTIMATION_H_

#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "limits_base.h"
#include "network_types.h"

class LinkCapacityTracker {
 public:
  LinkCapacityTracker() = default;
  ~LinkCapacityTracker() = default;
  // Call when a new delay-based estimate is available.
  void UpdateDelayBasedEstimate(int64_t at_time, int64_t delay_based_bitrate);
  void OnStartingRate(int64_t start_rate);
  void OnRateUpdate(std::optional<int64_t> acknowledged, int64_t target,
                    int64_t at_time);
  void OnRttBackoff(int64_t backoff_rate, int64_t at_time);
  int64_t estimate() const;

 private:
  double capacity_estimate_bps_ = 0;
  int64_t last_link_capacity_update_ = INT64_T_MIN;
  int64_t last_delay_based_estimate_ = INT64_T_MAX;
};

class RttBasedBackoff {
 public:
  explicit RttBasedBackoff();
  ~RttBasedBackoff();
  void UpdatePropagationRtt(int64_t at_time, int64_t propagation_rtt);
  bool IsRttAboveLimit() const;

  bool disabled_;
  int64_t configured_limit_;
  double drop_fraction_;
  int64_t drop_interval_;
  int64_t bandwidth_floor_;

 public:
  int64_t rtt_limit_;
  int64_t last_propagation_rtt_update_;
  int64_t last_propagation_rtt_;
  int64_t last_packet_sent_;

 private:
  int64_t CorrectedRtt() const;
};

class SendSideBandwidthEstimation {
 public:
  SendSideBandwidthEstimation();
  ~SendSideBandwidthEstimation();

  void OnRouteChange();

  int64_t target_rate() const;
  // Return whether the current rtt is higher than the rtt limited configured in
  // RttBasedBackoff.
  bool IsRttAboveLimit() const;
  uint8_t fraction_loss() const { return last_fraction_loss_; }
  int64_t round_trip_time() const { return last_round_trip_time_; }

  int64_t GetEstimatedLinkCapacity() const;
  // Call periodically to update estimate.
  void UpdateEstimate(int64_t at_time);
  void OnSentPacket(const SentPacket& sent_packet);
  void UpdatePropagationRtt(int64_t at_time, int64_t propagation_rtt);

  // Call when we receive a RTCP message with TMMBR or REMB.
  void UpdateReceiverEstimate(int64_t at_time, int64_t bandwidth);

  // Call when a new delay-based estimate is available.
  void UpdateDelayBasedEstimate(int64_t at_time, int64_t bitrate);

  // Call when we receive a RTCP message with a ReceiveBlock.
  void UpdatePacketsLost(int64_t packets_lost, int64_t number_of_packets,
                         int64_t at_time);

  // Call when we receive a RTCP message with a ReceiveBlock.
  void UpdateRtt(int64_t rtt, int64_t at_time);

  void SetBitrates(std::optional<int64_t> send_bitrate, int64_t min_bitrate,
                   int64_t max_bitrate, int64_t at_time);
  void SetSendBitrate(int64_t bitrate, int64_t at_time);
  void SetMinMaxBitrate(int64_t min_bitrate, int64_t max_bitrate);
  int GetMinBitrate() const;
  void SetAcknowledgedRate(std::optional<int64_t> acknowledged_rate,
                           int64_t at_time);

 private:
  friend class GoogCcStatePrinter;

  enum UmaState { kNoUpdate, kFirstDone, kDone };

  bool IsInStartPhase(int64_t at_time) const;

  void UpdateUmaStatsPacketsLost(int64_t at_time, int packets_lost);

  // Updates history of min bitrates.
  // After this method returns min_bitrate_history_.front().second contains the
  // min bitrate used during last kBweIncreaseIntervalMs.
  void UpdateMinHistory(int64_t at_time);

  // Gets the upper limit for the target bitrate. This is the minimum of the
  // delay based limit, the receiver limit and the loss based controller limit.
  int64_t GetUpperLimit() const;
  // Prints a warning if `bitrate` if sufficiently long time has past since last
  // warning.
  void MaybeLogLowBitrateWarning(int64_t bitrate, int64_t at_time);
  // Stores an update to the event log if the loss rate has changed, the target
  // has changed, or sufficient time has passed since last stored event.
  void MaybeLogLossBasedEvent(int64_t at_time);

  // Cap `bitrate` to [min_bitrate_configured_, max_bitrate_configured_] and
  // set `current_bitrate_` to the capped value and updates the event log.
  void UpdateTargetBitrate(int64_t bitrate, int64_t at_time);
  // Applies lower and upper bounds to the current target rate.
  // TODO(srte): This seems to be called even when limits haven't changed, that
  // should be cleaned up.
  void ApplyTargetLimits(int64_t at_time);

  RttBasedBackoff rtt_backoff_;
  LinkCapacityTracker link_capacity_;

  std::deque<std::pair<int64_t, int64_t> > min_bitrate_history_;

  // incoming filters
  int lost_packets_since_last_loss_update_;
  int expected_packets_since_last_loss_update_;

  std::optional<int64_t> acknowledged_rate_;
  int64_t current_target_;
  int64_t last_logged_target_;
  int64_t min_bitrate_configured_;
  int64_t max_bitrate_configured_;
  int64_t last_low_bitrate_log_;

  bool has_decreased_since_last_fraction_loss_;
  int64_t last_loss_feedback_;
  int64_t last_loss_packet_report_;
  uint8_t last_fraction_loss_;
  uint8_t last_logged_fraction_loss_;
  int64_t last_round_trip_time_;

  // The max bitrate as set by the receiver in the call. This is typically
  // signalled using the REMB RTCP message and is used when we don't have any
  // send side delay based estimate.
  int64_t receiver_limit_;
  int64_t delay_based_limit_;
  int64_t time_last_decrease_;
  int64_t first_report_time_;
  int initially_lost_packets_;
  int64_t bitrate_at_2_seconds_;
  UmaState uma_update_state_;
  UmaState uma_rtt_state_;
  std::vector<bool> rampup_uma_stats_updated_;
  int64_t last_rtc_event_log_;
  float low_loss_threshold_;
  float high_loss_threshold_;
  int64_t bitrate_threshold_;
  bool disable_receiver_limit_caps_only_;
};

#endif