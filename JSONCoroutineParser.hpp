#pragma once

#include "concepts.hpp"
#include "moped/ParserBase.hpp"

#include <coroutine>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <spanstream>
#include <sstream>
#include <stack>
#include <string>

namespace moped {

template <IParserEventDispatchC ParseEventDispatchT>
class JSONCoroutineParser : public ParserBase {
public:
private:
  using ExpectedText = std::expected<std::string, std::string>;
  using ValidatorStack = std::stack<char>;

  struct ParseResult {
    struct promise_type {
      ParseResult get_return_object() {
        return ParseResult{
            std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_always initial_suspend() { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void return_void() {}
      void unhandled_exception() { exception = std::current_exception(); }

      std::exception_ptr exception;
    };

    std::coroutine_handle<promise_type> handle;

    ParseResult(std::coroutine_handle<promise_type> h) : handle(h) {}

    ~ParseResult() {
      if (handle) {
        handle.destroy();
      }
    }

    // Move-only
    ParseResult(const ParseResult &) = delete;
    ParseResult &operator=(const ParseResult &) = delete;
    ParseResult(ParseResult &&other) noexcept
        : handle(std::exchange(other.handle, {})) {}
    ParseResult &operator=(ParseResult &&other) noexcept {
      if (this != &other) {
        if (handle)
          handle.destroy();
        handle = std::exchange(other.handle, {});
      }
      return *this;
    }

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

    bool done() const { return !handle || handle.done(); }
  };

  ParseEventDispatchT _eventDispatch;
  std::string _activeStreamContent;
  std::ispanstream _activeStream;
  bool _finalFrame{false}

  // Parser state
  ValidatorStack _validatorStack;
  ParseState _parseState = ParseState::MemberName;
  bool _isInitialized = false;


  // Check if stream has data available, suspend if not
  std::suspend_always checkStreamAndSuspend() { return {}; }

  // Safe stream read with suspension on EOF
  std::awaitable<bool> safeStreamRead(char &c) {
    if (_activeStream.eof()) {
      return false;
    }

    _activeStream >> c;
    if (_activeStream.eof() || _activeStream.fail()) {
      return false;
    }
    return true;
  }

  bool safeStreamPeek(char &c) {
    if (_activeStream.eof()) {
      return false;
    }

    c = _activeStream.peek();
    if (_activeStream.eof()) {
      return false;
    }
    return true;
  }

  bool safeStreamGet(char &c) {
    if (_activeStream.eof()) {
      return false;
    }

    c = _activeStream.get();
    if (_activeStream.eof() || _activeStream.fail()) {
      return false;
    }
    return true;
  }

  void prepareActiveStream(const std::string &newContent) {
    size_t totalSize = _pendingBuffer.size() + newContent.size();

    // Only reallocate if we don't have sufficient capacity
    if (_activeStreamContent.capacity() < totalSize) {
      _activeStreamContent.reserve(totalSize);
    }

    // Combine pending buffer with new content
    _activeStreamContent = _pendingBuffer + newContent;
    _pendingBuffer.clear();

    // Reconstruct the spanstream in-place to reference new content
    _activeStream = std::ispanstream(_activeStreamContent);
  }

public:
  ParseResult parseStreamAsync(const std::string &newContent) {
    prepareActiveStream(newContent);
    return parseCoroutine();
  }

private:
  ParseResult parseCoroutine() {
    try {
      if (!_isInitialized) {
        // Initial setup
        char c;
        _activeStream >> std::ws;

        if (!safeStreamRead(c)) {
          co_await checkStreamAndSuspend();
          if (!safeStreamRead(c)) {
            throw std::runtime_error(
                "Stream ended unexpectedly during initialization");
          }
        }

        if (c != '{') {
          throw std::runtime_error(
              std::format("Expected an object start, '{{' but got {}", c));
        }

        _validatorStack.push(c);
        _eventDispatch.onObjectStart();
        _parseState = ParseState::MemberName;
        _isInitialized = true;
      }

      while (!_validatorStack.empty()) {
        char c;

        switch (_parseState) {
        case ParseState::Object: {
          if (!safeStreamRead(c)) {
            co_await checkStreamAndSuspend();
            continue;
          }

          if (c != '{') {
            throw std::runtime_error(
                std::format("Expected an object start, '{{' but got {}", c));
          }
          _eventDispatch.onObjectStart();
          _validatorStack.push(c);
          _parseState = ParseState::MemberName;
        } break;

        case ParseState::MemberName: {
          auto result = co_await getMemberTextAsync();
          if (!result) {
            throw std::runtime_error(result.error());
          }

          _activeStream >> std::ws;
          if (!safeStreamRead(c)) {
            co_await checkStreamAndSuspend();
            continue;
          }

          if (c != ':') {
            throw std::runtime_error(
                std::format("Expected ':' in member definition but got {}", c));
          }

          _parseState = ParseState::OpenValue;
          _eventDispatch.onMember(result.value());
        } break;

        case ParseState::OpenValue: {
          _activeStream >> std::ws;

          if (!safeStreamPeek(c)) {
            co_await checkStreamAndSuspend();
            continue;
          }

          switch (c) {
          case '{':
            _parseState = ParseState::Object;
            continue;
          case '[':
            _parseState = ParseState::Array;
            _eventDispatch.onArrayStart();
            continue;
          case '"': {
            _activeStream.get(); // consume quote
            auto result = co_await getStringValueTextAsync();
            if (!result) {
              throw std::runtime_error(result.error());
            }
            _eventDispatch.onStringValue(result.value());
          } break;
          case 't':
          case 'f': {
            auto result = co_await getBooleanValueAsync();
            if (!result) {
              throw std::runtime_error(result.error());
            }
            _eventDispatch.onBooleanValue(result.value());
          } break;
          case ']':
          case '}':
            _parseState = ParseState::CloseValue;
            continue;
          default: {
            auto result = co_await getNumericValueTextAsync();
            if (!result) {
              throw std::runtime_error(result.error());
            }
            _eventDispatch.onNumericValue(result.value());
          }
          }
          _parseState = ParseState::CloseValue;
        } break;

        case ParseState::Array: {
          if (!safeStreamRead(c)) {
            co_await checkStreamAndSuspend();
            continue;
          }
          _validatorStack.push(c);
          _parseState = ParseState::OpenValue;
        } break;

        case ParseState::CloseValue: {
          if (!safeStreamRead(c)) {
            co_await checkStreamAndSuspend();
            continue;
          }

          switch (c) {
          case ',': {
            if (_validatorStack.top() == '{') {
              _parseState = ParseState::MemberName;
            } else if (_validatorStack.top() == '[') {
              _parseState = ParseState::OpenValue;
            }
          } break;
          case ']': {
            if (_validatorStack.top() != '[') {
              throw std::runtime_error(std::format(
                  "Invalid json parse, unexpected character, {}", c));
            }
            _eventDispatch.onArrayFinish();
            _validatorStack.pop();
          } break;
          case '}': {
            if (_validatorStack.top() != '{') {
              throw std::runtime_error(std::format(
                  "Invalid json parse, unexpected character, {}", c));
            }
            _eventDispatch.onObjectFinish();
            _validatorStack.pop();
          } break;
          }
        } break;
        }
      }
    } catch (...) {
      // Save current stream position for resumption
      saveStreamState();
      throw;
    }

    co_return;
  }

  std::awaitable<ExpectedText> getMemberTextAsync() {
    _reusableBuffer = _partialBuffer;
    _partialBuffer.clear();
    _partialState = PartialState::None;

    char c;
    while (safeStreamPeek(c) && c != '"') {
      _reusableBuffer += _activeStream.get();
      if (!_activeStream) {
        _partialBuffer = _reusableBuffer;
        _partialState = PartialState::MemberName;
        co_await checkStreamAndSuspend();
        co_return std::unexpected("Stream ended during member name parsing");
      }
    }

    if (!safeStreamGet(c)) { // consume closing quote
      _partialBuffer = _reusableBuffer;
      _partialState = PartialState::MemberName;
      co_await checkStreamAndSuspend();
      co_return std::unexpected("Stream ended during member name parsing");
    }

    if (isIdentifier(_reusableBuffer)) {
      co_return _reusableBuffer;
    }

    co_return std::unexpected(
        std::format("Invalid member identifier {}", _reusableBuffer));
  }

  std::awaitable<ExpectedText> getStringValueTextAsync() {
    if (_partialState == PartialState::StringValue) {
      _reusableBuffer = _partialBuffer;
      _partialBuffer.clear();
      _partialState = PartialState::None;
    } else {
      char c = _activeStream.get();
      _reusableBuffer.clear();
      _reusableBuffer += c;
    }

    char c;
    while (_activeStream) {
      if (!safeStreamPeek(c)) {
        _partialBuffer = _reusableBuffer;
        _partialState = PartialState::StringValue;
        co_await checkStreamAndSuspend();
        co_return std::unexpected("Stream ended during string value parsing");
      }

      if (c == '\\') {
        _reusableBuffer += _activeStream.get();
        if (!safeStreamGet(c)) {
          _partialBuffer = _reusableBuffer;
          _partialState = PartialState::StringValue;
          co_await checkStreamAndSuspend();
          co_return std::unexpected("Stream ended during string value parsing");
        }
        _reusableBuffer += c;
        continue;
      }

      c = _activeStream.get();
      if (c == '"') {
        co_return _reusableBuffer;
      }
      _reusableBuffer += c;
    }

    co_return std::unexpected(std::format(
        "String value='{}' missing closing quote", _reusableBuffer));
  }

  std::awaitable<ExpectedText> getNumericValueTextAsync() {
    if (_partialState == PartialState::NumericValue) {
      _reusableBuffer = _partialBuffer;
      _partialBuffer.clear();
      _partialState = PartialState::None;
    } else {
      _reusableBuffer.clear();
    }

    char c;
    while (safeStreamPeek(c) && isNumericChar(c)) {
      _reusableBuffer += _activeStream.get();
      if (!_activeStream) {
        _partialBuffer = _reusableBuffer;
        _partialState = PartialState::NumericValue;
        co_await checkStreamAndSuspend();
        co_return std::unexpected("Stream ended during numeric value parsing");
      }
    }

    if (isNumeric(_reusableBuffer)) {
      co_return _reusableBuffer;
    }

    co_return std::unexpected(
        std::format("Invalid numeric value {}", _reusableBuffer));
  }

  std::awaitable<std::expected<bool, std::string>> getBooleanValueAsync() {
    if (_partialState == PartialState::BooleanValue) {
      _reusableBuffer = _partialBuffer;
      _partialBuffer.clear();
      _partialState = PartialState::None;
    } else {
      _reusableBuffer.clear();
    }

    char c;
    while (safeStreamPeek(c) && std::isalpha(c)) {
      _reusableBuffer += _activeStream.get();
      if (!_activeStream) {
        _partialBuffer = _reusableBuffer;
        _partialState = PartialState::BooleanValue;
        co_await checkStreamAndSuspend();
        co_return std::unexpected("Stream ended during boolean value parsing");
      }
    }

    if (_reusableBuffer == "true") {
      co_return true;
    } else if (_reusableBuffer == "false") {
      co_return false;
    }

    co_return std::unexpected(
        std::format("Invalid boolean value '{}'", _reusableBuffer));
  }

  void saveStreamState() {
    // Calculate current position in the stream
    auto currentPos = _activeStream.tellg();
    if (currentPos != std::streampos(-1)) {
      // Save any remaining content from current position
      size_t remainingStart = static_cast<size_t>(currentPos);
      if (remainingStart < _activeStreamContent.size()) {
        _pendingBuffer = _activeStreamContent.substr(remainingStart);
      }
    }
  }

public:
  auto &getDispatcher() { return _eventDispatch; }

  void reset() {
    _validatorStack = ValidatorStack{};
    _parseState = ParseState::MemberName;
    _isInitialized = false;
    _pendingBuffer.clear();
    _partialBuffer.clear();
    _partialState = PartialState::None;
    _activeStreamContent.clear();
    _activeStream = std::ispanstream{};
  }
};

} // namespace moped