// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_BASIC_SHAPE_PATH_CONSUMER_H_
#define LYNX_BASIC_SHAPE_PATH_CONSUMER_H_

#import <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum LBSPathConsumerType {
  kLBSPathConsumerTypeUnknown = 0,
  kLBSPathConsumerTypeCoreGraphics = 1,
  kLBSPathConsumerTypeString = 2,
} LBSPathConsumerType;

typedef struct LBSPathConsumer {
  int type;   // The type of the path consumer, limited to String and
              // CoreGraphics
  void* ctx;  // The context associated with the path consumer, and will be pass
              // to the function.
  int error;
  void (*MoveToPoint)(void* ctx, float x,
                      float y);  // Function to handle 'M', 'm' path seg.
  void (*LineToPoint)(
      void* ctx, float x,
      float y);  // Function to handle 'L', 'l', 'V', 'v', 'H' and 'h'.
  void (*CubicToPoint)(void* ctx, float cp1x, float cp1y, float cp2x,
                       float cp2y, float x,
                       float y);  // Function to handle 'C', 'c', 'S', 's'.
  void (*QuadToPoint)(void* ctx, float cpx, float cpy, float x,
                      float y);  // Function to handle 'Q', 'q', 't', 'T'
  void (*EllipticToPoint)(void* ctx, float cpx, float cpy, float rx, float ry,
                          float angle, bool large, bool sweep, float x,
                          float y);  // Function to handle 'A' and 'a'
  void (*ClosePath)(void* ctx);      // Function to handle 'Z' and 'z'
} LBSPathConsumer;

/**
 Parses the given data and processes it with the provided path consumer.

 This function takes a data string as input and parses it to extract path
 commands. It processes the commands using the provided path consumer, which
 defines function pointers for handling different path operations. The consumer
 can refer to specific functions to perform actions such as moving to a point,
 drawing lines, drawing curves, drawing arcs, and closing the path.

 @param data The data string containing path commands to parse.
 @param consumer The path consumer that handles path operations and performs
 actions on the path.

 @note The path consumer must be properly implemented with the necessary
 function implementations for each operation. It is the responsibility of the
 caller to ensure correct implementation and memory management of the path
 consumer.
 */
void LBSParsePathWithConsumer(const char* data, LBSPathConsumer* consumer);
#ifdef __cplusplus
}
#endif

#endif  // LYNX_BASIC_SHAPE_PATH_CONSUMER_H_
