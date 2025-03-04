# Set the minimum version of CMAKE that is required
cmake_minimum_required(VERSION 3.4.1)

# According to the build variant, 
# import the CMakeLists-impl.cmake file to compile the native methods.
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_impl/${FLAVOR_NAME}${BUILD_TYPE}${ANDROID_ABI}/CMakeLists.txt)