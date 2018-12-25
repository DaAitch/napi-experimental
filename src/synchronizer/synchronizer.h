#ifndef __SYNCHRONIZER_H__
#define __SYNCHRONIZER_H__

#include <functional>
#include "../napi_experimental.h"

template<typename CallbackParam>
using CallbackFn = std::function<void(CallbackParam&&)>;
template<typename CallbackParam>
using AsyncFn = std::function<void(CallbackFn<CallbackParam>)>;
template<typename CallbackParam>
using SyncFn = std::function<void(Napi::Env, CallbackParam&&)>;

template<typename CallbackParam>
struct holder_t {
  SyncFn<CallbackParam> sync_fn;
  CallbackParam callback_param;

  napi_threadsafe_function ts_fn;
};

template<typename CallbackParam>
napi_value _Callback(napi_env env, napi_callback_info info) {
  holder_t<CallbackParam>* holder;
  CHECK_NAPI(napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**)&holder));

  holder->sync_fn(env, std::move(holder->callback_param));
  CHECK_NAPI(napi_unref_threadsafe_function(env, holder->ts_fn));
  return nullptr;
}

template<typename CallbackParam>
void _Finalize(napi_env env, void* data, void* hint) {
  auto&& holder = reinterpret_cast<holder_t<CallbackParam>*>(data);
  delete holder;
}

template<typename CallbackParam>
void Synchronized(Napi::Env env, const AsyncFn<CallbackParam>& async_fn, SyncFn<CallbackParam> sync_fn) {
  holder_t<CallbackParam>* holder = new holder_t<CallbackParam> {std::move(sync_fn)};

  napi_value cb;
  CHECK_NAPI(napi_create_function(env, nullptr, NAPI_AUTO_LENGTH, _Callback<CallbackParam>, holder, &cb));

  napi_value resource_name;
  napi_create_string_latin1(env, "synchronized", NAPI_AUTO_LENGTH, &resource_name);

  CHECK_NAPI(napi_create_threadsafe_function(env, cb, nullptr, resource_name, 0, 1, holder, _Finalize<CallbackParam>, nullptr, nullptr, &holder->ts_fn));
  CHECK_NAPI(napi_ref_threadsafe_function(env, holder->ts_fn));

  async_fn([env, holder](CallbackParam&& param) {
    holder->callback_param = std::forward<CallbackParam>(param);
    CHECK_NAPI(napi_call_threadsafe_function(holder->ts_fn, nullptr, napi_tsfn_nonblocking));
  });
}

#endif