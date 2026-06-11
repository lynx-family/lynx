# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)
project(lynx_explorer)

set(LYNX_ROOT_PATH ${PROJECT_SOURCE_DIR}/../../../../../..)

add_library(
        lynx_napi_addon_loader
        SHARED
        ${LYNX_ROOT_PATH}/explorer/cpp/LynxNodeAPI.cc
        lynx_node_api_napi.cpp
)

if(DEFINED OHOS_ARCH AND NOT "${OHOS_ARCH}" STREQUAL "")
    set(LYNX_NODE_API_CURRENT_ABI "${OHOS_ARCH}")
else()
    set(LYNX_NODE_API_CURRENT_ABI "arm64-v8a")
endif()
set(LYNX_NODE_API_ADDON_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/napi_addons/${LYNX_NODE_API_CURRENT_ABI})

file(GLOB_RECURSE NAPI_ADAPTER_ALL_CANDIDATES
     ${LYNX_ROOT_PATH}/explorer/harmony/oh_modules/.ohpm/libnapi_adapter.so)
file(GLOB_RECURSE NAPI_ALL_CANDIDATES
     ${LYNX_ROOT_PATH}/explorer/harmony/oh_modules/.ohpm/libnapi.so)
set(NAPI_ADAPTER_CANDIDATES "")
set(NAPI_CANDIDATES "")
foreach(_candidate ${NAPI_ADAPTER_ALL_CANDIDATES})
    string(FIND "${_candidate}" "/libs/${LYNX_NODE_API_CURRENT_ABI}/" _abi_match_pos)
    if(NOT _abi_match_pos EQUAL -1)
        list(APPEND NAPI_ADAPTER_CANDIDATES ${_candidate})
    endif()
endforeach()
foreach(_candidate ${NAPI_ALL_CANDIDATES})
    string(FIND "${_candidate}" "/libs/${LYNX_NODE_API_CURRENT_ABI}/" _abi_match_pos)
    if(NOT _abi_match_pos EQUAL -1)
        list(APPEND NAPI_CANDIDATES ${_candidate})
    endif()
endforeach()
list(LENGTH NAPI_ADAPTER_CANDIDATES NAPI_ADAPTER_COUNT)
if(NAPI_ADAPTER_COUNT EQUAL 0)
    message(FATAL_ERROR
        "libnapi_adapter.so not found under ${LYNX_ROOT_PATH}/explorer/harmony/oh_modules/.ohpm for ABI ${LYNX_NODE_API_CURRENT_ABI}")
endif()
list(LENGTH NAPI_CANDIDATES NAPI_COUNT)
if(NAPI_COUNT EQUAL 0)
    message(FATAL_ERROR
        "libnapi.so not found under ${LYNX_ROOT_PATH}/explorer/harmony/oh_modules/.ohpm for ABI ${LYNX_NODE_API_CURRENT_ABI}")
endif()
list(GET NAPI_ADAPTER_CANDIDATES 0 NAPI_ADAPTER_SO)
list(GET NAPI_CANDIDATES 0 NAPI_SO)
add_library(napi_adapter SHARED IMPORTED)
set_target_properties(napi_adapter PROPERTIES IMPORTED_LOCATION ${NAPI_ADAPTER_SO})
add_library(napi SHARED IMPORTED)
set_target_properties(napi PROPERTIES IMPORTED_LOCATION ${NAPI_SO})

target_include_directories(lynx_napi_addon_loader PUBLIC
${LYNX_ROOT_PATH}/explorer/cpp
${LYNX_ROOT_PATH})

target_link_libraries(lynx_napi_addon_loader PUBLIC libace_napi.z.so libhilog_ndk.z.so napi_adapter napi)
target_compile_definitions(lynx_napi_addon_loader PUBLIC USE_WEAK_SUFFIX_NAPI=1)

# Optional Node-API addon packaging:
# Put addon files under:
#   ${CMAKE_CURRENT_SOURCE_DIR}/napi_addons/<abi>/
# All regular files under the current ABI directory are copied into the
# native libs output directory so dlopen() can resolve them from the
# default loader search paths at runtime.
file(GLOB LYNX_NODE_API_ADDON_FILES LIST_DIRECTORIES false
    ${LYNX_NODE_API_ADDON_DIR}/*)

if(LYNX_NODE_API_ADDON_FILES)
    foreach(_addon_file ${LYNX_NODE_API_ADDON_FILES})
        get_filename_component(_addon_output_name ${_addon_file} NAME)
        add_custom_command(TARGET lynx_napi_addon_loader POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_addon_file}
                $<TARGET_FILE_DIR:lynx_napi_addon_loader>/${_addon_output_name}
            VERBATIM)
    endforeach()
else()
    message(STATUS
        "No Node-API addon files found under ${LYNX_NODE_API_ADDON_DIR}")
endif()

set_target_properties(lynx_napi_addon_loader PROPERTIES CXX_STANDARD 17)
