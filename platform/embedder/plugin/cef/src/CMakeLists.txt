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

set(CEF_WEBVIEW_SRCS_MAC
  macos/CEFTextInputClientOSRMac.h
  macos/CEFTextInputClientOSRMac.mm
  macos/CEFWebviewClientMac.h
  macos/CEFWebviewClientMac.mm
  macos/CEFWebviewInitializer.mm
  macos/CEFWebviewMac.h
  macos/CEFWebviewMac.mm
  macos/NSApplication+HandlingSendEvent.h
  macos/NSApplication+HandlingSendEvent.m
  )

set(CEF_WEBVIEW_SRCS_WINDOWS
  win/cef_webview_client_win.cc
  win/cef_webview_client_win.h
  win/cef_webview_initializer.cc
  win/cef_webview_win.cc
  win/cef_webview_win.h
  win/dpi_utils_win32.cc
  win/dpi_utils_win32.h
  win/osr_ime_handler_win.cc
  win/osr_ime_handler_win.h
  )

LIBCEF_APPEND_PLATFORM_SOURCES(CEF_WEBVIEW_SRCS)

add_library(${CEF_WEBVIEW_TARGET} STATIC ${CEF_WEBVIEW_SRCS})

target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${LYNX_ROOT_DIR})
target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${LYNX_ROOT_DIR}/lynx)
target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${PROJECT_ROOT_DIR}/third_party/cef_binary)
target_include_directories(${CEF_WEBVIEW_TARGET} PRIVATE ${LYNX_ROOT_DIR}/lynx/third_party/weak-node-api/vendor)

target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE USE_PRIMJS_NAPI)
if(OS_MAC)
  target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE OS_MAC)
endif()
if(OS_WINDOWS)
  target_compile_definitions(${CEF_WEBVIEW_TARGET} PRIVATE OS_WIN lynx_EXPORTS)
  target_compile_options(${CEF_WEBVIEW_TARGET} PRIVATE /wd4267)
endif()


SET_LIBRARY_TARGET_PROPERTIES(${CEF_WEBVIEW_TARGET})

add_dependencies(${CEF_WEBVIEW_TARGET} libcef_dll_wrapper)

#
# macOS configuration.
#
if(OS_MAC)
  list(APPEND CEF_COMPILER_FLAGS
    -fobjc-arc
    )
  set_target_properties(${CEF_WEBVIEW_TARGET} PROPERTIES
    CLANG_ENABLE_OBJC_ARC "YES"
    )
  
  # Copy the CEF framework into the build/frameworks directory and create the necessary symlinks.
  COPY_MAC_FRAMEWORK("${CEF_WEBVIEW_TARGET}" "${CEF_BINARY_DIR}" "${FRAMEWORK_DST_PATH}")

  # CEF Helper
  set(CEF_WEBVIEW_HELPER_SRCS_MAC
    macos/process_helper_mac.cc
    )
  APPEND_PLATFORM_SOURCES(CEF_WEBVIEW_HELPER_SRCS)
  set(CEF_HELPER_TARGET "${HOST_EXECUTABLE_NAME}_Helper")
  set(CEF_HELPER_OUTPUT_NAME "${HOST_EXECUTABLE_NAME} Helper")
  # Create the multiple Helper app bundle targets.
  foreach(_suffix_list ${CEF_HELPER_APP_SUFFIXES})
    # Convert to a list and extract the suffix values.
    string(REPLACE ":" ";" _suffix_list ${_suffix_list})
    list(GET _suffix_list 0 _name_suffix)
    list(GET _suffix_list 1 _target_suffix)
    list(GET _suffix_list 2 _plist_suffix)
    # Define Helper target and output names.
    set(_helper_target "${CEF_HELPER_TARGET}${_target_suffix}")
    set(_helper_output_name "${CEF_HELPER_OUTPUT_NAME}${_name_suffix}")
    # Create Helper-specific variants of the helper-Info.plist file. Do this
    # manually because the configure_file command (which is executed as part of
    # MACOSX_BUNDLE_INFO_PLIST) uses global env variables and would insert the
    # wrong values with multiple targets.
    set(_helper_info_plist "${CMAKE_CURRENT_BINARY_DIR}/macos/helper-Info${_target_suffix}.plist")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/macos/Helper-Info.plist.in" _plist_contents)
    string(REPLACE "\${EXECUTABLE_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
    string(REPLACE "\${PRODUCT_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
    string(REPLACE "\${BUNDLE_ID_SUFFIX}" "${_plist_suffix}" _plist_contents ${_plist_contents})
    file(WRITE ${_helper_info_plist} ${_plist_contents})
    # Create Helper executable target.
    add_executable(${_helper_target} MACOSX_BUNDLE ${CEF_WEBVIEW_HELPER_SRCS_MAC})
    SET_EXECUTABLE_TARGET_PROPERTIES(${_helper_target})
    add_dependencies(${_helper_target} libcef_dll_wrapper)
    target_link_libraries(${_helper_target} libcef_dll_wrapper ${CEF_STANDARD_LIBS})
    set_target_properties(${_helper_target} PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST ${_helper_info_plist}
      OUTPUT_NAME ${_helper_output_name}
      )
    # Add the Helper as a dependency of the main executable target.
    add_dependencies(${CEF_WEBVIEW_TARGET} "${_helper_target}")
    add_custom_command(
        TARGET ${CEF_WEBVIEW_TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CEF_TARGET_OUT_DIR}/${_helper_output_name}.app"
                "${FRAMEWORK_DST_PATH}/Contents/Frameworks/${_helper_output_name}.app"
        VERBATIM
        )
  endforeach()
endif() # OS_MAC

#
# Windows configuration.
#
if(OS_WINDOWS)
endif()
