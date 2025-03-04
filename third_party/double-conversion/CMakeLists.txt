cmake_minimum_required(VERSION 3.0)
project(double-conversion VERSION 3.2.0)

set(headers
    double-conversion/bignum.h
    double-conversion/cached-powers.h
    double-conversion/diy-fp.h
    double-conversion/double-conversion.h
    double-conversion/double-to-string.h
    double-conversion/fast-dtoa.h
    double-conversion/fixed-dtoa.h
    double-conversion/ieee.h
    double-conversion/string-to-double.h
    double-conversion/strtod.h
    double-conversion/utils.h)

add_library(double-conversion
            STATIC
            double-conversion/bignum.cc
            double-conversion/bignum-dtoa.cc
            double-conversion/cached-powers.cc
            double-conversion/double-to-string.cc
            double-conversion/fast-dtoa.cc
            double-conversion/fixed-dtoa.cc
            double-conversion/string-to-double.cc
            double-conversion/strtod.cc
            ${headers})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR})
