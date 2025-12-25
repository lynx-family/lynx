# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set(CEF_WEBVIEW_TARGET "cef_webview")

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()

# Sources
set(CEF_WEBVIEW_SRCS
  cef_extension_module.cc
  cef_extension_module.h
  cef_webview_client.cc
  cef_webview_client.h
  cef_webview.cc
  cef_webview.h
  )

add_library(${CEF_WEBVIEW_TARGET} STATIC ${CEF_WEBVIEW_SRCS})

if(OS_MAC)
  list(APPEND CEF_COMPILER_FLAGS
    -fobjc-arc
    )
  set_target_properties(${CEF_WEBVIEW_TARGET} PROPERTIES
    CLANG_ENABLE_OBJC_ARC "YES"
    )
endif()

target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${LYNX_ROOT_DIR})
target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${LYNX_ROOT_DIR}/lynx)
target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${PROJECT_ROOT_DIR}/third_party/cef_binary)

target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE USE_PRIMJS_NAPI)
if(OS_MAC)
  target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE OS_MAC)
endif()
if(OS_WIN)
  target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE OS_WIN)
endif()


SET_LIBRARY_TARGET_PROPERTIES(${CEF_WEBVIEW_TARGET})

add_dependencies(${CEF_WEBVIEW_TARGET} libcef_dll_wrapper)
