#ifndef __THREADSAFE_NODE_CALLS_H__
#define __THREADSAFE_NODE_CALLS_H__

#include <functional>
#include <memory>
#include "../napi_experimental.h"

class ThreadsafeNodeCalls : public std::enable_shared_from_this<ThreadsafeNodeCalls> {
  using Callback = std::function<void(Napi::Env)>;

public:
  static std::shared_ptr<ThreadsafeNodeCalls> Create(Napi::Env env) {
    return std::make_shared<ThreadsafeNodeCalls>(env);
  }

private:
  struct ts_cb_data_t {
    Callback callback;
    std::shared_ptr<ThreadsafeNodeCalls> node_calls;
  };

  static void Finalizer(napi_env env, void* data, void*) {
    auto&& node_calls = reinterpret_cast<ThreadsafeNodeCalls*>(data);
  }


  static napi_value NoopJsFunction(napi_env env, napi_callback_info info) {
    return nullptr;
  }

  static void CallJsCb(napi_env env, napi_value, void*, void* data) {
    auto&& cb_data = reinterpret_cast<ts_cb_data_t*>(data);

    cb_data->callback(env);
    delete cb_data;
  }

public:
  explicit ThreadsafeNodeCalls(Napi::Env env) {
    napi_value async_resource_name;
    CHECK_NAPI(napi_create_string_latin1(env, "runInNodeThread", NAPI_AUTO_LENGTH, &async_resource_name));

    napi_value func_noop;
    CHECK_NAPI(napi_create_function(env, nullptr, NAPI_AUTO_LENGTH, NoopJsFunction, nullptr, &func_noop));

    CHECK_NAPI(napi_create_threadsafe_function(env, func_noop, nullptr, async_resource_name, 0, 1, this,
                                               Finalizer, nullptr, CallJsCb, &ts_fn));

    CHECK_NAPI(napi_acquire_threadsafe_function(ts_fn));
  }

  ThreadsafeNodeCalls(const ThreadsafeNodeCalls&) = delete;
  ThreadsafeNodeCalls& operator=(const ThreadsafeNodeCalls&) = delete;

  ThreadsafeNodeCalls(ThreadsafeNodeCalls&&) = default;
  ThreadsafeNodeCalls& operator=(ThreadsafeNodeCalls&&) = default;

  virtual ~ThreadsafeNodeCalls() {
    if (ts_fn) {
      napi_release_threadsafe_function(ts_fn, napi_tsfn_abort);
    }
  }

public:
  bool RunInNodeThread(Callback callback) {
    auto&& data = new ts_cb_data_t{std::move(callback), shared_from_this()};

    napi_status status;
    status = napi_call_threadsafe_function(ts_fn, data, napi_tsfn_nonblocking);
    if (status != napi_ok) {
      CHECK_NAPI(status); // abort in debug

      delete data;
      return false;
    }

    return true;
  }

private:
  napi_threadsafe_function ts_fn = nullptr;
};

#endif