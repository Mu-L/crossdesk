#ifndef _RTP_STATISTICS_H_
#define _RTP_STATISTICS_H_

#include "thread_base.h"

class RtpStatistics : public ThreadBase {
 public:
  RtpStatistics();
  virtual ~RtpStatistics();

 public:
  // send side
  void UpdateSentBytes(uint32_t sent_bytes);

  // receive side
  void UpdateReceiveBytes(uint32_t received_bytes);
  void UpdatePacketLossRate(uint16_t seq_num);

 private:
  bool Process();

 private:
  uint32_t sent_bytes_ = 0;
  uint32_t received_bytes_ = 0;
  uint16_t last_received_seq_num_ = 0;
  uint32_t lost_packets_num_ = 0;
};

#endif