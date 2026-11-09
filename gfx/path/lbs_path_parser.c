// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gfx/public/path_parser.h"

static bool LBSIsDigit(char c) { return c >= '0' && c <= '9'; }
static bool LBSReadNumber(const char** s, float* result) {
  const char* begin = *s;
  const char* end = begin;
  if (*end == '+' || *end == '-') ++end;
  bool digits = false;
  while (LBSIsDigit(*end)) {
    digits = true;
    ++end;
  }
  if (*end == '.') {
    ++end;
    while (LBSIsDigit(*end)) {
      digits = true;
      ++end;
    }
  }
  if (!digits) return false;
  if (*end == 'e' || *end == 'E') {
    ++end;
    if (*end == '+' || *end == '-') ++end;
    if (!LBSIsDigit(*end)) return false;
    while (LBSIsDigit(*end)) ++end;
  }
  char* parsed_end = NULL;
  *result = strtof(begin, &parsed_end);
  if (parsed_end != end || !isfinite(*result)) return false;
  *s = end;
  return true;
}

static float LBSNextNumber(char** s, bool* error) {
  const char* cursor = *s;
  float value = 0;
  if (!LBSReadNumber(&cursor, &value)) *error = true;
  *s = (char*)cursor;
  return value;
}

static void LBSSkipSep(char** s) {
  if (!*s) {
    return;
  }
  while (**s && (isspace((unsigned char)**s) || **s == ',')) {
    (*s)++;
  }
}

static bool LBSExtractPathArgs(char** s, float args[], int nArgs) {
  bool error = false;
  for (int i = 0; i < nArgs && !error; i++) {
    LBSSkipSep(s);
    args[i] = LBSNextNumber(s, &error);
  }
  return error;
}

static bool LBSArgsAreFinite(const float args[], int count) {
  for (int i = 0; i < count; ++i) {
    if (!isfinite(args[i])) return false;
  }
  return true;
}

void LBSParsePathWithConsumer(const char* data, LBSPathConsumer* consumer) {
  if (!consumer) return;
  consumer->error = 1;
  if (!data) return;
  consumer->error = 0;
  char* s = (char*)data;
  float args[6] = {0};
  LBSSkipSep(&s);
  if (s) {
    char cmd = 0, lastCmd = 0;
    float currentPointX = 0, currentPointY = 0, controlPointX = 0,
          controlPointY = 0, subpathStartX = 0, subpathStartY = 0;
    bool error = false;
    while (*s && !error) {
      char previousCmd = lastCmd;
      if (isalpha((unsigned char)*s)) {
        cmd = lastCmd = *s++;
      } else {
        cmd = lastCmd;
      }
      switch (cmd) {
        case 'm':
        case 'M': {
          if (LBSExtractPathArgs(&s, args, 2)) goto invalid;
          if (cmd == 'm') {
            args[0] += currentPointX;
            args[1] += currentPointY;
            if (!LBSArgsAreFinite(args, 2)) goto invalid;
            lastCmd = 'l';
          } else {
            lastCmd = 'L';
          }
          consumer->MoveToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = subpathStartX = args[0];
          currentPointY = subpathStartY = args[1];
          break;
        }
        case 'l':
        case 'L':
          if (LBSExtractPathArgs(&s, args, 2)) goto invalid;
          if (cmd == 'l') {
            args[0] += currentPointX;
            args[1] += currentPointY;
            if (!LBSArgsAreFinite(args, 2)) goto invalid;
          }
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = args[0];
          currentPointY = args[1];
          lastCmd = cmd;
          break;
        case 'h':
        case 'H':
          if (LBSExtractPathArgs(&s, args, 1)) goto invalid;
          if (cmd == 'h') {
            args[0] += currentPointX;
            if (!isfinite(args[0])) goto invalid;
          }
          args[1] = currentPointY;
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = args[0];
          lastCmd = cmd;
          break;
        case 'v':
        case 'V':
          if (LBSExtractPathArgs(&s, args + 1, 1)) goto invalid;
          if (cmd == 'v') {
            args[1] += currentPointY;
            if (!isfinite(args[1])) goto invalid;
          }
          args[0] = currentPointX;
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointY = args[1];
          lastCmd = cmd;
          break;
        case 'c':
        case 'C':
          if (LBSExtractPathArgs(&s, args, 6)) goto invalid;
          if (cmd == 'c') {
            for (int i = 0; i < 3; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
            if (!LBSArgsAreFinite(args, 6)) goto invalid;
          }
          consumer->CubicToPoint(consumer->ctx, args[0], args[1], args[2],
                                 args[3], args[4], args[5]);
          currentPointX = args[4];
          currentPointY = args[5];
          controlPointX = args[2];
          controlPointY = args[3];
          lastCmd = cmd;
          break;
        case 's':
        case 'S':
          if (LBSExtractPathArgs(&s, args + 2, 4)) goto invalid;
          if ('s' == cmd) {
            for (int i = 1; i < 3; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
          }

          // If there is no previous command or if the previous command was not
          // an C, c, S or s, assume the first control point is coincident with
          // the current point.
          if (previousCmd == 'C' || previousCmd == 'c' || previousCmd == 'S' ||
              previousCmd == 's') {
            args[0] = currentPointX + (currentPointX - controlPointX);
            args[1] = currentPointY + (currentPointY - controlPointY);
          } else {
            args[0] = currentPointX;
            args[1] = currentPointY;
          }
          if (!LBSArgsAreFinite(args, 6)) goto invalid;
          consumer->CubicToPoint(consumer->ctx, args[0], args[1], args[2],
                                 args[3], args[4], args[5]);
          controlPointX = args[2];
          controlPointY = args[3];
          currentPointX = args[4];
          currentPointY = args[5];
          lastCmd = cmd;
          break;
        case 'q':
        case 'Q':
          if (LBSExtractPathArgs(&s, args, 4)) goto invalid;
          if (cmd == 'q') {
            for (int i = 0; i < 2; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
            if (!LBSArgsAreFinite(args, 4)) goto invalid;
          }
          controlPointX = args[0];
          controlPointY = args[1];
          consumer->QuadToPoint(consumer->ctx, args[0], args[1], args[2],
                                args[3]);
          currentPointX = args[2];
          currentPointY = args[3];
          lastCmd = cmd;
          break;
        case 't':
        case 'T':
          if (LBSExtractPathArgs(&s, args + 2, 2)) goto invalid;
          if (cmd == 't') {
            args[2] += currentPointX;
            args[3] += currentPointY;
          }

          // The control point is assumed to be the reflection of the control
          // point on the previous command relative to the current point. (If
          // there is no previous command or if the previous command was not a
          // Q, q, T or t, assume the control point is coincident with the
          // current point.)
          if (previousCmd == 'Q' || previousCmd == 'q' || previousCmd == 'T' ||
              previousCmd == 't') {
            controlPointX = currentPointX + (currentPointX - controlPointX);
            controlPointY = currentPointY + (currentPointY - controlPointY);
          } else {
            controlPointX = currentPointX;
            controlPointY = currentPointY;
          }

          args[0] = controlPointX;
          args[1] = controlPointY;
          if (!LBSArgsAreFinite(args, 4)) goto invalid;
          consumer->QuadToPoint(consumer->ctx, args[0], args[1], args[2],
                                args[3]);
          currentPointX = args[2];
          currentPointY = args[3];
          lastCmd = cmd;
          break;
        case 'a':
        case 'A':
          if (LBSExtractPathArgs(&s, args, 3)) goto invalid;
          LBSSkipSep(&s);
          if (!*s) goto invalid;
          bool largeArcFlag = *s++ == '1';
          LBSSkipSep(&s);
          if (!*s) goto invalid;
          bool sweepArcFlag = *s++ == '1';
          if (LBSExtractPathArgs(&s, args + 3, 2)) goto invalid;
          if (cmd == 'a') {
            args[3] += currentPointX;
            args[4] += currentPointY;
            if (!LBSArgsAreFinite(args + 3, 2)) goto invalid;
          }
          consumer->EllipticToPoint(consumer->ctx, currentPointX, currentPointY,
                                    args[0], args[1], args[2], largeArcFlag,
                                    sweepArcFlag, args[3], args[4]);
          controlPointX = currentPointX = args[3];
          controlPointY = currentPointY = args[4];
          lastCmd = cmd;
          break;
        case 'z':
        case 'Z':
          consumer->ClosePath(consumer->ctx);
          currentPointX = subpathStartX;
          currentPointY = subpathStartY;
          lastCmd = 0;
          break;
        default:
          // Unknown command
          error = true;
          break;
      }
      // next command
      LBSSkipSep(&s);
    }
    consumer->error = error;
  }
  return;
invalid:
  consumer->error = 1;
}
