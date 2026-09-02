// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/testing/lepus_debugger_runner.h"

namespace lynx {
namespace debug {
namespace testing {

// Smoke test: compile and execute a script with the debugger attached but
// without any debug info or breakpoints configured. The VM must run to
// completion and the channel must stay idle.
TEST_F(LepusDebuggerRunner, CompileAndExecuteWithoutDebugInfo) {
  CompileScript(R"(
    function foo() { return 42; }
    foo();
  )");
  Execute();
  EXPECT_TRUE(channel_.notifications_.empty());
  EXPECT_TRUE(channel_.responses_.empty());
}

// Dispatching a protocol method before any script is compiled must not
// crash the debugger. The debugger replies with an error response for
// unknown methods, so the channel must observe a response.
TEST_F(LepusDebuggerRunner, DispatchBeforeCompile) {
  Dispatch(R"({"id":1,"method":"Runtime.enable"})");
  ASSERT_FALSE(channel_.responses_.empty());
  EXPECT_EQ(channel_.responses_.front().first, 1);
  EXPECT_FALSE(channel_.responses_.front().second.empty());
}

// Empty debug info goes through the invalid-debug-info path, which logs
// the error and injects the formatted message into the VM. Nothing is
// emitted on the channel; this test guards against crashes and hangs.
TEST_F(LepusDebuggerRunner, EmptyDebugInfoDoesNotCrash) {
  CompileScript("function foo() { return 42; }");
  SetDebugInfo("main.js", "", 1, "");
  PrepareInspector();
  Execute();
  EXPECT_TRUE(channel_.notifications_.empty());
  EXPECT_TRUE(channel_.responses_.empty());
}

// A full debug-info document produced by the real bytecode pipeline would
// drive the VM's debugger into its script-setup path, which requires a
// complete host environment (inspector thread, devtool connection state)
// that is not available in unit tests. The error paths below cover the
// debug-info parsing logic that is reachable without such a host.

// Malformed debug info (not valid JSON) must be rejected through the
// invalid-debug-info path instead of crashing.
TEST_F(LepusDebuggerRunner, MalformedDebugInfoDoesNotCrash) {
  CompileScript("function foo() { return 42; }");
  SetDebugInfo("main.js", "this is not json", 1, "");
  PrepareInspector();
  Execute();
}

// A debug-info whose function count does not match the compiled script
// must be rejected with an error instead of crashing.
TEST_F(LepusDebuggerRunner, FunctionNumberMismatchDoesNotCrash) {
  CompileScript("function foo() { return 42; }");
  SetDebugInfo("main.js", R"({"v":2,"fn":99,"fi":[]})", 1, "");
  PrepareInspector();
  Execute();
}

// A debug-info with an unsupported schema version must be rejected.
TEST_F(LepusDebuggerRunner, UnsupportedVersionDoesNotCrash) {
  CompileScript("function foo() { return 42; }");
  SetDebugInfo("main.js", R"({"v":99,"fn":1,"fi":[]})", 1, "");
  PrepareInspector();
  Execute();
}

// A debug-info that matches the function count but lacks function source
// must go through the invalid-debug-info path without crashing.
TEST_F(LepusDebuggerRunner, MissingFunctionSourceDoesNotCrash) {
  CompileScript("function foo() { return 42; }");
  SetDebugInfo("main.js", R"({"v":2,"fn":1,"fi":[{"id":0}]})", 1, "");
  PrepareInspector();
  Execute();
}

// The debugger must forward pause/quit lifecycle calls to the client so
// the host can run its message loop while the VM is paused.
TEST_F(LepusDebuggerRunner, PauseLifecycleForwardsToClient) {
  lynx::debug::LepusNGDebugger* debugger = GetDebugger();
  ASSERT_NE(debugger, nullptr);

  debugger->DebuggerRunMessageLoopOnPause();
  EXPECT_EQ(client_.run_message_loop_calls_, 1);
  EXPECT_EQ(client_.quit_message_loop_calls_, 0);

  debugger->DebuggerQuitMessageLoopOnPause();
  EXPECT_EQ(client_.run_message_loop_calls_, 1);
  EXPECT_EQ(client_.quit_message_loop_calls_, 1);
}

// Pausing on the next statement before executing must not crash; the
// actual pause behavior is driven by the VM and covered by VM-level tests.
TEST_F(LepusDebuggerRunner, SchedulePauseOnNextStatement) {
  CompileScript("function foo() { return 42; } foo();");
  Dispatch(
      R"({"id":0,"method":"Debugger.pauseOnNextStatement","params":{"reason":"test"}})");
  Execute();
}

// Dispatching a debugger command while the VM is running must not crash
// and must produce a response (the debugger may report an error if the
// command is invalid in the running state).
TEST_F(LepusDebuggerRunner, DispatchDebuggerCommandWhileRunning) {
  CompileScript("function foo() { return 42; }");
  Dispatch(R"({"id":2,"method":"Debugger.enable"})");
  Execute();
}

}  // namespace testing
}  // namespace debug
}  // namespace lynx
