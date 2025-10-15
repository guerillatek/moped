#pragma once

#include <coroutine>
#include <exception>

namespace moped {
template <typename T = void> struct CoParseResult {
  struct promise_type {
    std::optional<T> result;
    std::exception_ptr exception;

    CoParseResult get_return_object() {
      return CoParseResult{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    // For void return type
    void return_void()
      requires std::is_void_v<T>
    {}

    // For non-void return type
    void return_value(T value)
      requires(!std::is_void_v<T>)
    {
      result = std::move(value);
    }

    void unhandled_exception() { exception = std::current_exception(); }
  };

  std::coroutine_handle<promise_type> handle;

  // ... existing move/copy constructors ...

  bool resume() {
    if (handle && !handle.done()) {
      handle.resume();
      if (handle.promise().exception) {
        std::rethrow_exception(handle.promise().exception);
      }
      return !handle.done();
    }
    return false;
  }

  // Get the result (for non-void types)
  std::optional<T> getResult() const
    requires(!std::is_void_v<T>)
  {
    if (handle && handle.done()) {
      return handle.promise().result;
    }
    return std::nullopt;
  }

  bool done() const { return !handle || handle.done(); }
};

} // namespace moped