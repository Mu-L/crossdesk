/*
 * @Author: DI JUNKUN
 * @Date: 2025-02-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _RTP_PACKET_HISTORY_H_
#define _RTP_PACKET_HISTORY_H_

#include <deque>

#include "rtp_packet_to_send.h"

class RtpPacketHistory {
 public:
  RtpPacketHistory();
  ~RtpPacketHistory();

  void AddPacket(std::shared_ptr<RtpPacketToSend> rtp_packet,
                 Timestamp send_time);

 private:
  int GetPacketIndex(uint16_t sequence_number) const;

  return packet_index;
}

private : struct RtpPacketToSendInfo {
  std::shared_ptr<RtpPacketToSend> rtp_packet;
  Timestamp send_time;
  uint64_t index;
};

private:
std::deque<std::shared_ptr<RtpPacketToSend>> rtp_packet_history_;
}

#endif