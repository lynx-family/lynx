// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LBS_CORE_GRAPHICS_PATH_PARSER_H_
#define LBS_CORE_GRAPHICS_PATH_PARSER_H_
#import <CoreGraphics/CGPath.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct LBSPathConsumer LBSPathConsumer;

/**
 Creates a CGPath object from a data string.

 The path reference returned by this method is automatically retained once when
 created. It is the responsibility of the caller to release the path reference
 once it is no longer needed.

 @param data A string representation of the data used to create the CGPath
 object. This string should follow SVG path data format.

 @return A CGPath object created from the provided data string.

 @note Make sure to release the path reference by calling `CGPathRelease()` once
 it is no longer needed to avoid memory leaks.
 */
CGPathRef LBSCreatePathFromData(const char* data);

#ifdef __cplusplus
}
#endif

#endif  // LBS_CORE_GRAPHICS_PATH_PARSER_H_
