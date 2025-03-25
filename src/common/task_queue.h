/*
 * @Author: DI JUNKUN
 * @Date: 2025-03-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "any_invocable.h"

class TaskQueue {
 public:
  TaskQueue(std::string task_name, size_t numThreads = 1)
      : task_name_(task_name),
        stop_(false),
        workers_(),
        taskQueue_(),
        mutex_(),
        cond_var_() {
    for (size_t i = 0; i < numThreads; i++) {
      workers_.emplace_back([this]() { this->WorkerThread(); });
    }
  }

  ~TaskQueue() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cond_var_.notify_all();
    for (std::thread &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  void PostTask(AnyInvocable<void()> task) {
    PostDelayedTask(std::move(task), 0);
  }

  void PostDelayedTask(AnyInvocable<void()> task, int delay_ms) {
    auto execute_time =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
    {
      std::unique_lock<std::mutex> lock(mutex_);
      taskQueue_.emplace(execute_time, std::move(task));
    }
    cond_var_.notify_one();
  }

  void ClearTasks() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
      while (!taskQueue_.empty()) {
        taskQueue_.pop();
      }
    }
    cond_var_.notify_all();
    for (std::thread &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

 private:
  struct TaskItem {
    std::chrono::steady_clock::time_point execute_time;
    AnyInvocable<void()> task = nullptr;

    TaskItem(std::chrono::steady_clock::time_point time,
             AnyInvocable<void()> func)
        : execute_time(time), task(std::move(func)) {}

    bool operator>(const TaskItem &other) const {
      return execute_time > other.execute_time;
    }
  };

  void WorkerThread() {
    while (true) {
      AnyInvocable<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { return !taskQueue_.empty() || stop_; });

        if (stop_ && taskQueue_.empty()) return;

        auto now = std::chrono::steady_clock::now();
        if (taskQueue_.top().execute_time > now) {
          cond_var_.wait_until(lock, taskQueue_.top().execute_time,
                               [this]() { return stop_; });
        }

        if (stop_ && taskQueue_.empty()) return;

        task = std::move(
            const_cast<AnyInvocable<void()> &>(taskQueue_.top().task));
        taskQueue_.pop();
      }
      task();
    }
  }

  std::string task_name_;
  std::vector<std::thread> workers_;
  std::priority_queue<TaskItem, std::vector<TaskItem>, std::greater<>>
      taskQueue_;
  mutable std::mutex mutex_;
  std::condition_variable cond_var_;
  bool stop_;
};

#endif
