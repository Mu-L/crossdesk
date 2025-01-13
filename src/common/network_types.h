/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-13
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _NETWORK_TYPES_H_
#define _NETWORK_TYPES_H_

#include <algorithm>
#include <limits>
#include <vector>

#include "enc_mark.h"

struct NetworkEstimate {
  int64_t at_time = std::numeric_limits<int64_t>::max();
  // Deprecated, use TargetTransferRate::target_rate instead.
  int64_t bandwidth = std::numeric_limits<int64_t>::max();
  int64_t round_trip_time = std::numeric_limits<int64_t>::max();
  int64_t bwe_period = std::numeric_limits<int64_t>::max();

  float loss_rate_ratio = 0;
};

struct TargetTransferRate {
  int64_t at_time = std::numeric_limits<int64_t>::max();
  // The estimate on which the target rate is based on.
  NetworkEstimate network_estimate;
  int64_t target_rate = 0;
  int64_t stable_target_rate = 0;
  double cwnd_reduce_ratio = 0;
};

struct NetworkControlUpdate {
  NetworkControlUpdate() = default;
  NetworkControlUpdate(const NetworkControlUpdate&) = default;
  ~NetworkControlUpdate() = default;

  bool has_updates() const {
    // return congestion_window.has_value() || pacer_config.has_value() ||
    // !probe_cluster_configs.empty() ||
    return target_rate.has_value();
  }

  std::optional<int64_t> congestion_window;
  // std::optional<PacerConfig> pacer_config;
  // std::vector<ProbeClusterConfig> probe_cluster_configs;
  std::optional<TargetTransferRate> target_rate;
};

struct PacedPacketInfo {
  PacedPacketInfo() = default;
  PacedPacketInfo(int probe_cluster_id, int probe_cluster_min_probes,
                  int probe_cluster_min_bytes)
      : probe_cluster_id(probe_cluster_id),
        probe_cluster_min_probes(probe_cluster_min_probes),
        probe_cluster_min_bytes(probe_cluster_min_bytes) {}

  bool operator==(const PacedPacketInfo& rhs) const {
    return send_bitrate == rhs.send_bitrate &&
           probe_cluster_id == rhs.probe_cluster_id &&
           probe_cluster_min_probes == rhs.probe_cluster_min_probes &&
           probe_cluster_min_bytes == rhs.probe_cluster_min_bytes;
  }

  // TODO(srte): Move probing info to a separate, optional struct.
  static constexpr int kNotAProbe = -1;
  int64_t send_bitrate = 0;
  int probe_cluster_id = kNotAProbe;
  int probe_cluster_min_probes = -1;
  int probe_cluster_min_bytes = -1;
  int probe_cluster_bytes_sent = 0;
};

struct SentPacket {
  int64_t send_time = std::numeric_limits<int64_t>::max();
  // Size of packet with overhead up to IP layer.
  int64_t size = 0;
  // Size of preceeding packets that are not part of feedback.
  int64_t prior_unacked_data = 0;
  // Probe cluster id and parameters including bitrate, number of packets and
  // number of bytes.
  PacedPacketInfo pacing_info;
  // True if the packet is an audio packet, false for video, padding, RTX etc.
  bool audio = false;
  // Transport independent sequence number, any tracked packet should have a
  // sequence number that is unique over the whole call and increasing by 1 for
  // each packet.
  int64_t sequence_number;
  // Tracked data in flight when the packet was sent, excluding unacked data.
  int64_t data_in_flight = 0;
};

struct PacketResult {
  class ReceiveTimeOrder {
   public:
    bool operator()(const PacketResult& lhs, const PacketResult& rhs);
  };

  PacketResult() = default;
  PacketResult(const PacketResult&) = default;
  ~PacketResult() = default;

  inline bool IsReceived() const {
    return receive_time != std::numeric_limits<int64_t>::max();
  }

  SentPacket sent_packet;
  int64_t receive_time = std::numeric_limits<int64_t>::max();
  EcnMarking ecn = EcnMarking::kNotEct;
};

struct TransportPacketsFeedback {
  TransportPacketsFeedback() = default;
  TransportPacketsFeedback(const TransportPacketsFeedback& other) = default;
  ~TransportPacketsFeedback() = default;

  int64_t feedback_time = std::numeric_limits<int64_t>::max();
  int64_t data_in_flight = 0;
  bool transport_supports_ecn = false;
  std::vector<PacketResult> packet_feedbacks;

  // Arrival times for messages without send time information.
  std::vector<int64_t> sendless_arrival_times;

  std::vector<PacketResult> ReceivedWithSendInfo() const {
    std::vector<PacketResult> res;
    for (const PacketResult& fb : packet_feedbacks) {
      if (fb.IsReceived()) {
        res.push_back(fb);
      }
    }
    return res;
  }
  std::vector<PacketResult> LostWithSendInfo() const {
    std::vector<PacketResult> res;
    for (const PacketResult& fb : packet_feedbacks) {
      if (!fb.IsReceived()) {
        res.push_back(fb);
      }
    }
    return res;
  }

  std::vector<PacketResult> PacketsWithFeedback() const {
    return packet_feedbacks;
  }
  std::vector<PacketResult> SortedByReceiveTime() const {
    std::vector<PacketResult> res;
    for (const PacketResult& fb : packet_feedbacks) {
      if (fb.IsReceived()) {
        res.push_back(fb);
      }
    }
    std::sort(res.begin(), res.end(), PacketResult::ReceiveTimeOrder());
    return res;
  }
};

#endif