#include <future>
#include <iostream>
#include <thread>
#include <chrono>
#include "napi_experimental.h"
#include "synchronizer/synchronizer.h"
#include "threadsafe-node-calls/ThreadsafeNodeCalls.h"

using namespace std::chrono_literals;

std::thread::id main_thread;

std::string GetThreadName() {
  if (std::this_thread::get_id() == main_thread) {
    return "MainThread  ";
  }

  return "WorkerThread";
}

void SumAsync(const Napi::CallbackInfo& info) {

  // switch to C-NAPI, because we need to copy a reference inside a lambda,
  // which is not working with Napi::Reference<..> at the moment
  napi_ref js_cb_ref;
  CHECK_NAPI(napi_create_reference(info.Env(), info[0], 1, &js_cb_ref));

  Synchronized<int>(info.Env(),
    // called from node thread; can call `cb(...)` from any thread
    [](auto&& cb) {
      std::cout << GetThreadName() << ": SumAsync -> async_fn" << std::endl;
      std::thread([cb=std::move(cb)]() {
        std::cout << GetThreadName() << ": SumAsync -> async_fn -> std::thread" << std::endl;

        std::this_thread::sleep_for(5s);
        int sum = 0;
        for (int i = 0; i < 100000; i++) {
          sum += i;
        }

        cb(std::move(sum));
      }).detach();
    }, [js_cb_ref](Napi::Env env, int result) {
      std::cout << GetThreadName() << ": SumAsync -> sync_fn" << std::endl;

      auto&& cb = Napi::FunctionReference(env, js_cb_ref);
      cb.Call({
        Napi::Number::New(env, result)
      });
    });
}

void SumAsync2(const Napi::CallbackInfo& info) {

  // switch to C-NAPI, because we need to copy a reference inside a lambda,
  // which is not working with Napi::Reference<..> at the moment
  napi_ref cb_ref;
  CHECK_NAPI(napi_create_reference(info.Env(), info[0], 1, &cb_ref));

  auto&& node_calls = ThreadsafeNodeCalls::Create(info.Env());
  std::cout << GetThreadName() << ": SumAsync2" << std::endl;

  // async
  std::thread([node_calls, cb_ref]() {
    std::cout << GetThreadName() << ": SumAsync2 -> std::thread" << std::endl;

    std::this_thread::sleep_for(5s);
    int sum = 0;
    for (int i = 0; i < 100000; i++) {
      sum += i;
    }

    // we can call this from any thread
    node_calls->RunInNodeThread([cb_ref, sum](Napi::Env env) {
      std::cout << GetThreadName() << ": SumAsync2 -> std::thread -> RunInNodeThread" << std::endl;

      auto&& cb = Napi::FunctionReference(env, cb_ref);
      cb.Call({
        Napi::Number::New(env, sum)
      });
    });
  }).detach();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  main_thread = std::this_thread::get_id();

  exports["sumAsync"] = Napi::Function::New(env, SumAsync, nullptr);
  exports["sumAsync2"] = Napi::Function::New(env, SumAsync2, nullptr);
  return exports;
}

NODE_API_MODULE(napi_experimental, Init)