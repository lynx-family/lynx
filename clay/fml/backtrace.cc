// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/backtrace.h"

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

#include <__config>
#include <csignal>
#include <sstream>

#if OS_WIN
#include <crtdbg.h>
#include <debugapi.h>
#endif

#include <cstdlib>

#include "clay/fml/logging.h"

namespace fml {

static const char* kUnknownFrameName = "Unknown";

#if !OS_IOS
static std::string DemangleSymbolName(const std::string& mangled) {
  if (std::strcmp(mangled.c_str(), kUnknownFrameName) == 0) {
    return kUnknownFrameName;
  }

  int status = 0;
  size_t length = 0;
  char* demangled = __cxxabiv1::__cxa_demangle(
      mangled.data(),  // mangled name
      nullptr,         // output buffer (malloc-ed if nullptr)
      &length,         // demangled length
      &status);

  if (demangled == nullptr || status != 0) {
    return mangled;
  }

  auto demangled_string = std::string{demangled, length};
  free(demangled);
  return demangled_string;
}
#endif

static std::string GetSymbolName(void* symbol) {
#if !OS_IOS
  Dl_info info = {};

  if (::dladdr(symbol, &info) == 0) {
    return kUnknownFrameName;
  }
  if (info.dli_sname == nullptr) {
    return kUnknownFrameName;
  }

  return DemangleSymbolName({info.dli_sname});
#endif
  return kUnknownFrameName;
}

static int Backtrace(void** symbols, int size) {
#if OS_WIN
  return CaptureStackBackTrace(0, size, symbols, NULL);
#else
  return ::backtrace(symbols, size);
#endif  // OS_WIN
}

std::string BacktraceHere(size_t offset) {
  constexpr size_t kMaxFrames = 256;
  void* symbols[kMaxFrames];
  const auto available_frames = Backtrace(symbols, kMaxFrames);
  if (available_frames <= 0) {
    return "";
  }

  // Exclude here.
  offset += 2;

  std::stringstream stream;
  for (int i = offset; i < available_frames; ++i) {
    stream << "Frame " << i - offset << ": " << symbols[i] << " "
           << GetSymbolName(symbols[i]) << std::endl;
  }
  return stream.str();
}

static size_t kKnownSignalHandlers[] = {
    SIGABRT,  // abort program
    SIGFPE,   // floating-point exception
    SIGTERM,  // software termination signal
    SIGSEGV,  // segmentation violation
#if !OS_WIN
    SIGBUS,   // bus error
    SIGSYS,   // non-existent system call invoked
    SIGPIPE,  // write on a pipe with no reader
    SIGALRM,  // real-time timer expired
#endif        // !OS_WIN
};

static std::string SignalNameToString(int signal) {
  switch (signal) {
    case SIGABRT:
      return "SIGABRT";
    case SIGFPE:
      return "SIGFPE";
    case SIGSEGV:
      return "SIGSEGV";
    case SIGTERM:
      return "SIGTERM";
#if !OS_WIN
    case SIGBUS:
      return "SIGBUS";
    case SIGSYS:
      return "SIGSYS";
    case SIGPIPE:
      return "SIGPIPE";
    case SIGALRM:
      return "SIGALRM";
#endif  // !OS_WIN
  }
  return std::to_string(signal);
}

static void ToggleSignalHandlers(bool set);

static void SignalHandler(int signal) {
  // We are a crash signal handler. This can only happen once. Since we don't
  // want to catch crashes while we are generating the crash reports, disable
  // all set signal handlers to their default values before reporting the crash
  // and re-raising the signal.
  ToggleSignalHandlers(false);

  FML_LOG(ERROR) << "Caught signal " << SignalNameToString(signal)
                 << " during program execution." << std::endl
                 << BacktraceHere();

  ::raise(signal);
}

static void ToggleSignalHandlers(bool set) {
  for (size_t i = 0; i < sizeof(kKnownSignalHandlers) / sizeof(size_t); ++i) {
    auto signal_name = kKnownSignalHandlers[i];
    auto handler = set ? &SignalHandler : SIG_DFL;

    if (::signal(signal_name, handler) == SIG_ERR) {
      FML_LOG(ERROR) << "Could not attach signal handler for " << signal_name;
    }
  }
}

void InstallCrashHandler() {
#if OS_WIN
  if (!IsDebuggerPresent()) {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
  }
#endif
  ToggleSignalHandlers(true);
}

bool IsCrashHandlingSupported() { return true; }

}  // namespace fml
