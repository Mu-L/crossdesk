
#include "packet_sender.h"

#include "log.h"

PacketSender::PacketSender(std::shared_ptr<IceAgent> ice_agent,
                           std::shared_ptr<webrtc::Clock> clock)
    : ice_agent_(ice_agent),
      clock_(clock),
      pacing_controller_(clock.get(), this) {}

PacketSender::~PacketSender() {}

// int PacketSender::SendPacket(const char *data, size_t size) {
//   LOG_INFO("Send packet, size: %d", size);
//   return ice_agent_->Send(data, size);
// }

// void PacketSender::CreateProbeClusters(
//     std::vector<webrtc::ProbeClusterConfig> probe_cluster_configs) {
//   pacing_controller_.CreateProbeClusters(probe_cluster_configs);
//   MaybeScheduleProcessPackets();
// }

// void PacketSender::MaybeScheduleProcessPackets() {
//   if (!processing_packets_)
//     MaybeProcessPackets(webrtc::Timestamp::MinusInfinity());
// }

// void PacketSender::MaybeProcessPackets(
//     webrtc::Timestamp scheduled_process_time) {
//   if (is_shutdown_ || !is_started_) {
//     return;
//   }

//   // Protects against re-entry from transport feedback calling into the task
//   // queue pacer.
//   processing_packets_ = true;
//   auto cleanup = std::unique_ptr<void, std::function<void(void *)>>(
//       nullptr, [this](void *) { processing_packets_ = false; });

//   webrtc::Timestamp next_send_time = pacing_controller_.NextSendTime();
//   const webrtc::Timestamp now = clock_->CurrentTime();
//   webrtc::TimeDelta early_execute_margin =
//       pacing_controller_.IsProbing()
//           ? webrtc::PacingController::kMaxEarlyProbeProcessing
//           : webrtc::TimeDelta::Zero();

//   // Process packets and update stats.
//   while (next_send_time <= now + early_execute_margin) {
//     pacing_controller_.ProcessPackets();
//     next_send_time = pacing_controller_.NextSendTime();

//     // Probing state could change. Get margin after process packets.
//     early_execute_margin =
//         pacing_controller_.IsProbing()
//             ? webrtc::PacingController::kMaxEarlyProbeProcessing
//             : webrtc::TimeDelta::Zero();
//   }
//   UpdateStats();

//   // Ignore retired scheduled task, otherwise reset `next_process_time_`.
//   if (scheduled_process_time.IsFinite()) {
//     if (scheduled_process_time != next_process_time_) {
//       return;
//     }
//     next_process_time_ = webrtc::Timestamp::MinusInfinity();
//   }

//   // Do not hold back in probing.
//   webrtc::TimeDelta hold_back_window = webrtc::TimeDelta::Zero();
//   if (!pacing_controller_.IsProbing()) {
//     hold_back_window = max_hold_back_window_;
//     webrtc::DataRate pacing_rate = pacing_controller_.pacing_rate();
//     if (max_hold_back_window_in_packets_ != kNoPacketHoldback &&
//         !pacing_rate.IsZero() &&
//         packet_size_.filtered() != rtc::ExpFilter::kValueUndefined) {
//       webrtc::TimeDelta avg_packet_send_time =
//           webrtc::DataSize::Bytes(packet_size_.filtered()) / pacing_rate;
//       hold_back_window =
//           std::min(hold_back_window,
//                    avg_packet_send_time * max_hold_back_window_in_packets_);
//     }
//   }

//   // Calculate next process time.
//   webrtc::TimeDelta time_to_next_process =
//       std::max(hold_back_window, next_send_time - now -
//       early_execute_margin);
//   next_send_time = now + time_to_next_process;

//   // If no in flight task or in flight task is later than `next_send_time`,
//   // schedule a new one. Previous in flight task will be retired.
//   if (next_process_time_.IsMinusInfinity() ||
//       next_process_time_ > next_send_time) {
//     // Prefer low precision if allowed and not probing.
//     task_queue_->PostDelayedHighPrecisionTask(
//         SafeTask(
//             safety_.flag(),
//             [this, next_send_time]() { MaybeProcessPackets(next_send_time);
//             }),
//         time_to_next_process.RoundUpTo(webrtc::TimeDelta::Millis(1)));
//     next_process_time_ = next_send_time;
//   }
// }

// void PacketSender::UpdateStats() {
//   Stats new_stats;
//   new_stats.expected_queue_time = pacing_controller_.ExpectedQueueTime();
//   new_stats.first_sent_packet_time =
//   pacing_controller_.FirstSentPacketTime();
//   new_stats.oldest_packet_enqueue_time =
//       pacing_controller_.OldestPacketEnqueueTime();
//   new_stats.queue_size = pacing_controller_.QueueSizeData();
//   OnStatsUpdated(new_stats);
// }

// PacketSender::Stats PacketSender::GetStats() const { return current_stats_; }