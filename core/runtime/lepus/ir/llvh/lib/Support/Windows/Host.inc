//===- llvm/Support/Win32/Host.inc ------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Win32 Host support.
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <string>

#include "core/runtime/lepus/ir/llvh/lib/Support/Windows/WindowsSupport.h"

using llvh;

static std::string updateTripleOSVersion(std::string Triple) {
  return Triple;
}

std::string sys::getDefaultTargetTriple() {
  const char *Triple = LLVH_DEFAULT_TARGET_TRIPLE;

  // Override the default target with an environment variable named by LLVM_TARGET_TRIPLE_ENV.
#if defined(LLVM_TARGET_TRIPLE_ENV)
  if (const char *EnvTriple = std::getenv(LLVM_TARGET_TRIPLE_ENV))
    Triple = EnvTriple;
#endif

  return Triple;
}
