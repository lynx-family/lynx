# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)

project("lynx_explorer")

set(LYNX_ROOT_PATH ${PROJECT_SOURCE_DIR}/../../../)

add_library(
        lynx_napi_addon_loader
        SHARED
        ../../cpp/LynxNodeAPI.cc
        ../../cpp/LynxNodeAPIJNI.cc
)

target_include_directories(lynx_napi_addon_loader PUBLIC
${LYNX_ROOT_PATH})

add_library(napi_adapter SHARED IMPORTED)
set_target_properties(napi_adapter PROPERTIES IMPORTED_LOCATION
    ${LYNX_ROOT_PATH}/platform/android/lynx_android/build/jniLibs/primjs/jni/${ANDROID_ABI}/libnapi_adapter.so
)

target_link_libraries(lynx_napi_addon_loader log napi_adapter)

# Optional Node-API addon packaging:
# Put addon files under:
#   ${PROJECT_SOURCE_DIR}/napi_addons/<abi>/
# All regular files under the current ABI directory are copied into the
# native libs output directory so dlopen() can resolve them from the
# default loader search paths at runtime.
set(LYNX_NODE_API_ADDON_DIR ${PROJECT_SOURCE_DIR}/napi_addons/${ANDROID_ABI})
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
