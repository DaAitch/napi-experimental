#include <future>
#include <iostream>
#include "napi_experimental.h"
#include "synchronizer/synchronizer.h"

void SumAsync(const Napi::CallbackInfo& info) {
  Napi::Function js_cb = info[0].As<Napi::Function>();

  // switch to C-NAPI, because we need to copy a reference inside a lambda
  napi_ref js_cb_ref;
  CHECK_NAPI(napi_create_reference(info.Env(), js_cb, 1, &js_cb_ref));

  Synchronized<int>(info.Env(),
    // called from node thread; can call `cb(...)` from any thread
    [](auto&& cb) {
      std::async(std::launch::async, [cb=std::move(cb)](){
        int sum = 0;
        for (int i = 0; i < 1000; i++) {
          sum += i;
        }

        return sum;
      });
    }, [js_cb_ref](Napi::Env env, int result) {
      napi_value js_cb;
      CHECK_NAPI(napi_get_reference_value(env, js_cb_ref, &js_cb));

      auto&& number = Napi::Number::New(env, result);
      napi_value args = number;

      napi_value object;
      CHECK_NAPI(napi_create_object(env, &object));

      CHECK_NAPI(napi_call_function(env, object, js_cb, 1, &args, nullptr));
      CHECK_NAPI(napi_delete_reference(env, js_cb_ref));
    });
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports["sumAsync"] = Napi::Function::New(env, SumAsync, nullptr);
  return exports;
}

NODE_API_MODULE(synchronizer, Init)