/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-12
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _RECEIVE_SIDE_CONGESTION_CONTROLLER_H_
#define _RECEIVE_SIDE_CONGESTION_CONTROLLER_H_

#include <mutex>

#include "congestion_control_feedback_generator.h"
#include "rtp_packet_received.h"

class ReceiveSideCongestionController {
 public:
  enum MediaType { VIDEO, AUDIO, DATA };

 public:
  ReceiveSideCongestionController(RtcpSender feedback_sender);
  ~ReceiveSideCongestionController() = default;

 public:
  void OnReceivedPacket(RtpPacketReceived& packet, MediaType media_type);

  // This is send bitrate, used to control the rate of feedback messages.
  void OnBitrateChanged(int bitrate_bps);

  // Ensures the remote party is notified of the receive bitrate no larger than
  // `bitrate` using RTCP REMB.
  void SetMaxDesiredReceiveBitrate(int64_t bitrate);

  void SetTransportOverhead(int64_t overhead_per_packet);

  // Runs periodic tasks if it is time to run them, returns time until next
  // call to `MaybeProcess` should be non idle.
  int64_t MaybeProcess();

 private:
  //   RembThrottler remb_throttler_;

  // TODO: bugs.webrtc.org/42224904 - Use sequence checker for all usage of
  // ReceiveSideCongestionController. At the time of
  // writing OnReceivedPacket and MaybeProcess can unfortunately be called on an
  // arbitrary thread by external projects.
  //   SequenceChecker sequence_checker_;

  CongestionControlFeedbackGenerator congestion_control_feedback_generator_;

  std::mutex mutex_;
  bool using_absolute_send_time_;
  uint32_t packets_since_absolute_send_time_;
};

#endif