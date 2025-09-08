// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_
#define BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "base/include/value/lynx_value_types.h"

#ifdef _WIN32
#ifdef lynx_EXPORTS
#define LYNX_VALUE_EXPORT __declspec(dllexport)
#else
#define LYNX_VALUE_EXPORT __declspec(dllimport)
#endif  // lynx_EXPORTS
#elif defined(__ANDROID__)
#define LYNX_VALUE_EXPORT
#else
#define LYNX_VALUE_EXPORT __attribute__((visibility("default")))
#endif  // _WIN32

// Get the type of lynx_value.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_typeof(lynx_api_env env,
                                                    lynx_value value,
                                                    lynx_value_type* result);
// This API creates a lynx_value of type null.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_null(lynx_api_env env,
                                                         lynx_value* result);
// This API creates a lynx_value of type bool with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_bool(lynx_api_env env,
                                                         bool value,
                                                         lynx_value* result);
// This API creates a lynx_value of type bool with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_double(lynx_api_env env,
                                                           double value,
                                                           lynx_value* result);
// This API creates a lynx_value of type int32_t with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_int32(lynx_api_env env,
                                                          int32_t value,
                                                          lynx_value* result);
// This API creates a lynx_value of type uint32_t with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_uint32(lynx_api_env env,
                                                           uint32_t value,
                                                           lynx_value* result);
// This API creates a lynx_value of type int64_t with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_int64(lynx_api_env env,
                                                          int64_t value,
                                                          lynx_value* result);
// This API creates a lynx_value of type uint64_t with given values.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_uint64(lynx_api_env env,
                                                           uint64_t value,
                                                           lynx_value* result);
// This API creates a string value from a UTF8-encoded C string. The native
// string is copied.
// This API allocates memory on the heap to store the string. After use,
// lynx_value_remove_reference must be called to free this portion of
// memory, otherwise it will result in a memory leak.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_string_utf8(
    lynx_api_env env, const char* str, size_t length, lynx_value* result);
// This API creates a map container.
// This API allocates memory on the heap to store the container. After use,
// lynx_value_remove_reference must be called to free this portion of
// memory, otherwise it will result in a memory leak.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_map(lynx_api_env env,
                                                        lynx_value* result);
// This API creates a array container.
// This API allocates memory on the heap to store the container. After use,
// lynx_value_remove_reference must be called to free this portion of
// memory, otherwise it will result in a memory leak.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_create_array(lynx_api_env env,
                                                          lynx_value* result);

// Read the double value from lynx_value. lynx_api_double_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_double(lynx_api_env env,
                                                        lynx_value value,
                                                        double* result);
// Read the int32_t value from lynx_value. lynx_api_int32_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_int32(lynx_api_env env,
                                                       lynx_value value,
                                                       int32_t* result);
// Read the uint32_t value from lynx_value. lynx_api_uint32_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_uint32(lynx_api_env env,
                                                        lynx_value value,
                                                        uint32_t* result);
// Read the int64_t value from lynx_value. lynx_api_int64_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_int64(lynx_api_env env,
                                                       lynx_value value,
                                                       int64_t* result);
// Read the uint64_t value from lynx_value. lynx_api_uint64_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_uint64(lynx_api_env env,
                                                        lynx_value value,
                                                        uint64_t* result);
// Read the bool value from lynx_value. lynx_api_bool_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_bool(lynx_api_env env,
                                                      lynx_value value,
                                                      bool* result);
// Read the UTF8-encoded string value from lynx_value. lynx_api_string_expected
// is returned if the types do not match. If the buf argument is passed NULL,
// the length of the string in bytes and excluding the null terminator is
// returned in result
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_string_utf8(lynx_api_env env,
                                                             lynx_value value,
                                                             char* buf,
                                                             size_t bufsize,
                                                             size_t* result);
// Read the length of array from lynx_value. lynx_api_array_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_array_length(lynx_api_env env,
                                                              lynx_value value,
                                                              uint32_t* result);
// Insert lynx_value element into array at the index specified.
// lynx_api_array_expected is returned if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_set_element(lynx_api_env env,
                                                         lynx_value object,
                                                         uint32_t index,
                                                         lynx_value value);
// Read the array element from lynx_value. lynx_api_array_expected is returned
// if the types do not match.
// The reference count of the lynx_value read out is automatically incremented
// by 1. After use, lynx_value_remove_reference needs to be called to free the
// memory Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_element(lynx_api_env env,
                                                         lynx_value object,
                                                         uint32_t index,
                                                         lynx_value* result);
// This API checks if the object passed in has the named property.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_has_property(lynx_api_env env,
                                                          lynx_value object,
                                                          const char* utf8name,
                                                          bool* result);
// Read the map keys from lynx_value. lynx_api_map_expected is returned
// if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_get_property_names(
    lynx_api_env env, lynx_value object, lynx_value* result);
// Set lynx_value property into map at the key specified.
// lynx_api_map_expected is returned if the types do not match.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status
lynx_value_set_named_property(lynx_api_env env, lynx_value object,
                              const char* utf8name, lynx_value value);
// Read the map property from lynx_value. lynx_api_map_expected is returned
// if the types do not match.
// The reference count of the lynx_value read out is automatically incremented
// by 1. After use, lynx_value_remove_reference needs to be called to free the
// memory. Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status
lynx_value_get_named_property(lynx_api_env env, lynx_value object,
                              const char* utf8name, lynx_value* result);
// This API is used for iterating through array or map type lynx_value.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_iterate_value(
    lynx_api_env env, lynx_value object, lynx_value_iterator_callback callback,
    void* pfunc, void* raw_data);
// Increment the reference count by 1 for pointer data.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_add_reference(
    lynx_api_env env, lynx_value value, lynx_value_ref* result);
// Decrement the reference count by 1 for pointer data.
// Returns lynx_api_ok if the API succeeded.
LYNX_VALUE_EXPORT lynx_api_status lynx_value_remove_reference(
    lynx_api_env env, lynx_value value, lynx_value_ref ref);

#ifdef __cplusplus
}
#endif

#endif  // BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_
