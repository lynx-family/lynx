// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LBSPathParser.h"

#import <ctype.h>
#import <stdbool.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>

static float LBSNextNumber(char** s, bool* error) {
  char* start = *s;
  uint32_t len = 0;
  // sign
  if (**s == '-' || **s == '+') {
    len++;
    (*s)++;
  }

  // integer
  while (**s && isdigit(**s)) {
    (*s)++;
    len++;
  }

  // decimal
  if (**s == '.') {
    len++;
    (*s)++;
    while (**s && isdigit(**s)) {
      (*s)++;
      len++;
    }
  }

  // exponent
  if ((**s == 'e' || **s == 'E') && (isdigit(*s[1]) || *s[1] == '+' || *s[1] == '-')) {
    (*s)++;
    len++;
    if (**s == '-' || **s == '+') {
      (*s)++;
      len++;
    }
  }
  while (**s && isdigit(**s)) {
    (*s)++;
    len++;
  }
  char number[len + 1];
  strncpy(number, start, len);
  number[len] = 0;
  char* err;
  float res = strtof(number, &err);
  *error = *err != '\0' || len == 0;
  return res;
}

static void LBSSkipSep(char** s) {
  if (!*s) {
    return;
  }
  while (**s && (isspace(**s) || **s == ',')) {
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

void LBSParsePathWithConsumer(const char* data, LBSPathConsumer* consumer) {
  char* s = (char*)data;
  float args[6] = {0};
  LBSSkipSep(&s);
  if (s) {
    char cmd = 0, lastCmd = 0;
    float currentPointX = 0, currentPointY = 0, controlPointX = 0, controlPointY = 0;
    bool error = false;
    while (*s && !error) {
      if (isalpha(*s)) {
        cmd = lastCmd = *s++;
      } else {
        cmd = lastCmd;
      }
      switch (cmd) {
        case 'm':
        case 'M': {
          error = LBSExtractPathArgs(&s, args, 2);
          if (cmd == 'm') {
            args[0] += currentPointX;
            args[1] += currentPointY;
            lastCmd = 'l';
          } else {
            lastCmd = 'L';
          }
          consumer->MoveToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = args[0];
          currentPointY = args[1];
          break;
        }
        case 'l':
        case 'L':
          error = LBSExtractPathArgs(&s, args, 2);
          if (cmd == 'l') {
            args[0] += currentPointX;
            args[1] += currentPointY;
          }
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = args[0];
          currentPointY = args[1];
          lastCmd = cmd;
          break;
        case 'h':
        case 'H':
          error = LBSExtractPathArgs(&s, args, 1);
          if (cmd == 'h') {
            args[0] += currentPointX;
          }
          args[1] = currentPointY;
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointX = args[0];
          lastCmd = cmd;
          break;
        case 'v':
        case 'V':
          error = LBSExtractPathArgs(&s, args + 1, 1);
          if (cmd == 'v') {
            args[1] += currentPointY;
          }
          args[0] = currentPointX;
          consumer->LineToPoint(consumer->ctx, args[0], args[1]);
          currentPointY = args[1];
          lastCmd = cmd;
          break;
        case 'c':
        case 'C':
          error = LBSExtractPathArgs(&s, args, 6);
          if (cmd == 'c') {
            for (int i = 0; i < 3; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
          }
          consumer->CubicToPoint(consumer->ctx, args[0], args[1], args[2], args[3], args[4],
                                 args[5]);
          currentPointX = args[4];
          currentPointY = args[5];
          controlPointX = args[2];
          controlPointY = args[3];
          lastCmd = cmd;
          break;
        case 's':
        case 'S':
          error = LBSExtractPathArgs(&s, args + 2, 4);
          if ('s' == cmd) {
            for (int i = 1; i < 3; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
          }

          // If there is no previous command or if the previous command was not
          // an C, c, S or s, assume the first control point is coincident with
          // the current point.
          if (strchr("cCsS", lastCmd)) {
            args[0] = 2 * currentPointX - controlPointX;
            args[1] = 2 * currentPointY - controlPointY;
          } else {
            args[0] = currentPointX;
            args[1] = currentPointY;
          }
          consumer->CubicToPoint(consumer->ctx, args[0], args[1], args[2], args[3], args[4],
                                 args[5]);
          controlPointX = args[2];
          controlPointY = args[3];
          currentPointX = args[4];
          currentPointY = args[5];
          lastCmd = cmd;
          break;
        case 'q':
        case 'Q':
          error = LBSExtractPathArgs(&s, args, 4);
          if (cmd == 'q') {
            for (int i = 0; i < 2; i++) {
              args[2 * i] += currentPointX;
              args[2 * i + 1] += currentPointY;
            }
          }
          controlPointX = args[0];
          controlPointY = args[1];
          consumer->QuadToPoint(consumer->ctx, args[0], args[1], args[2], args[3]);
          currentPointX = args[2];
          currentPointY = args[3];
          lastCmd = cmd;
          break;
        case 't':
        case 'T':
          error = LBSExtractPathArgs(&s, args + 2, 2);
          if (cmd == 't') {
            args[2] += currentPointX;
            args[3] += currentPointY;
          }

          // The control point is assumed to be the reflection of the control
          // point on the previous command relative to the current point. (If
          // there is no previous command or if the previous command was not a
          // Q, q, T or t, assume the control point is coincident with the
          // current point.)
          if (strchr("tTqQ", lastCmd)) {
            controlPointX = 2 * currentPointX - controlPointX;
            controlPointY = 2 * currentPointY - controlPointY;
          } else {
            controlPointX = currentPointX;
            controlPointY = currentPointY;
          }

          args[0] = controlPointX;
          args[1] = controlPointY;
          consumer->QuadToPoint(consumer->ctx, args[0], args[1], args[2], args[3]);
          currentPointX = args[2];
          currentPointY = args[3];
          lastCmd = cmd;
          break;
        case 'a':
        case 'A':
          error = LBSExtractPathArgs(&s, args, 3);
          LBSSkipSep(&s);
          bool largeArcFlag = *s++ == '1' ? 1.f : 0.f;
          LBSSkipSep(&s);
          bool sweepArcFlag = *s++ == '1' ? 1.f : 0.f;
          error |= LBSExtractPathArgs(&s, args + 3, 2);
          if (cmd == 'a') {
            args[3] += currentPointX;
            args[4] += currentPointY;
          }
          consumer->EllipticToPoint(consumer->ctx, currentPointX, currentPointY, args[0], args[1],
                                    args[2], largeArcFlag, sweepArcFlag, args[3], args[4]);
          controlPointX = currentPointX = args[3];
          controlPointY = currentPointY = args[4];
          lastCmd = cmd;
          break;
        case 'z':
        case 'Z':
          consumer->ClosePath(consumer->ctx);
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
}
