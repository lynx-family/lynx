/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

#ifndef PRIMJS_SRC_JS_NATIVE_API_H_
#define PRIMJS_SRC_JS_NATIVE_API_H_

// This file needs to be compatible with C compilers.
#include <stdbool.h>  // NOLINT(modernize-deprecated-headers)
#include <stddef.h>   // NOLINT(modernize-deprecated-headers)

#ifdef ENABLE_CODECACHE
#include <functional>
#include <vector>
#endif  // ENABLE_CODECACHE

#ifndef PRIMJS_NAPI_VERSION
// The baseline version for N-API
#define PRIMJS_NAPI_VERSION 2
#endif

#include "js_native_api_types.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
// If you need __declspec(dllimport), either include <node_api.h> instead, or
// define NAPI_EXTERN as __declspec(dllimport) on the compiler's command line.
#ifndef NAPI_EXTERN
#ifdef _WIN32
#define NAPI_EXTERN __declspec(dllexport)
#elif defined(__wasm32__)
#define NAPI_EXTERN                      \
  __attribute__((visibility("default"))) \
  __attribute__((__import_module__("napi")))
#else
#define NAPI_EXTERN __attribute__((visibility("default")))
#endif
#endif

#if defined(__GNUC__)
#define NAPI_NO_RETURN __attribute__((noreturn))
#elif defined(_WIN32)
#define NAPI_NO_RETURN __declspec(noreturn)
#else
#define NAPI_NO_RETURN
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NAPI_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define NAPI_DEPRECATED __declspec(deprecated)
#else
#define NAPI_DEPRECATED
#endif

#if (defined(__GNUC__) || defined(__clang__)) && defined(NDEBUG)
#define NAPI_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER) && defined(NDEBUG)
#define NAPI_INLINE __forceinline
#else
#define NAPI_INLINE inline
#endif

#define NAPI_AUTO_LENGTH SIZE_MAX

#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

#if defined(ENABLE_CODECACHE) && defined(PROFILE_CODECACHE) && \
    !defined(BAZEL_BUILD)
#include <chrono>

#include "basic/log/logging.h"
#define LOG_TIME_START() \
  auto _t_start_ = std::chrono::high_resolution_clock::now()

#define LOG_TIME_END(...)                                   \
  auto _t_end_ = std::chrono::high_resolution_clock::now(); \
  std::chrono::duration<double, std::milli> interval_time = \
      _t_end_ - _t_start_;                                  \
  VLOGD("time comsumption: %f ms", interval_time.count());  \
  VLOGD(__VA_ARGS__)
#else
#define LOG_TIME_START()
#define LOG_TIME_END(...)
#endif

struct napi_env__ {
  napi_state state;
  napi_runtime rt;
  napi_context ctx;

  // Warning: Keep in-sync with macros in napi_macro.h!
  // Always append function at the end to keep ABI compatible!

  napi_status (*napi_get_version)(napi_env env, uint32_t* result);

  // ENGINE CALL
  // Getters for defined singletons
  napi_status (*napi_get_undefined)(napi_env env, napi_value* result);
  napi_status (*napi_get_null)(napi_env env, napi_value* result);
  napi_status (*napi_get_global)(napi_env env, napi_value* result);
  napi_status (*napi_get_boolean)(napi_env env, bool value, napi_value* result);

  // Methods to create Primitive types/Objects
  napi_status (*napi_create_object)(napi_env env, napi_value* result);
  napi_status (*napi_create_array)(napi_env env, napi_value* result);
  napi_status (*napi_create_array_with_length)(napi_env env, size_t length,
                                               napi_value* result);
  napi_status (*napi_create_double)(napi_env env, double value,
                                    napi_value* result);
  napi_status (*napi_create_int32)(napi_env env, int32_t value,
                                   napi_value* result);
  napi_status (*napi_create_uint32)(napi_env env, uint32_t value,
                                    napi_value* result);
  napi_status (*napi_create_int64)(napi_env env, int64_t value,
                                   napi_value* result);
  napi_status (*napi_create_string_latin1)(napi_env env, const char* str,
                                           size_t length, napi_value* result);
  napi_status (*napi_create_string_utf8)(napi_env env, const char* str,
                                         size_t length, napi_value* result);
  napi_status (*napi_create_string_utf16)(napi_env env, const char16_t* str,
                                          size_t length, napi_value* result);
  napi_status (*napi_create_symbol)(napi_env env, napi_value description,
                                    napi_value* result);
  napi_status (*napi_create_function)(napi_env env, const char* utf8name,
                                      size_t length, napi_callback cb,
                                      void* data, napi_value* result);
  napi_status (*napi_create_error)(napi_env env, napi_value code,
                                   napi_value msg, napi_value* result);
  napi_status (*napi_create_type_error)(napi_env env, napi_value code,
                                        napi_value msg, napi_value* result);
  napi_status (*napi_create_range_error)(napi_env env, napi_value code,
                                         napi_value msg, napi_value* result);

  // Methods to get the native napi_value from Primitive type
  napi_status (*napi_typeof)(napi_env env, napi_value value,
                             napi_valuetype* result);
  napi_status (*napi_get_value_double)(napi_env env, napi_value value,
                                       double* result);
  napi_status (*napi_get_value_int32)(napi_env env, napi_value value,
                                      int32_t* result);
  napi_status (*napi_get_value_uint32)(napi_env env, napi_value value,
                                       uint32_t* result);
  napi_status (*napi_get_value_int64)(napi_env env, napi_value value,
                                      int64_t* result);
  napi_status (*napi_get_value_bool)(napi_env env, napi_value value,
                                     bool* result);

  // Copies LATIN-1 encoded bytes from a string into a buffer.
  napi_status (*napi_get_value_string_latin1)(napi_env env, napi_value value,
                                              char* buf, size_t bufsize,
                                              size_t* result);

  // Copies UTF-8 encoded bytes from a string into a buffer.
  napi_status (*napi_get_value_string_utf8)(napi_env env, napi_value value,
                                            char* buf, size_t bufsize,
                                            size_t* result);

  // Copies UTF-16 encoded bytes from a string into a buffer.
  napi_status (*napi_get_value_string_utf16)(napi_env env, napi_value value,
                                             char16_t* buf, size_t bufsize,
                                             size_t* result);

  // Methods to coerce values
  // These APIs may execute user scripts
  napi_status (*napi_coerce_to_bool)(napi_env env, napi_value value,
                                     napi_value* result);
  napi_status (*napi_coerce_to_number)(napi_env env, napi_value value,
                                       napi_value* result);
  napi_status (*napi_coerce_to_object)(napi_env env, napi_value value,
                                       napi_value* result);
  napi_status (*napi_coerce_to_string)(napi_env env, napi_value value,
                                       napi_value* result);

  // Methods to work with Objects
  napi_status (*napi_get_prototype)(napi_env env, napi_value object,
                                    napi_value* result);
  napi_status (*napi_get_property_names)(napi_env env, napi_value object,
                                         napi_value* result);
  napi_status (*napi_set_property)(napi_env env, napi_value object,
                                   napi_value key, napi_value value);
  napi_status (*napi_has_property)(napi_env env, napi_value object,
                                   napi_value key, bool* result);
  napi_status (*napi_get_property)(napi_env env, napi_value object,
                                   napi_value key, napi_value* result);
  napi_status (*napi_delete_property)(napi_env env, napi_value object,
                                      napi_value key, bool* result);
  napi_status (*napi_has_own_property)(napi_env env, napi_value object,
                                       napi_value key, bool* result);
  napi_status (*napi_set_named_property)(napi_env env, napi_value object,
                                         const char* utf8name,
                                         napi_value value);
  napi_status (*napi_has_named_property)(napi_env env, napi_value object,
                                         const char* utf8name, bool* result);
  napi_status (*napi_get_named_property)(napi_env env, napi_value object,
                                         const char* utf8name,
                                         napi_value* result);
  napi_status (*napi_set_element)(napi_env env, napi_value object,
                                  uint32_t index, napi_value value);
  napi_status (*napi_has_element)(napi_env env, napi_value object,
                                  uint32_t index, bool* result);
  napi_status (*napi_get_element)(napi_env env, napi_value object,
                                  uint32_t index, napi_value* result);
  napi_status (*napi_delete_element)(napi_env env, napi_value object,
                                     uint32_t index, bool* result);

  napi_status (*napi_define_properties)(
      napi_env env, napi_value object, size_t property_count,
      const napi_property_descriptor* properties);

  // Methods to work with Arrays
  napi_status (*napi_is_array)(napi_env env, napi_value value, bool* result);
  napi_status (*napi_get_array_length)(napi_env env, napi_value value,
                                       uint32_t* result);

  // Methods to compare values
  napi_status (*napi_strict_equals)(napi_env env, napi_value lhs,
                                    napi_value rhs, bool* result);

  // Methods to work with Functions
  napi_status (*napi_call_function)(napi_env env, napi_value recv,
                                    napi_value func, size_t argc,
                                    const napi_value* argv, napi_value* result);
  napi_status (*napi_new_instance)(napi_env env, napi_value constructor,
                                   size_t argc, const napi_value* argv,
                                   napi_value* result);
  napi_status (*napi_instanceof)(napi_env env, napi_value object,
                                 napi_value constructor, bool* result);

  // Methods to work with napi_callbacks

  // Gets all callback info in a single call. (Ugly, but faster.)
  napi_status (*napi_get_cb_info)(
      napi_env env,               // [in] NAPI environment handle
      napi_callback_info cbinfo,  // [in] Opaque callback-info handle
      size_t* argc,  // [in-out] Specifies the size of the provided argv array
                     // and receives the actual count of args.
      napi_value* argv,      // [out] Array of values
      napi_value* this_arg,  // [out] Receives the JS 'this' arg for the call
      void** data);  // [out] Receives the data pointer for the callback.

  napi_status (*napi_get_new_target)(napi_env env, napi_callback_info cbinfo,
                                     napi_value* result);

  napi_status (*napi_define_class)(napi_env env, const char* utf8name,
                                   size_t length, napi_callback constructor,
                                   void* data, size_t property_count,
                                   const napi_property_descriptor* properties,
                                   napi_class super_class, napi_class* result);

  napi_status (*napi_release_class)(napi_env env, napi_class clazz);

  napi_status (*napi_class_get_function)(napi_env env, napi_class clazz,
                                         napi_value* result);

  // Methods to work with external data objects
  napi_status (*napi_wrap)(napi_env env, napi_value js_object,
                           void* native_object, napi_finalize finalize_cb,
                           void* finalize_hint, napi_ref* result);
  napi_status (*napi_unwrap)(napi_env env, napi_value js_object, void** result);
  napi_status (*napi_remove_wrap)(napi_env env, napi_value js_object,
                                  void** result);
  napi_status (*napi_create_external)(napi_env env, void* data,
                                      napi_finalize finalize_cb,
                                      void* finalize_hint, napi_value* result);
  napi_status (*napi_get_value_external)(napi_env env, napi_value value,
                                         void** result);

  // Methods to control object lifespan

  // Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
  napi_status (*napi_create_reference)(napi_env env, napi_value value,
                                       uint32_t initial_refcount,
                                       napi_ref* result);

  // Deletes a reference. The referenced value is released, and may
  // be GC'd unless there are other references to it.
  napi_status (*napi_delete_reference)(napi_env env, napi_ref ref);

  // Increments the reference count, optionally returning the resulting count.
  // After this call the  reference will be a strong reference because its
  // refcount is >0, and the referenced object is effectively "pinned".
  // Calling this when the refcount is 0 and the object is unavailable
  // results in an error.
  napi_status (*napi_reference_ref)(napi_env env, napi_ref ref,
                                    uint32_t* result);

  // Decrements the reference count, optionally returning the resulting count.
  // If the result is 0 the reference is now weak and the object may be GC'd
  // at any time if there are no other references. Calling this when the
  // refcount is already 0 results in an error.
  napi_status (*napi_reference_unref)(napi_env env, napi_ref ref,
                                      uint32_t* result);

  // Attempts to get a referenced value. If the reference is weak,
  // the value might no longer be available, in that case the call
  // is still successful but the result is NULL.
  napi_status (*napi_get_reference_value)(napi_env env, napi_ref ref,
                                          napi_value* result);

  napi_status (*napi_open_handle_scope)(napi_env env,
                                        napi_handle_scope* result);
  napi_status (*napi_close_handle_scope)(napi_env env, napi_handle_scope scope);
  napi_status (*napi_open_escapable_handle_scope)(
      napi_env env, napi_escapable_handle_scope* result);
  napi_status (*napi_close_escapable_handle_scope)(
      napi_env env, napi_escapable_handle_scope scope);

  napi_status (*napi_escape_handle)(napi_env env,
                                    napi_escapable_handle_scope scope,
                                    napi_value escapee, napi_value* result);

  // Methods to support error handling
  napi_status (*napi_throw_)(napi_env env, napi_value error);
  napi_status (*napi_throw_error)(napi_env env, const char* code,
                                  const char* msg);
  napi_status (*napi_throw_type_error)(napi_env env, const char* code,
                                       const char* msg);
  napi_status (*napi_throw_range_error)(napi_env env, const char* code,
                                        const char* msg);
  napi_status (*napi_is_error)(napi_env env, napi_value value, bool* result);

  // Methods to support catching exceptions
  napi_status (*napi_is_exception_pending)(napi_env env, bool* result);
  napi_status (*napi_get_and_clear_last_exception)(napi_env env,
                                                   napi_value* result);

  // Methods to work with array buffers and typed arrays
  napi_status (*napi_is_arraybuffer)(napi_env env, napi_value value,
                                     bool* result);
  napi_status (*napi_create_arraybuffer)(napi_env env, size_t byte_length,
                                         void** data, napi_value* result);
  napi_status (*napi_create_external_arraybuffer)(
      napi_env env, void* external_data, size_t byte_length,
      napi_finalize finalize_cb, void* finalize_hint, napi_value* result);
  napi_status (*napi_get_arraybuffer_info)(napi_env env, napi_value arraybuffer,
                                           void** data, size_t* byte_length);
  napi_status (*napi_is_typedarray)(napi_env env, napi_value value,
                                    bool* result);
  napi_status (*napi_create_typedarray)(napi_env env, napi_typedarray_type type,
                                        size_t length, napi_value arraybuffer,
                                        size_t byte_offset, napi_value* result);

  napi_status (*napi_is_typedarray_of)(napi_env env, napi_value typedarray,
                                       napi_typedarray_type type, bool* result);

  napi_status (*napi_get_typedarray_info)(napi_env env, napi_value typedarray,
                                          napi_typedarray_type* type,
                                          size_t* length, void** data,
                                          napi_value* arraybuffer,
                                          size_t* byte_offset);

  napi_status (*napi_create_dataview)(napi_env env, size_t length,
                                      napi_value arraybuffer,
                                      size_t byte_offset, napi_value* result);
  napi_status (*napi_is_dataview)(napi_env env, napi_value value, bool* result);
  napi_status (*napi_get_dataview_info)(napi_env env, napi_value dataview,
                                        size_t* bytelength, void** data,
                                        napi_value* arraybuffer,
                                        size_t* byte_offset);

  // Promises
  napi_status (*napi_create_promise)(napi_env env, napi_deferred* deferred,
                                     napi_value* promise);
  napi_status (*napi_release_deferred)(napi_env env, napi_deferred deferred,
                                       napi_value resolution,
                                       napi_deferred_release_mode mode);
  napi_status (*napi_is_promise)(napi_env env, napi_value value,
                                 bool* is_promise);

  // Running a script
  napi_status (*napi_run_script)(napi_env env, const char* script,
                                 size_t length, const char* filename,
                                 napi_value* result);

  // Memory management
  napi_status (*napi_adjust_external_memory)(napi_env env,
                                             int64_t change_in_bytes,
                                             int64_t* adjusted_value);

  // Add finalizer for pointer
  napi_status (*napi_add_finalizer)(napi_env env, napi_value js_object,
                                    void* native_object,
                                    napi_finalize finalize_cb,
                                    void* finalize_hint, napi_ref* result);

  // Instance data
  napi_status (*napi_set_instance_data)(napi_env env, uint64_t key, void* data,
                                        napi_finalize finalize_cb,
                                        void* finalize_hint);

  napi_status (*napi_get_instance_data)(napi_env env, uint64_t key,
                                        void** data);

  // ENGINE CALL END

  // Universal CALL
  napi_status (*napi_get_last_error_info)(
      napi_env env, const napi_extended_error_info** result);

  napi_status (*napi_add_env_cleanup_hook)(napi_env env, void (*fun)(void* arg),
                                           void* arg);
  napi_status (*napi_remove_env_cleanup_hook)(napi_env env,
                                              void (*fun)(void* arg),
                                              void* arg);

  napi_status (*napi_create_async_work)(napi_env env, napi_value async_resource,
                                        napi_value async_resource_name,
                                        napi_async_execute_callback execute,
                                        napi_async_complete_callback complete,
                                        void* data, napi_async_work* result);

  napi_status (*napi_delete_async_work)(napi_env env, napi_async_work work);
  napi_status (*napi_queue_async_work)(napi_env env, napi_async_work work);
  napi_status (*napi_cancel_async_work)(napi_env env, napi_async_work work);

  // Calling into JS from other threads
  napi_status (*napi_create_threadsafe_function)(
      napi_env env, void* thread_finalize_data,
      napi_finalize thread_finalize_cb, void* context,
      napi_threadsafe_function_call_js call_js_cb,
      napi_threadsafe_function* result);

  napi_status (*napi_get_threadsafe_function_context)(
      napi_threadsafe_function func, void** result);

  napi_status (*napi_call_threadsafe_function)(
      napi_threadsafe_function func, void* data,
      napi_threadsafe_function_call_mode is_blocking);

  // no need to acquire tsfn, just hold the pointer and release in any thread
  napi_status NAPI_DEPRECATED (*napi_acquire_threadsafe_function)(
      napi_threadsafe_function func);

  napi_status (*napi_delete_threadsafe_function)(napi_threadsafe_function func);

  // no need to ref thread
  napi_status NAPI_DEPRECATED (*napi_unref_threadsafe_function)(
      napi_env env, napi_threadsafe_function func);

  // no need to ref thread
  napi_status NAPI_DEPRECATED (*napi_ref_threadsafe_function)(
      napi_env env, napi_threadsafe_function func);

  napi_status (*napi_get_loader)(napi_env env, napi_value* result);

  // UNIVERSAL CALL END

  napi_status (*napi_open_context_scope)(napi_env env,
                                         napi_context_scope* result);
  napi_status (*napi_close_context_scope)(napi_env env,
                                          napi_context_scope scope);

  napi_status (*napi_open_error_scope)(napi_env env, napi_error_scope* result);
  napi_status (*napi_close_error_scope)(napi_env env, napi_error_scope scope);

  // loose equals
  napi_status (*napi_equals)(napi_env env, napi_value lhs, napi_value rhs,
                             bool* result);

  napi_status (*napi_get_unhandled_rejection_exception)(napi_env env,
                                                        napi_value* result);
  napi_status (*napi_get_own_property_descriptor)(napi_env env, napi_value obj,
                                                  napi_value prop,
                                                  napi_value* result);
#ifdef ENABLE_CODECACHE
  napi_status (*napi_post_worker_task)(napi_env env,
                                       std::function<void()> task);
  napi_status (*napi_store_code_cache)(napi_env env,
                                       const std::string& filename,
                                       const uint8_t* data, int length);
  napi_status (*napi_get_code_cache)(napi_env env, const std::string& filename,
                                     const uint8_t** data, int* length);
  napi_status (*napi_output_code_cache)(napi_env env,
                                        unsigned int place_holder);
  // this interface may be optimized by
  // discarding the parameter of std::function type
  napi_status (*napi_init_code_cache)(napi_env env, int capacity,
                                      const std::string& cache_file,
                                      std::function<void(bool)> callback);
  napi_status (*napi_dump_code_cache_status)(napi_env env, void* dump_vec);
  napi_status (*napi_run_script_cache)(napi_env env, const char* script,
                                       size_t length, const char* filename,
                                       napi_value* result);
  napi_status (*napi_run_code_cache)(napi_env env, const uint8_t* data,
                                     int length, napi_value* result);
  napi_status (*napi_gen_code_cache)(napi_env env, const char* script,
                                     size_t script_len, const uint8_t** data,
                                     int* length);
#endif  // ENABLE_CODECACHE
};

#ifdef ENABLE_CODECACHE
#define NAPI_RUNTIME_CODECACHE_CALL(V) \
  V(post_worker_task)                  \
  V(store_code_cache)                  \
  V(get_code_cache)                    \
  V(output_code_cache)                 \
  V(init_code_cache)                   \
  V(dump_code_cache_status)

#define NAPI_ENGINE_CACHE_CALL(V) \
  V(run_script_cache)             \
  V(run_code_cache)               \
  V(gen_code_cache)
#else
#define NAPI_RUNTIME_CODECACHE_CALL(V)
#define NAPI_ENGINE_CACHE_CALL(V)
#endif  // ENABLE_CODECACHE

// These functions are different in JS engines
#define FOR_EACH_NAPI_ENGINE_CALL(V)   \
  V(get_undefined)                     \
  V(get_null)                          \
  V(get_global)                        \
  V(get_boolean)                       \
  V(create_object)                     \
  V(create_array)                      \
  V(create_array_with_length)          \
  V(create_double)                     \
  V(create_int32)                      \
  V(create_uint32)                     \
  V(create_int64)                      \
  V(create_string_latin1)              \
  V(create_string_utf8)                \
  V(create_string_utf16)               \
  V(create_symbol)                     \
  V(create_function)                   \
  V(create_error)                      \
  V(create_type_error)                 \
  V(create_range_error)                \
  V(typeof)                            \
  V(get_value_double)                  \
  V(get_value_int32)                   \
  V(get_value_uint32)                  \
  V(get_value_int64)                   \
  V(get_value_bool)                    \
  V(get_value_string_latin1)           \
  V(get_value_string_utf8)             \
  V(get_value_string_utf16)            \
  V(coerce_to_bool)                    \
  V(coerce_to_number)                  \
  V(coerce_to_object)                  \
  V(coerce_to_string)                  \
  V(get_prototype)                     \
  V(get_property_names)                \
  V(set_property)                      \
  V(has_property)                      \
  V(get_property)                      \
  V(delete_property)                   \
  V(has_own_property)                  \
  V(set_named_property)                \
  V(has_named_property)                \
  V(get_named_property)                \
  V(set_element)                       \
  V(has_element)                       \
  V(get_element)                       \
  V(delete_element)                    \
  V(define_properties)                 \
  V(is_array)                          \
  V(get_array_length)                  \
  V(strict_equals)                     \
  V(equals)                            \
  V(call_function)                     \
  V(new_instance)                      \
  V(instanceof)                        \
  V(get_cb_info)                       \
  V(get_new_target)                    \
  V(define_class)                      \
  V(release_class)                     \
  V(class_get_function)                \
  V(wrap)                              \
  V(unwrap)                            \
  V(remove_wrap)                       \
  V(create_external)                   \
  V(get_value_external)                \
  V(create_reference)                  \
  V(delete_reference)                  \
  V(reference_ref)                     \
  V(reference_unref)                   \
  V(get_reference_value)               \
  V(open_handle_scope)                 \
  V(close_handle_scope)                \
  V(open_escapable_handle_scope)       \
  V(close_escapable_handle_scope)      \
  V(escape_handle)                     \
  V(throw_)                            \
  V(throw_error)                       \
  V(throw_type_error)                  \
  V(throw_range_error)                 \
  V(is_error)                          \
  V(is_exception_pending)              \
  V(get_and_clear_last_exception)      \
  V(get_unhandled_rejection_exception) \
  V(is_arraybuffer)                    \
  V(create_arraybuffer)                \
  V(create_external_arraybuffer)       \
  V(get_arraybuffer_info)              \
  V(is_typedarray)                     \
  V(create_typedarray)                 \
  V(is_typedarray_of)                  \
  V(get_typedarray_info)               \
  V(create_dataview)                   \
  V(is_dataview)                       \
  V(get_dataview_info)                 \
  V(create_promise)                    \
  V(release_deferred)                  \
  V(is_promise)                        \
  V(run_script)                        \
  V(adjust_external_memory)            \
  V(add_finalizer)                     \
  V(set_instance_data)                 \
  V(get_instance_data)                 \
  V(open_context_scope)                \
  V(close_context_scope)               \
  V(get_own_property_descriptor)       \
  NAPI_ENGINE_CACHE_CALL(V)

// These functions share same implementations across JS engines
#define FOR_EACH_NAPI_ENV_CALL(V) \
  V(get_last_error_info)          \
  V(get_version)                  \
  V(add_env_cleanup_hook)         \
  V(remove_env_cleanup_hook)      \
  V(get_loader)

// These functions are different in different JS runtimes
#define FOR_EACH_NAPI_RUNTIME_CALL(V) \
  V(create_async_work)                \
  V(delete_async_work)                \
  V(queue_async_work)                 \
  V(cancel_async_work)                \
  V(create_threadsafe_function)       \
  V(get_threadsafe_function_context)  \
  V(call_threadsafe_function)         \
  V(delete_threadsafe_function)       \
  V(open_error_scope)                 \
  V(close_error_scope)                \
  NAPI_RUNTIME_CODECACHE_CALL(V)

#define NAPI_ENV_CALL(API, ENV, ...) \
  napi_env(ENV)->napi_##API((ENV), __VA_ARGS__)

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_JS_NATIVE_API_H_
