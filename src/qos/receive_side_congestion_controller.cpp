/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "receive_side_congestion_controller.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

namespace {
static const uint32_t kTimeOffsetSwitchThreshold = 30;
}  // namespace

ReceiveSideCongestionController::ReceiveSideCongestionController(
    RtcpSender feedback_sender)
    : congestion_control_feedback_generator_(feedback_sender),
      using_absolute_send_time_(false),
      packets_since_absolute_send_time_(0) {}

void ReceiveSideCongestionController::OnReceivedPacket(
    RtpPacketReceived& packet, MediaType media_type) {
  // RTC_DCHECK_RUN_ON(&sequence_checker_);
  congestion_control_feedback_generator_.OnReceivedPacket(packet);
  return;
}

void ReceiveSideCongestionController::OnBitrateChanged(int bitrate_bps) {
  // RTC_DCHECK_RUN_ON(&sequence_checker_);
  int64_t send_bandwidth_estimate = bitrate_bps;
  congestion_control_feedback_generator_.OnSendBandwidthEstimateChanged(
      send_bandwidth_estimate);
}

int64_t ReceiveSideCongestionController::MaybeProcess() {
  auto now = std::chrono::system_clock::now();
  int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count();
  // RTC_DCHECK_RUN_ON(&sequence_checker_);
  return congestion_control_feedback_generator_.Process(now_ms);
}

void ReceiveSideCongestionController::SetMaxDesiredReceiveBitrate(
    int64_t bitrate) {
  // remb_throttler_.SetMaxDesiredReceiveBitrate(bitrate);
}

void ReceiveSideCongestionController::SetTransportOverhead(
    int64_t overhead_per_packet) {
  // RTC_DCHECK_RUN_ON(&sequence_checker_);
  congestion_control_feedback_generator_.SetTransportOverhead(
      overhead_per_packet);
}
