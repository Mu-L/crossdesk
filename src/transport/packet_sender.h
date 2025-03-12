/*
 * @Author: DI JUNKUN
 * @Date: 2025-03-12
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <memory>

#include "api/array_view.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/data_size.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "ice_agent.h"
#include "pacing_controller.h"
#include "rtc_base/numerics/exp_filter.h"
#include "rtp_packet_pacer.h"
#include "rtp_packet_to_send.h"

class PacketSender : public webrtc::RtpPacketPacer,
                     public webrtc::PacingController::PacketSender {
 public:
  PacketSender(std::shared_ptr<IceAgent> ice_agent,
               std::shared_ptr<webrtc::Clock> clock);
  ~PacketSender();

  int SendPacket(const char* data, size_t size);

 public:
  void CreateProbeClusters(
      std::vector<webrtc::ProbeClusterConfig> probe_cluster_configs) override{};

  // Temporarily pause all sending.
  void Pause() override{};

  // Resume sending packets.
  void Resume() override{};

  void SetCongested(bool congested) override{};

  // Sets the pacing rates. Must be called once before packets can be sent.
  void SetPacingRates(webrtc::DataRate pacing_rate,
                      webrtc::DataRate padding_rate) override{};

  // Time since the oldest packet currently in the queue was added.
  webrtc::TimeDelta OldestPacketWaitTime() const override {
    return webrtc::TimeDelta::Zero();
  };

  // Sum of payload + padding bytes of all packets currently in the pacer queue.
  webrtc::DataSize QueueSizeData() const override {
    return webrtc::DataSize::Zero();
  };

  // Returns the time when the first packet was sent.
  std::optional<webrtc::Timestamp> FirstSentPacketTime() const override {
    return {};
  }

  // Returns the expected number of milliseconds it will take to send the
  // current packets in the queue, given the current size and bitrate, ignoring
  // priority.
  webrtc::TimeDelta ExpectedQueueTime() const override {
    return webrtc::TimeDelta::Zero();
  };

  // Set the average upper bound on pacer queuing delay. The pacer may send at
  // a higher rate than what was configured via SetPacingRates() in order to
  // keep ExpectedQueueTimeMs() below `limit_ms` on average.
  void SetQueueTimeLimit(webrtc::TimeDelta limit) override{};

  // Currently audio traffic is not accounted by pacer and passed through.
  // With the introduction of audio BWE audio traffic will be accounted for
  // the pacer budget calculation. The audio traffic still will be injected
  // at high priority.
  void SetAccountForAudioPackets(bool account_for_audio) override{};
  void SetIncludeOverhead() override{};
  void SetTransportOverhead(webrtc::DataSize overhead_per_packet) override{};

 public:
  void SendPacket(std::unique_ptr<webrtc::RtpPacketToSend> packet,
                  const webrtc::PacedPacketInfo& cluster_info) override {}
  // Should be called after each call to SendPacket().
  std::vector<std::unique_ptr<webrtc::RtpPacketToSend>> FetchFec() override {
    return {};
  }
  std::vector<std::unique_ptr<webrtc::RtpPacketToSend>> GeneratePadding(
      webrtc::DataSize size) override {
    return {};
  }
  // TODO(bugs.webrtc.org/1439830): Make pure  once subclasses adapt.
  void OnBatchComplete() override {}

  // TODO(bugs.webrtc.org/11340): Make pure  once downstream projects
  // have been updated.
  void OnAbortedRetransmissions(
      uint32_t /* ssrc */,
      rtc::ArrayView<const uint16_t> /* sequence_numbers */) {}
  std::optional<uint32_t> GetRtxSsrcForMedia(
      uint32_t /* ssrc */) const override {
    return std::nullopt;
  }

 private:
  std::shared_ptr<IceAgent> ice_agent_ = nullptr;
  webrtc::PacingController pacing_controller_;

 private:
  std::shared_ptr<webrtc::Clock> clock_ = nullptr;
};

#endif