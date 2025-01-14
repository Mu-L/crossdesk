/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _DELAY_BASED_BWE_H_
#define _DELAY_BASED_BWE_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <vector>

#include "network_types.h"

enum class BandwidthUsage {
  kBwNormal = 0,
  kBwUnderusing = 1,
  kBwOverusing = 2,
  kLast
};

struct BweSeparateAudioPacketsSettings {
  static constexpr char kKey[] = "WebRTC-Bwe-SeparateAudioPackets";

  BweSeparateAudioPacketsSettings() = default;
  explicit BweSeparateAudioPacketsSettings(
      const FieldTrialsView* key_value_config);

  bool enabled = false;
  int packet_threshold = 10;
  int64_t time_threshold = int64_t::Seconds(1);

  std::unique_ptr<StructParametersParser> Parser();
};

class DelayBasedBwe {
 public:
  struct Result {
    Result();
    ~Result() = default;
    bool updated;
    bool probe;
    int64_t target_bitrate = int64_t::Zero();
    bool recovered_from_overuse;
    BandwidthUsage delay_detector_state;
  };

  explicit DelayBasedBwe(const FieldTrialsView* key_value_config,
                         RtcEventLog* event_log,
                         NetworkStatePredictor* network_state_predictor);

  DelayBasedBwe() = delete;
  DelayBasedBwe(const DelayBasedBwe&) = delete;
  DelayBasedBwe& operator=(const DelayBasedBwe&) = delete;

  virtual ~DelayBasedBwe();

  Result IncomingPacketFeedbackVector(
      const TransportPacketsFeedback& msg, std::optional<int64_t> acked_bitrate,
      std::optional<int64_t> probe_bitrate,
      std::optional<NetworkStateEstimate> network_estimate, bool in_alr);
  void OnRttUpdate(int64_t avg_rtt);
  bool LatestEstimate(std::vector<uint32_t>* ssrcs, int64_t* bitrate) const;
  void SetStartBitrate(int64_t start_bitrate);
  void SetMinBitrate(int64_t min_bitrate);
  int64_t GetExpectedBwePeriod() const;
  int64_t TriggerOveruse(int64_t at_time, std::optional<int64_t> link_capacity);
  int64_t last_estimate() const { return prev_bitrate_; }
  BandwidthUsage last_state() const { return prev_state_; }

 private:
  friend class GoogCcStatePrinter;
  void IncomingPacketFeedback(const PacketResult& packet_feedback,
                              int64_t at_time);
  Result MaybeUpdateEstimate(std::optional<int64_t> acked_bitrate,
                             std::optional<int64_t> probe_bitrate,
                             std::optional<NetworkStateEstimate> state_estimate,
                             bool recovered_from_overuse, bool in_alr,
                             int64_t at_time);
  // Updates the current remote rate estimate and returns true if a valid
  // estimate exists.
  bool UpdateEstimate(int64_t at_time, std::optional<int64_t> acked_bitrate,
                      int64_t* target_rate);

  rtc::RaceChecker network_race_;
  RtcEventLog* const event_log_;
  const FieldTrialsView* const key_value_config_;

  // Alternatively, run two separate overuse detectors for audio and video,
  // and fall back to the audio one if we haven't seen a video packet in a
  // while.
  BweSeparateAudioPacketsSettings separate_audio_;
  int64_t audio_packets_since_last_video_;
  int64_t last_video_packet_recv_time_;

  NetworkStatePredictor* network_state_predictor_;
  std::unique_ptr<InterArrival> video_inter_arrival_;
  std::unique_ptr<InterArrivalDelta> video_inter_arrival_delta_;
  std::unique_ptr<DelayIncreaseDetectorInterface> video_delay_detector_;
  std::unique_ptr<InterArrival> audio_inter_arrival_;
  std::unique_ptr<InterArrivalDelta> audio_inter_arrival_delta_;
  std::unique_ptr<DelayIncreaseDetectorInterface> audio_delay_detector_;
  DelayIncreaseDetectorInterface* active_delay_detector_;

  int64_t last_seen_packet_;
  bool uma_recorded_;
  AimdRateControl rate_control_;
  int64_t prev_bitrate_;
  BandwidthUsage prev_state_;
};

#endif