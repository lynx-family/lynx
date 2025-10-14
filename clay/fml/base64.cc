/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Copyright 2022 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "clay/fml/base64.h"

#include <cstdint>

#include "clay/fml/logging.h"

#define DecodePad -2
#define EncodePad 64

static const char default_encode[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/=";

static const signed char decodeData[] = {
    62, -1, -1,        -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1,
    -1, -1, DecodePad, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12,        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    -1, -1, -1,        -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38,        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

#if defined \
    _WIN32  // disable 'two', etc. may be used without having been initialized
#pragma warning(push)
#pragma warning(disable : 4701)
#endif

namespace fml {

Base64::Error Base64::Decode(const void* srcv, size_t srcLength, void* dstv,
                             size_t* dstLength) {
  const unsigned char* src = static_cast<const unsigned char*>(srcv);
  unsigned char* dst = static_cast<unsigned char*>(dstv);

  int i = 0;
  bool padTwo = false;
  bool padThree = false;
  char unsigned const* const end = src + srcLength;
  while (src < end) {
    unsigned char bytes[4];
    int byte = 0;
    do {
      unsigned char srcByte = *src++;
      if (srcByte == 0) goto goHome;
      if (srcByte <= ' ') continue;  // treat as white space
      if (srcByte < '+' || srcByte > 'z') return kBadCharError;
      signed char decoded = decodeData[srcByte - '+'];
      bytes[byte] = decoded;
      if (decoded < 0) {
        if (decoded == DecodePad) goto handlePad;
        return kBadCharError;
      } else
        byte++;
      if (*src) continue;
      if (byte == 0) goto goHome;
      if (byte == 4) break;
    handlePad:
      if (byte < 2) return kPadError;
      padThree = true;
      if (byte == 2) padTwo = true;
      break;
    } while (byte < 4);
    int two = 0;
    int three = 0;
    if (dst) {
      int one = (uint8_t)(bytes[0] << 2);
      two = bytes[1];
      one |= two >> 4;
      two = (uint8_t)((two << 4) & 0xFF);
      three = bytes[2];
      two |= three >> 2;
      three = (uint8_t)((three << 6) & 0xFF);
      three |= bytes[3];
      FML_CHECK(one < 256 && two < 256 && three < 256);
      dst[i] = (unsigned char)one;
    }
    i++;
    if (padTwo) break;
    if (dst) dst[i] = (unsigned char)two;
    i++;
    if (padThree) break;
    if (dst) dst[i] = (unsigned char)three;
    i++;
  }
goHome:
  *dstLength = i;
  return kNoError;
}

#if defined _WIN32
#pragma warning(pop)
#endif

size_t Base64::Encode(const void* srcv, size_t length, void* dstv,
                      const char* encodeMap) {
  const unsigned char* src = static_cast<const unsigned char*>(srcv);
  unsigned char* dst = static_cast<unsigned char*>(dstv);

  const char* encode;
  if (nullptr == encodeMap) {
    encode = default_encode;
  } else {
    encode = encodeMap;
  }
  if (dst) {
    size_t remainder = length % 3;
    char unsigned const* const end = &src[length - remainder];
    while (src < end) {
      unsigned a = *src++;
      unsigned b = *src++;
      unsigned c = *src++;
      int d = c & 0x3F;
      c = (c >> 6 | b << 2) & 0x3F;
      b = (b >> 4 | a << 4) & 0x3F;
      a = a >> 2;
      *dst++ = encode[a];
      *dst++ = encode[b];
      *dst++ = encode[c];
      *dst++ = encode[d];
    }
    if (remainder > 0) {
      int k1 = 0;
      int k2 = EncodePad;
      int a = (uint8_t)*src++;
      if (remainder == 2) {
        int b = *src++;
        k1 = b >> 4;
        k2 = (b << 2) & 0x3F;
      }
      *dst++ = encode[a >> 2];
      *dst++ = encode[(k1 | a << 4) & 0x3F];
      *dst++ = encode[k2];
      *dst++ = encode[EncodePad];
    }
  }
  return (length + 2) / 3 * 4;
}
}  // namespace fml
