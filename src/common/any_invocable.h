/*
 * @Author: DI JUNKUN
 * @Date: 2025-03-14
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _ANY_INVOCABLE_H_
#define _ANY_INVOCABLE_H_

#include <functional>
#include <iostream>
#include <memory>

// 简化版的 AnyInvocable
template <typename Signature>
class AnyInvocable;

template <typename R, typename... Args>
class AnyInvocable<R(Args...)> {
 public:
  // 默认构造函数
  AnyInvocable() = default;

  // 构造函数：接受一个可以调用的对象
  template <typename Callable>
  AnyInvocable(Callable&& callable)
      : callable_(std::make_unique<CallableWrapper<Callable>>(
            std::forward<Callable>(callable))) {}

  // 调用运算符
  R operator()(Args... args) {
    return callable_->Invoke(std::forward<Args>(args)...);
  }

  // 移动构造函数
  AnyInvocable(AnyInvocable&&) = default;
  // 移动赋值运算符
  AnyInvocable& operator=(AnyInvocable&&) = default;

  explicit operator bool() const { return static_cast<bool>(callable_); }

 private:
  // 抽象基类，允许不同类型的可调用对象
  struct CallableBase {
    virtual ~CallableBase() = default;
    virtual R Invoke(Args&&... args) = 0;
  };

  // 模板派生类：实际存储 callable 对象
  template <typename Callable>
  struct CallableWrapper : public CallableBase {
    CallableWrapper(Callable&& callable)
        : callable_(std::forward<Callable>(callable)) {}

    R Invoke(Args&&... args) override {
      return callable_(std::forward<Args>(args)...);
    }

    Callable callable_;
  };

  std::unique_ptr<CallableBase> callable_;
};

// 简单的包装函数
template <typename R, typename... Args>
AnyInvocable<R(Args...)> MakeMoveOnlyFunction(std::function<R(Args...)>&& f) {
  return AnyInvocable<R(Args...)>(std::move(f));
}

#endif