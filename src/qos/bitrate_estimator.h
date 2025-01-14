/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _BITRATE_ESTIMATOR_H_
#define _BITRATE_ESTIMATOR_H_

#include <stdint.h>

#include <optional>

#include "constrained.h"

// Computes a bayesian estimate of the throughput given acks containing
// the arrival time and payload size. Samples which are far from the current
// estimate or are based on few packets are given a smaller weight, as they
// are considered to be more likely to have been caused by, e.g., delay spikes
// unrelated to congestion.
class BitrateEstimator {
 public:
  explicit BitrateEstimator();
  virtual ~BitrateEstimator();
  virtual void Update(int64_t at_time, int64_t amount, bool in_alr);

  virtual std::optional<int64_t> bitrate() const;
  std::optional<int64_t> PeekRate() const;

  virtual void ExpectFastRateChange();

 private:
  float UpdateWindow(int64_t now_ms, int bytes, int rate_window_ms,
                     bool* is_small_sample);
  int sum_;
  Constrained<int> initial_window_ms_;
  Constrained<int> noninitial_window_ms_;
  double uncertainty_scale_;
  double uncertainty_scale_in_alr_;
  double small_sample_uncertainty_scale_;
  int64_t small_sample_threshold_;
  int64_t uncertainty_symmetry_cap_;
  int64_t estimate_floor_;
  int64_t current_window_ms_;
  int64_t prev_time_ms_;
  float bitrate_estimate_kbps_;
  float bitrate_estimate_var_;
};

#endif