#define NAPI_EXPERIMENTAL
#include <napi.h>

#ifndef NDEBUG
#include <cassert>

#define CHECK_NAPI(napi_call) \
  do { \
    napi_status status = (napi_call); \
    assert(status != napi_invalid_arg); \
    assert(status != napi_object_expected); \
    assert(status != napi_string_expected); \
    assert(status != napi_name_expected); \
    assert(status != napi_function_expected); \
    assert(status != napi_number_expected); \
    assert(status != napi_boolean_expected); \
    assert(status != napi_array_expected); \
    assert(status != napi_generic_failure); \
    assert(status != napi_pending_exception); \
    assert(status != napi_cancelled); \
    assert(status != napi_escape_called_twice); \
    assert(status != napi_handle_scope_mismatch); \
    assert(status != napi_callback_scope_mismatch); \
    assert(status != napi_queue_full); \
    assert(status != napi_closing); \
    assert(status != napi_bigint_expected); \
    assert(status == napi_ok); \
  } while (0)
#else
#define CHECK_NAPI(napi_call) napi_call
#endif