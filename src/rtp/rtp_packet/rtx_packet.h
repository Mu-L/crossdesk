/*
 * @Author: DI JUNKUN
 * @Date: 2025-03-31
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _RTX_PACKET_H_
#define _RTX_PACKET_H_

#include "rtp_packet.h"
#include "rtp_packet_to_send.h"

// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         RTP Header                            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |            OSN                |                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
// |                  Original RTP Packet Payload                  |
// |                                                               |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class RtxPacket {
 public:
  RtxPacket();
  ~RtxPacket();

  void Build(RtpPacket& rtp_packet, uint32_t origin_ssrc, uint16_t seq_no);
};

#endif