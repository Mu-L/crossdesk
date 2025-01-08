/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-18
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _RTP_PACKET_RECEIVED_H_
#define _RTP_PACKET_RECEIVED_H_

#include <limits>

#include "enc_mark.h"
#include "rtp_packet.h"

class RtpPacketReceived : public RtpPacket {
 public:
  RtpPacketReceived();
  RtpPacketReceived(const RtpPacketReceived& packet);
  RtpPacketReceived(RtpPacketReceived&& packet);

  RtpPacketReceived& operator=(const RtpPacketReceived& packet);
  RtpPacketReceived& operator=(RtpPacketReceived&& packet);

  ~RtpPacketReceived();

 public:
  int64_t arrival_time() const { return arrival_time_; }
  void set_arrival_time(int64_t time) { arrival_time_ = time; }

  // Explicit Congestion Notification (ECN), RFC-3168, Section 5.
  // Used by L4S: https://www.rfc-editor.org/rfc/rfc9331.html
  EcnMarking ecn() const { return ecn_; }
  void set_ecn(EcnMarking ecn) { ecn_ = ecn; }

  // Flag if packet was recovered via RTX or FEC.
  bool recovered() const { return recovered_; }
  void set_recovered(bool value) { recovered_ = value; }

  int payload_type_frequency() const { return payload_type_frequency_; }
  void set_payload_type_frequency(int value) {
    payload_type_frequency_ = value;
  }

 private:
  int64_t arrival_time_ = std::numeric_limits<int64_t>::min();
  EcnMarking ecn_ = EcnMarking::kNotEct;
  int payload_type_frequency_ = 0;
  bool recovered_ = false;
};

#endif