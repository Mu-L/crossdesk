/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "transport_feedback_adapter.h"

#include <stdlib.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "log.h"

constexpr int64_t kSendTimeHistoryWindow = 60;

void InFlightBytesTracker::AddInFlightPacketBytes(
    const PacketFeedback& packet) {
  auto it = in_flight_data_.find(packet.network_route);
  if (it != in_flight_data_.end()) {
    it->second += packet.sent.size;
  } else {
    in_flight_data_.insert({packet.network_route, packet.sent.size});
  }
}

void InFlightBytesTracker::RemoveInFlightPacketBytes(
    const PacketFeedback& packet) {
  if (packet.sent.send_time == std::numeric_limits<int64_t>::max() ||
      packet.sent.send_time == std::numeric_limits<int64_t>::min())
    return;
  auto it = in_flight_data_.find(packet.network_route);
  if (it != in_flight_data_.end()) {
    it->second -= packet.sent.size;
    if (it->second == 0) in_flight_data_.erase(it);
  }
}

int64_t InFlightBytesTracker::GetOutstandingData(
    const NetworkRoute& network_route) const {
  auto it = in_flight_data_.find(network_route);
  if (it != in_flight_data_.end()) {
    return it->second;
  } else {
    return 0;
  }
}

// Comparator for consistent map with NetworkRoute as key.
bool InFlightBytesTracker::NetworkRouteComparator::operator()(
    const NetworkRoute& a, const NetworkRoute& b) const {
  if (a.local.network_id() != b.local.network_id())
    return a.local.network_id() < b.local.network_id();
  if (a.remote.network_id() != b.remote.network_id())
    return a.remote.network_id() < b.remote.network_id();

  if (a.local.adapter_id() != b.local.adapter_id())
    return a.local.adapter_id() < b.local.adapter_id();
  if (a.remote.adapter_id() != b.remote.adapter_id())
    return a.remote.adapter_id() < b.remote.adapter_id();

  if (a.local.uses_turn() != b.local.uses_turn())
    return a.local.uses_turn() < b.local.uses_turn();
  if (a.remote.uses_turn() != b.remote.uses_turn())
    return a.remote.uses_turn() < b.remote.uses_turn();

  return a.connected < b.connected;
}

TransportFeedbackAdapter::TransportFeedbackAdapter() = default;

std::optional<TransportPacketsFeedback>
TransportFeedbackAdapter::ProcessCongestionControlFeedback(
    const CongestionControlFeedback& feedback, int64_t feedback_receive_time) {
  if (feedback.packets().empty()) {
    LOG_INFO("Empty congestion control feedback packet received");
    return std::nullopt;
  }
  if (current_offset_ == std::numeric_limits<int64_t>::max() ||
      current_offset_ == std::numeric_limits<int64_t>::min()) {
    current_offset_ = feedback_receive_time;
  }
  int64_t feedback_delta = last_feedback_compact_ntp_time_
                               ? (feedback.report_timestamp_compact_ntp() -
                                  *last_feedback_compact_ntp_time_)
                               : 0;
  last_feedback_compact_ntp_time_ = feedback.report_timestamp_compact_ntp();
  if (feedback_delta < 0) {
    LOG_WARN("Unexpected feedback ntp time delta {}", feedback_delta);
    current_offset_ = feedback_receive_time;
  } else {
    current_offset_ += feedback_delta;
  }

  int ignored_packets = 0;
  int failed_lookups = 0;
  bool supports_ecn = true;
  std::vector<PacketResult> packet_result_vector;
  for (const CongestionControlFeedback::PacketInfo& packet_info :
       feedback.packets()) {
    std::optional<PacketFeedback> packet_feedback =
        RetrievePacketFeedback({packet_info.ssrc, packet_info.sequence_number},
                               /*received=*/packet_info.arrival_time_offset !=
                                       std::numeric_limits<int64_t>::min() &&
                                   packet_info.arrival_time_offset !=
                                       std::numeric_limits<int64_t>::max());
    if (!packet_feedback) {
      ++failed_lookups;
      continue;
    }
    if (packet_feedback->network_route != network_route_) {
      ++ignored_packets;
      continue;
    }
    PacketResult result;
    result.sent_packet = packet_feedback->sent;
    if (packet_info.arrival_time_offset !=
            std::numeric_limits<int64_t>::min() &&
        packet_info.arrival_time_offset !=
            std::numeric_limits<int64_t>::max()) {
      result.receive_time = current_offset_ - packet_info.arrival_time_offset;
      supports_ecn &= packet_info.ecn != EcnMarking::kNotEct;
    }
    result.ecn = packet_info.ecn;
    packet_result_vector.push_back(result);
  }

  if (failed_lookups > 0) {
    LOG_WARN(
        "Failed to lookup send time for {} packet {}. Packets reordered or "
        "send time history too small?",
        failed_lookups, (failed_lookups > 1 ? "s" : ""));
  }
  if (ignored_packets > 0) {
    LOG_INFO("Ignoring {} packets because they were sent on a different route",
             ignored_packets);
  }

  // Feedback is expected to be sorted in send order.
  std::sort(packet_result_vector.begin(), packet_result_vector.end(),
            [](const PacketResult& lhs, const PacketResult& rhs) {
              return lhs.sent_packet.sequence_number <
                     rhs.sent_packet.sequence_number;
            });
  return ToTransportFeedback(std::move(packet_result_vector),
                             feedback_receive_time, supports_ecn);
}

std::optional<TransportPacketsFeedback>
TransportFeedbackAdapter::ToTransportFeedback(
    std::vector<PacketResult> packet_results, int64_t feedback_receive_time,
    bool supports_ecn) {
  TransportPacketsFeedback msg;
  msg.feedback_time = feedback_receive_time;
  if (packet_results.empty()) {
    return std::nullopt;
  }
  msg.packet_feedbacks = std::move(packet_results);
  msg.data_in_flight = in_flight_.GetOutstandingData(network_route_);
  msg.transport_supports_ecn = supports_ecn;

  return msg;
}

void TransportFeedbackAdapter::SetNetworkRoute(
    const NetworkRoute& network_route) {
  network_route_ = network_route;
}

int64_t TransportFeedbackAdapter::GetOutstandingData() const {
  return in_flight_.GetOutstandingData(network_route_);
}

std::optional<PacketFeedback> TransportFeedbackAdapter::RetrievePacketFeedback(
    const SsrcAndRtpSequencenumber& key, bool received) {
  auto it = rtp_to_transport_sequence_number_.find(key);
  if (it == rtp_to_transport_sequence_number_.end()) {
    return std::nullopt;
  }
  return RetrievePacketFeedback(it->second, received);
}

std::optional<PacketFeedback> TransportFeedbackAdapter::RetrievePacketFeedback(
    int64_t transport_seq_num, bool received) {
  if (transport_seq_num > last_ack_seq_num_) {
    // Starts at history_.begin() if last_ack_seq_num_ < 0, since any
    // valid sequence number is >= 0.
    for (auto it = history_.upper_bound(last_ack_seq_num_);
         it != history_.upper_bound(transport_seq_num); ++it) {
      in_flight_.RemoveInFlightPacketBytes(it->second);
    }
    last_ack_seq_num_ = transport_seq_num;
  }

  auto it = history_.find(transport_seq_num);
  if (it == history_.end()) {
    LOG_WARN(
        "Failed to lookup send time for packet with {}. Send time history too "
        "small?",
        transport_seq_num);
    return std::nullopt;
  }

  if (it->second.sent.send_time == std::numeric_limits<int64_t>::max() ||
      it->second.sent.send_time == std::numeric_limits<int64_t>::min()) {
    // TODO(srte): Fix the tests that makes this happen and make this a
    // DCHECK.
    LOG_ERROR("Received feedback before packet was indicated as sent");
    return std::nullopt;
  }

  PacketFeedback packet_feedback = it->second;
  if (received) {
    // Note: Lost packets are not removed from history because they might
    // be reported as received by a later feedback.
    rtp_to_transport_sequence_number_.erase(
        {packet_feedback.ssrc, packet_feedback.rtp_sequence_number});
    history_.erase(it);
  }
  return packet_feedback;
}
