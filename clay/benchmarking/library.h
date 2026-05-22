// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_BENCHMARKING_LIBRARY_H_
#define CLAY_BENCHMARKING_LIBRARY_H_

extern "C" {
__attribute__((visibility("default"))) int RunBenchmarks(int argc, char** argv);
}

#endif  // CLAY_BENCHMARKING_LIBRARY_H_
