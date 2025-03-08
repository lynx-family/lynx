// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/js_error_reporter.h"

#include <optional>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/string/string_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace common {
namespace testing {
namespace {

const std::string kLepusNgRuntimeOriginErr =
    "\"lepusng exception: TypeError: not a function backtrace:\n    at "
    "<anonymous> (file://lepus.js:24:40)\n    at <anonymous> "
    "(file://lepus.js:26:334)\n    at render (file://lepus.js:27:118)\n    at "
    "entryPoint (file://lepus.js:28:47)\n    at $renderPage0 "
    "(file://lepus.js:29:184)\n\ntemplate_debug_url:http:/foo.bar:8787/dist/"
    "00-counter-2/intermediate/debug-info.json\"";

const int kLynxErrorCodeJavascript = 201;
const JSErrorInfo kJSCTypeError1{
    .name = "TypeError",
    .stack = R"(not_a_func@file:///app-service.js:431:10
    @file:///lynx_core.js:1:212072
    @file:///lynx_core.js:1:210891)",
    .message =
        "publishEvent not_a_func error: obj is not a function. (In 'obj()', "
        "'obj' is an instance of Object)"};

const JSErrorInfo kQuickjsTypeError1{
    .name = "TypeError",
    .stack = R"(TypeError: obj is not a function
at <anonymous> (file:///app-service.js:444:16)
at wrapError (file:///app-service.js:413:5)
at _Page.not_a_func (file:///app-service.js:443:7)
at App.handlerEvent (file:///lynx_core.js:1:142231)
at App.publishEvent (file:///lynx_core.js:1:141043))",
    .message = "publishEvent not_a_func error: obj is not a function"};

const JSErrorInfo kV8TypeError1{
    .name = "TypeError",
    .stack = R"(TypeError: obj is not a function
    at file:///app-service.js:445
    at wrapError (file:///app-service.js:414)
    at _Page.not_a_func (file:///app-service.js:444)
    at App.handlerEvent (file:///lynx_core.js:1133)
    at App.publishEvent (file:///lynx_core.js:1054))",
    .message = "publishEvent not_a_func error: obj is not a function"};

const JSErrorInfo kJSCTypeError2{
    .name = "TypeError",
    .stack = R"(@file:///app-service.js:452:21
  wrapError@file:///app-service.js:413:6
  undefNotAObj@file:///app-service.js:451:16
  @file:///lynx_core.js:1:212072
  @file:///lynx_core.js:1:210891)",
    .message =
        "publishEvent undefNotAObj error: undefined is not an object "
        "(evaluating 'undef.ccc')",
};

const JSErrorInfo kQuickJSTypeError2{
    .name = "TypeError",
    .stack = R"(TypeError: Cannot read property 'ccc' of undefined
at <anonymous> (file:///app-service.js:452:22)
at wrapError (file:///app-service.js:413:5)
at _Page.undefNotAObj (file:///app-service.js:451:7)
at App.handlerEvent (file:///lynx_core.js:1:142231)
at App.publishEvent (file:///lynx_core.js:1:141043))",
    .message =
        "publishEvent undefNotAObj error: Cannot read property 'ccc' of "
        "undefined",
};

const JSErrorInfo kJSCReferenceError1 = {
    .name = "ReferenceError",
    .stack = R"(@file:///app-service.js:459:19
wrapError@file:///app-service.js:413:6
refError@file:///app-service.js:458:16
@file:///lynx_core.js:1:212072
@file:///lynx_core.js:1:210891)",
    .message = "publishEvent refError error: Can't find variable: foo",
};

const JSErrorInfo kQuickJSReferenceError1 = {
    .name = "ReferenceError",
    .stack = R"(ReferenceError: foo is not defined
at <anonymous> (file:///app-service.js:459:9)
at wrapError (file:///app-service.js:413:5)
at _Page.refError (file:///app-service.js:458:7)
at App.handlerEvent (file:///lynx_core.js:1:142231)
at App.publishEvent (file:///lynx_core.js:1:141043))",
    .message = "publishEvent refError error: foo is not defined",
};

const JSErrorInfo kV8ReferenceError1 = {
    .name = "ReferenceError",
    .stack = R"(ReferenceError: foo is not defined
    at app-service.js:460
    at wrapError (file:///app-service.js:414)
    at _Page.refError (file:///app-service.js:459)
    at App.handlerEvent (file:///lynx_core.js:1133)
    at App.publishEvent (/lynx_core.js:1054))",
    .message = "publishEvent refError error: foo is not defined",
};

const JSErrorInfo kJSCErrorWithNative = {
    .name = "TypeError",
    .stack = R"(@file:///app-service.js:472:24
forEach@[native code]
inNative@file:///app-service.js:471:24
@file:///lynx_core.js:1:212072
@file:///lynx_core.js:1:210891)",
    .message = "publishEvent inNative error: in forEach",
};

const JSErrorInfo kQuickJSErrorWithNative = {
    .name = "TypeError",
    .stack = R"(Error: in forEach
at <anonymous> (file:///app-service.js:466:15)
at Array.forEach (native)
at _Page.inNative (file:///app-service.js:465:17)
at App.handlerEvent (file:///lynx_core.js:1:142231)
at App.publishEvent (file:///lynx_core.js:1:141043))",
    .message = "publishEvent inNative error: in forEach",
};

const JSErrorInfo kV8ErrorWithNative = {
    .name = "TypeError",
    .stack = R"(Error: in forEach
    at app-service.js:467
    at Array.forEach (<anonymous>)
    at _Page.inNative (file:///app-service.js:466)
    at App.handlerEvent (file:///lynx_core.js:1133)
    at App.publishEvent (file:///lynx_core.js:1054))",
    .message = "publishEvent inNative error: in forEach",
};

const JSErrorInfo kJSCEvalError = {
    .name = "TypeError",
    .stack = R"(eval code@
eval@[native code]
@file:///app-service.js:473:20
wrapError@file:///app-service.js:413:6
withEval@file:///app-service.js:472:16
@file:///lynx_core.js:1:212072
@file:///lynx_core.js:1:210891)",
    .message = "publishEvent withEval error: in eval error",
};

const JSErrorInfo kQuickJSEvalError = {
    .name = "TypeError",
    .stack = R"(Error: in eval error    at <eval> (<input>:1:33)
    at <anonymous> (file:///app-service.js:474:56)
    at wrapError (file:///app-service.js:414:8)
    at withEval (file:///app-service.js:475:9)
    at apply (native)
    at handlerEvent (file:///lynx_core.js:6183:62)
    at publishEvent (file:///lynx_core.js:6116:73))",
    .message = "publishEvent withEval error: in eval error",
};

const JSErrorInfo kV8EvalError = {
    .name = "TypeError",
    .stack = R"(Error: in eval error
    at eval (eval at <anonymous> (file:///app-service.js:474), <anonymous>:1:7)
    at app-service.js:474
    at wrapError (file:///app-service.js:414)
    at _Page.withEval (file:///app-service.js:473)
    at App.handlerEvent (file:///lynx_core.js:1133)
    at App.publishEvent (file:///lynx_core.js:1054))",
    .message = "publishEvent withEval error: in eval error",
};

const JSErrorInfo kQuickJSOnlineError = {
    .name = "TypeError",
    .stack = R"(    at <anonymous> (file:///app-service.js:1:911126)
    at call (native)
    at c (/lynx_core.js:1:6632)
    at <anonymous> (/lynx_core.js:1:6419)
    at <anonymous> (/lynx_core.js:1:7060)
    at B (file:///app-service.js:1:2759)
    at a (file:///app-service.js:1:2978)
    at <anonymous> (file:///app-service.js:1:3025)
    at <anonymous> (/lynx_core.js:1:1935)
    at u (/lynx_core.js:1:2026)
    at a (/lynx_core.js:1:599)
    at <anonymous> (file:///app-service.js:1:3028)
    at n (file:///app-service.js:1:913394)
    at <anonymous> (/lynx_core.js:1:124512)
)",
    .message = "unhandled rejection: not a function",
};

const JSErrorInfo kQuickJSOnlineError2 = {
    .name = "TypeError",
    .stack = R"(    at <anonymous> (file:///app-service.js:1:60526)
    at call (native)
    at d (file:///app-service.js:1:82749)
    at <anonymous> (file:///app-service.js:1:82536)
    at <anonymous> (file:///app-service.js:1:83201)
    at de (file:///app-service.js:1:3027)
    at i (file:///app-service.js:1:3248)
    at <anonymous> (/lynx_core.js:1:987)
    at <anonymous> (/lynx_core.js:1:1023)
)",
    .message = "unhandled rejection: cannot read property 'cc' of undefined",
};

const JSErrorInfo kQuickJSOnlineError3 = {
    .name = "TypeError",
    .stack = R"(    at dispatchException (/app-service.js:1:351931)
   at <anonymous> (/app-service.js:1:348108)
   at <anonymous> (/app-service.js:1:348848)
   at f (/app-service.js:1:1477)
   at a (/app-service.js:1:1696)
   at <anonymous> (/lynx_core.js:1:987)
   at <anonymous> (/lynx_core.js:1:1023)
)",
    .message = "unhandled rejection: cannot read property 'code' of undefined",
};

const JSErrorInfo kV8CommonChunkError = {
    .name = "TypeError",
    .stack =
        R"(JS error from native: Exception calling object as function,Error: fake
     Error: fake
        at Object.f (file://view4/http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:9)
        at _Page.componentDidMount (file://view4/app-service.js:213:34)
        at App.updateComponent (file://shared/lynx_core.js:15504:24)
        at App.didUpdate (file://shared/lynx_core.js:15538:16)
    )",
    .message =
        R"(JS error: JS error from native: Exception calling object as function,Error: fake
     Error: fake
        at Object.f (file://view4/http://fo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:9)
        at _Page.componentDidMount (file://view4/app-service.js:213:34)
        at App.updateComponent (file://shared/lynx_core.js:15504:24)
        at App.didUpdate (file://shared/lynx_core.js:15538:16)
    )",
};

const JSErrorInfo kJSCCommonChunkError = {
    .name = "TypeError",
    .stack =
        R"(JS error from native: [JSC] call js function fail in JSCHelper::call! ,fake f@file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:18
componentDidMount@file:///app-service.js:214:35
@file:///lynx_core.js:1:211362
@file:///lynx_core.js:1:211691)",
    .message =
        R"(JS error: JS error from native: [JSC] call js function fail in JSCHelper::call! ,fake f@file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:18
componentDidMount@file:///app-service.js:214:35
@file:///lynx_core.js:1:211362
@file:///lynx_core.js:1:211691))",
};

const JSErrorInfo kQuickJSCommonChunkError = {
    .name = "TypeError",
    .stack =
        R"(JS error from native: [QuickJS] call js function failed in QuickjsHelper::call!  LYNX error=Error: fk     at f (file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:24)
        at componentDidMount (file:///app-service.js:214:37)
        at <anonymous> (/lynx_core.js:1:141518)
        at <anonymous> (/lynx_core.js:1:141848))",
    .message =
        R"(JS error: JS error from native: [QuickJS] call js function failed in QuickjsHelper::call!  LYNX error=Error: fk     at f (file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:42:24)
        at componentDidMount (file:///app-service.js:214:37)
        at <anonymous> (/lynx_core.js:1:141518)
        at <anonymous> (/lynx_core.js:1:141848)))",
};

const JSErrorInfo kV8SetSourceMapReleaseError{
    .name = "LynxGetSourceMapReleaseError",
    .stack = R"(LynxGetSourceMapReleaseError: d65f0615f0ef96cdd787ec2f80aedbd3
    at file://view4/app-service.js:17:11
    at Object.__init_card_bundle__ [as init] (file://view4/app-service.js:22:3)
    at Object.loadCard (file://shared/lynx_core.js:17115:39))",
    .message = "d65f0615f0ef96cdd787ec2f80aedbd3",
};

const JSErrorInfo kV8SetSourceMapReleaseError2{
    .name = "LynxGetSourceMapReleaseError",
    .stack = R"(LynxGetSourceMapReleaseError: 88704bb03dd3c39d3b24092c838bd049
    at file://view4/http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:14:11
    at initBundle (file://view4/http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:19:3)
    at Lynx.requireModule (file://shared/lynx_core.js:9491:22)
    at commonChunk:./d.ts?commonChunk (file://view4/app-service.js:189:28)
    at __require (file://view4/app-service.js:169:44)
    at ReactApp.<anonymous> (file://view4/app-service.js:206:27)
    at ReactApp.require (file://shared/lynx_core.js:15850:25)
    at Object.__init_card_bundle__ [as init] (file://view4/app-service.js:233:4)
    at Object.loadCard (file://shared/lynx_core.js:17115:39))",
    .message = "88704bb03dd3c39d3b24092c838bd049",
};

const JSErrorInfo kJSCSetSourceMapReleaseError{
    .name = "LynxGetSourceMapReleaseError",
    .stack = R"(@file:///app-service.js:17:20
__init_card_bundle__@file:///app-service.js:22:3
loadCard@file:///lynx_core.js:1:242440)",
    .message = "d65f0615f0ef96cdd787ec2f80aedbd3",
};

const JSErrorInfo kJSCSetSourceMapReleaseError2{
    .name = "LynxGetSourceMapReleaseError",
    .stack =
        R"(@file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:14:20
initBundle@file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:19:3
commonChunk:./d.ts?commonChunk@file:///app-service.js:189:41
__require@file:///app-service.js:169:44
@file:///app-service.js:206:36
@[native code]
@file:///lynx_core.js:1:218122
__init_card_bundle__@file:///app-service.js:234:11
loadCard@file:///lynx_core.js:1:242440)",
    .message = "88704bb03dd3c39d3b24092c838bd049",
};

const JSErrorInfo kQuickJSSetSourceMapReleaseError{
    .name = "LynxGetSourceMapReleaseError",
    .stack = R"(    at <anonymous> (file:///app-service.js:17:56)
        at __init_card_bundle__ (file:///app-service.js:22:5)
        at loadCard (/lynx_core.js:1:172603))",
    .message = "d65f0615f0ef96cdd787ec2f80aedbd3",
};

const JSErrorInfo kQuickJSSetSourceMapReleaseError2{
    .name = "LynxGetSourceMapReleaseError",
    .stack =
        R"(    at <anonymous> (file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:14:56)
        at initBundle (file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js:19:5)
        at <anonymous> (/lynx_core.js:1:45863)
        at commonChunk:./d.ts?commonChunk (file:///app-service.js:189:101)
        at __require (file:///app-service.js:169:82)
        at <anonymous> (file:///app-service.js:206:38)
        at <anonymous> (/lynx_core.js:1:148539)
        at __init_card_bundle__ (file:///app-service.js:234:29)
        at loadCard (/lynx_core.js:1:172603))",
    .message = "88704bb03dd3c39d3b24092c838bd049",
};
}  // namespace

class TestJSErrorReporter {
 public:
  ErrorEvent FormatError(JSErrorInfo& error_info) {
    return reporter_.FormatError(error_info);
  }
  StackTrace ComputeStackTrace(const JSErrorInfo& error,
                               bool find_file_name_only) {
    return reporter_.ComputeStackTrace(error, find_file_name_only);
  }
  std::optional<StackFrame> ParseChromiumBasedStack(const std::string& line) {
    return reporter_.ParseChromiumBasedStack(line);
  }
  std::optional<StackFrame> ParseDarwinStack(const std::string& line) {
    return reporter_.ParseDarwinStack(line);
  }
  std::string GetFileNameFromStack(const std::string& line) {
    return reporter_.GetFileNameFromStack(line);
  }
  base::LynxError ReportException(const std::string& msg,
                                  const std::string& stack, int32_t error_code,
                                  base::LynxErrorLevel error_level,
                                  const std::string& filename) {
    return reporter_.ReportException(msg, stack, error_code, error_level,
                                     filename);
  }

 private:
  JSErrorReporter reporter_;
};

TEST(JSErrorReporterTest, ParseChromiumBasedStackTest) {
  TestJSErrorReporter test_error_reporter;
  std::string normal_line = "at myFunction (file:///path/to/file.js:123:45)";
  auto opt_stack_frame =
      test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "myFunction");
  EXPECT_EQ(opt_stack_frame->filename, "file:///path/to/file.js");
  EXPECT_EQ(opt_stack_frame->lineno, 123);
  EXPECT_EQ(opt_stack_frame->colno, 45);

  normal_line = "";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_FALSE(opt_stack_frame.has_value());

  normal_line = "at <input>0:0";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());

  normal_line = "at JSON.parse (<anonymous>)";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "JSON.parse");
  EXPECT_EQ(opt_stack_frame->filename, "<anonymous>");
  EXPECT_EQ(opt_stack_frame->lineno, 0);
  EXPECT_EQ(opt_stack_frame->colno, 0);

  normal_line = "at parse (native)";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "parse");
  EXPECT_EQ(opt_stack_frame->filename, "native");
  EXPECT_EQ(opt_stack_frame->lineno, 0);
  EXPECT_EQ(opt_stack_frame->colno, 0);
  // EXPECT_EQ(opt_stack_frame->args.size(), 1);
  // EXPECT_EQ(opt_stack_frame->args[0], "native");

  normal_line = "at eval (eval at <anonymous> (foo.html:1), <anonymous>:1:6)";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "<anonymous>");
  EXPECT_EQ(opt_stack_frame->filename, "foo.html");
  EXPECT_EQ(opt_stack_frame->lineno, 1);
  EXPECT_EQ(opt_stack_frame->colno, 0);

  normal_line = "at <eval> (<input>:1:15)";
  opt_stack_frame = test_error_reporter.ParseChromiumBasedStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "<eval>");
  EXPECT_EQ(opt_stack_frame->filename, "<input>");
  EXPECT_EQ(opt_stack_frame->lineno, 1);
  EXPECT_EQ(opt_stack_frame->colno, 15);
}

TEST(JSErrorReporterTest, ParseDarwinStackTest) {
  TestJSErrorReporter test_error_reporter;
  std::string normal_line = "Foo@file:///bar.js:1:10";
  auto opt_stack_frame = test_error_reporter.ParseDarwinStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "Foo");
  EXPECT_EQ(opt_stack_frame->filename, "file:///bar.js");
  EXPECT_EQ(opt_stack_frame->lineno, 1);
  EXPECT_EQ(opt_stack_frame->colno, 10);

  normal_line = "";
  opt_stack_frame = test_error_reporter.ParseDarwinStack(normal_line);
  EXPECT_FALSE(opt_stack_frame.has_value());

  normal_line = "eval code@";
  opt_stack_frame = test_error_reporter.ParseDarwinStack(normal_line);
  EXPECT_FALSE(opt_stack_frame.has_value());

  normal_line = "parse@[native code]";
  opt_stack_frame = test_error_reporter.ParseDarwinStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "parse");
  EXPECT_EQ(opt_stack_frame->filename, "[native code]");
  EXPECT_EQ(opt_stack_frame->lineno, 0);
  EXPECT_EQ(opt_stack_frame->colno, 0);

  normal_line = "eval@[native code]";
  opt_stack_frame = test_error_reporter.ParseDarwinStack(normal_line);
  EXPECT_TRUE(opt_stack_frame.has_value());
  EXPECT_EQ(opt_stack_frame->function, "eval");
  EXPECT_EQ(opt_stack_frame->filename, "[native code]");
  EXPECT_EQ(opt_stack_frame->lineno, 0);
  EXPECT_EQ(opt_stack_frame->colno, 0);
}

TEST(JSErrorReporterTest, ComputeStackTraceTest) {
  TestJSErrorReporter test_error_reporter;
  JSErrorInfo error;
  error.message = "foo";
  error.name = "error";
  error.stack = R"(
    at App (file://view3/foo.js:1865:33)
    at <anonymous> (file://view3/foo.js:389:31)
    at forEach (native)
    at require (file://shared/bar.js:6250:9)
    at loadCard (file://shared/bar.js:7794:63)
    )";
  auto stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.name, "error");
  EXPECT_EQ(stack_trace.message, "foo");
  EXPECT_EQ(stack_trace.frames.size(), 5);
  stack_trace = test_error_reporter.ComputeStackTrace(error, true);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.name, "error");
  EXPECT_EQ(stack_trace.message, "foo");
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].filename, "file://view3/foo.js");

  error.stack = R"(
    at <input>:0:0
    at parse (native)
    at App (file://view3/foo.js:1870:21)
    )";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 3);
  stack_trace = test_error_reporter.ComputeStackTrace(error, true);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].filename, "<input>");

  error.stack = R"(
    at <eval> (<input>:1:1)
    at App (file://view3/foo.js:1872:44)
    )";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 2);
  stack_trace = test_error_reporter.ComputeStackTrace(error, true);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].filename, "<input>");

  error.stack = R"(
Foo@file:///foo.js:1865:26
forEach@[native code]
ay@file:///bar.js:1:188888
    )";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.name, "error");
  EXPECT_EQ(stack_trace.message, "foo");
  EXPECT_EQ(stack_trace.frames.size(), 3);

  stack_trace = test_error_reporter.ComputeStackTrace(error, true);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.name, "error");
  EXPECT_EQ(stack_trace.message, "foo");
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].filename, "file:///foo.js");

  error.stack = R"(
eval code@
eval@[native code]
Foo@file:///foo.js:1869:17
    )";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 2);
  stack_trace = test_error_reporter.ComputeStackTrace(error, true);
  EXPECT_FALSE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].filename, "[native code]");

  error.stack = "";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_TRUE(stack_trace.failed);
  EXPECT_EQ(stack_trace.frames.size(), 0);

  error.stack = R"(at captureMessage (file://foo.js:1:1)
  at foo (file://foo.js:2:1)
  at sentryWrapped (file://foo.js:3:1))";
  stack_trace = test_error_reporter.ComputeStackTrace(error, false);
  EXPECT_EQ(stack_trace.frames.size(), 1);
  EXPECT_EQ(stack_trace.frames[0].function, "foo");
}

TEST(JSErrorReporterTest, ComputeStackTraceJSCTest) {
  TestJSErrorReporter test_error_reporter;
  auto stacktrace =
      test_error_reporter.ComputeStackTrace(kJSCTypeError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCTypeError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 3);

    EXPECT_EQ(stacktrace.frames[0].colno, 10);
    EXPECT_EQ(stacktrace.frames[0].function, "not_a_func");
    EXPECT_EQ(stacktrace.frames[0].lineno, 431);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 212072);
    EXPECT_EQ(stacktrace.frames[1].function, "?");
    EXPECT_EQ(stacktrace.frames[1].lineno, 1);
    EXPECT_EQ(stacktrace.frames[1].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 210891);
    EXPECT_EQ(stacktrace.frames[2].function, "?");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kJSCTypeError2, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCTypeError2.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 21);
    EXPECT_EQ(stacktrace.frames[0].function, "?");
    EXPECT_EQ(stacktrace.frames[0].lineno, 452);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 6);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 413);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 16);
    EXPECT_EQ(stacktrace.frames[2].function, "undefNotAObj");
    EXPECT_EQ(stacktrace.frames[2].lineno, 451);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 212072);
    EXPECT_EQ(stacktrace.frames[3].function, "?");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 210891);
    EXPECT_EQ(stacktrace.frames[4].function, "?");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }
  stacktrace =
      test_error_reporter.ComputeStackTrace(kJSCReferenceError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCReferenceError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 19);
    EXPECT_EQ(stacktrace.frames[0].function, "?");
    EXPECT_EQ(stacktrace.frames[0].lineno, 459);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 6);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 413);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 16);
    EXPECT_EQ(stacktrace.frames[2].function, "refError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 458);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 212072);
    EXPECT_EQ(stacktrace.frames[3].function, "?");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 210891);
    EXPECT_EQ(stacktrace.frames[4].function, "?");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }
  stacktrace =
      test_error_reporter.ComputeStackTrace(kJSCErrorWithNative, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCErrorWithNative.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 24);
    EXPECT_EQ(stacktrace.frames[0].function, "?");
    EXPECT_EQ(stacktrace.frames[0].lineno, 472);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "forEach");
    EXPECT_EQ(stacktrace.frames[1].lineno, 0);
    EXPECT_EQ(stacktrace.frames[1].filename, "[native code]");

    EXPECT_EQ(stacktrace.frames[2].colno, 24);
    EXPECT_EQ(stacktrace.frames[2].function, "inNative");
    EXPECT_EQ(stacktrace.frames[2].lineno, 471);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 212072);
    EXPECT_EQ(stacktrace.frames[3].function, "?");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 210891);
    EXPECT_EQ(stacktrace.frames[4].function, "?");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }
  stacktrace = test_error_reporter.ComputeStackTrace(kJSCEvalError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCEvalError.message);
    EXPECT_EQ(stacktrace.frames.size(), 6);

    EXPECT_EQ(stacktrace.frames[0].colno, 0);
    EXPECT_EQ(stacktrace.frames[0].function, "eval");
    EXPECT_EQ(stacktrace.frames[0].lineno, 0);
    EXPECT_EQ(stacktrace.frames[0].filename, "[native code]");

    EXPECT_EQ(stacktrace.frames[1].colno, 20);
    EXPECT_EQ(stacktrace.frames[1].function, "?");
    EXPECT_EQ(stacktrace.frames[1].lineno, 473);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 6);
    EXPECT_EQ(stacktrace.frames[2].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 413);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 16);
    EXPECT_EQ(stacktrace.frames[3].function, "withEval");
    EXPECT_EQ(stacktrace.frames[3].lineno, 472);
    EXPECT_EQ(stacktrace.frames[3].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 212072);
    EXPECT_EQ(stacktrace.frames[4].function, "?");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[5].colno, 210891);
    EXPECT_EQ(stacktrace.frames[5].function, "?");
    EXPECT_EQ(stacktrace.frames[5].lineno, 1);
    EXPECT_EQ(stacktrace.frames[5].filename, "lynx_core.js");
  }
  stacktrace =
      test_error_reporter.ComputeStackTrace(kJSCCommonChunkError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kJSCCommonChunkError.message);
    EXPECT_EQ(stacktrace.frames.size(), 4);

    EXPECT_EQ(stacktrace.frames[0].colno, 18);
    EXPECT_EQ(stacktrace.frames[0].function,
              "JS error from native: [JSC] call js function fail in "
              "JSCHelper::call! ,fake f");
    EXPECT_EQ(stacktrace.frames[0].lineno, 42);
    EXPECT_EQ(stacktrace.frames[0].filename,
              "file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 35);
    EXPECT_EQ(stacktrace.frames[1].function, "componentDidMount");
    EXPECT_EQ(stacktrace.frames[1].lineno, 214);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 211362);
    EXPECT_EQ(stacktrace.frames[2].function, "?");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 211691);
    EXPECT_EQ(stacktrace.frames[3].function, "?");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");
  }
}

TEST(JSErrorReporterTest, ComputeStackTraceQuickjsTest) {
  TestJSErrorReporter test_error_reporter;
  auto stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickjsTypeError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickjsTypeError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 16);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 444);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 5);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 413);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 7);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.not_a_func");
    EXPECT_EQ(stacktrace.frames[2].lineno, 443);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 142231);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 141043);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kQuickJSTypeError2, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSTypeError2.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 22);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 452);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 5);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 413);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 7);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.undefNotAObj");
    EXPECT_EQ(stacktrace.frames[2].lineno, 451);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 142231);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 141043);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSReferenceError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSReferenceError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 9);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 459);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 5);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 413);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 7);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.refError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 458);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 142231);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 141043);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSErrorWithNative, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSErrorWithNative.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 15);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 466);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "Array.forEach");
    EXPECT_EQ(stacktrace.frames[1].lineno, 0);
    EXPECT_EQ(stacktrace.frames[1].filename, "native");

    EXPECT_EQ(stacktrace.frames[2].colno, 17);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.inNative");
    EXPECT_EQ(stacktrace.frames[2].lineno, 465);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 142231);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 141043);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kQuickJSEvalError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSEvalError.message);
    EXPECT_EQ(stacktrace.frames.size(), 7);

    EXPECT_EQ(stacktrace.frames[0].colno, 33);
    EXPECT_EQ(stacktrace.frames[0].function, "<eval>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 1);
    EXPECT_EQ(stacktrace.frames[0].filename, "<input>");

    EXPECT_EQ(stacktrace.frames[1].colno, 56);
    EXPECT_EQ(stacktrace.frames[1].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[1].lineno, 474);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 8);
    EXPECT_EQ(stacktrace.frames[2].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 414);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 9);
    EXPECT_EQ(stacktrace.frames[3].function, "withEval");
    EXPECT_EQ(stacktrace.frames[3].lineno, 475);
    EXPECT_EQ(stacktrace.frames[3].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 0);
    EXPECT_EQ(stacktrace.frames[4].function, "apply");
    EXPECT_EQ(stacktrace.frames[4].lineno, 0);
    EXPECT_EQ(stacktrace.frames[4].filename, "native");

    EXPECT_EQ(stacktrace.frames[5].colno, 62);
    EXPECT_EQ(stacktrace.frames[5].function, "handlerEvent");
    EXPECT_EQ(stacktrace.frames[5].lineno, 6183);
    EXPECT_EQ(stacktrace.frames[5].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[6].colno, 73);
    EXPECT_EQ(stacktrace.frames[6].function, "publishEvent");
    EXPECT_EQ(stacktrace.frames[6].lineno, 6116);
    EXPECT_EQ(stacktrace.frames[6].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSOnlineError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSOnlineError.message);
    EXPECT_EQ(stacktrace.frames.size(), 14);

    EXPECT_EQ(stacktrace.frames[0].colno, 911126);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 1);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "call");
    EXPECT_EQ(stacktrace.frames[1].lineno, 0);
    EXPECT_EQ(stacktrace.frames[1].filename, "native");

    EXPECT_EQ(stacktrace.frames[2].colno, 6632);
    EXPECT_EQ(stacktrace.frames[2].function, "c");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 6419);
    EXPECT_EQ(stacktrace.frames[3].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 7060);
    EXPECT_EQ(stacktrace.frames[4].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[5].colno, 2759);
    EXPECT_EQ(stacktrace.frames[5].function, "B");
    EXPECT_EQ(stacktrace.frames[5].lineno, 1);
    EXPECT_EQ(stacktrace.frames[5].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[6].colno, 2978);
    EXPECT_EQ(stacktrace.frames[6].function, "a");
    EXPECT_EQ(stacktrace.frames[6].lineno, 1);
    EXPECT_EQ(stacktrace.frames[6].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[7].colno, 3025);
    EXPECT_EQ(stacktrace.frames[7].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[7].lineno, 1);
    EXPECT_EQ(stacktrace.frames[7].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[8].colno, 1935);
    EXPECT_EQ(stacktrace.frames[8].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[8].lineno, 1);
    EXPECT_EQ(stacktrace.frames[8].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[9].colno, 2026);
    EXPECT_EQ(stacktrace.frames[9].function, "u");
    EXPECT_EQ(stacktrace.frames[9].lineno, 1);
    EXPECT_EQ(stacktrace.frames[9].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[10].colno, 599);
    EXPECT_EQ(stacktrace.frames[10].function, "a");
    EXPECT_EQ(stacktrace.frames[10].lineno, 1);
    EXPECT_EQ(stacktrace.frames[10].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[11].colno, 3028);
    EXPECT_EQ(stacktrace.frames[11].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[11].lineno, 1);
    EXPECT_EQ(stacktrace.frames[11].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[12].colno, 913394);
    EXPECT_EQ(stacktrace.frames[12].function, "n");
    EXPECT_EQ(stacktrace.frames[12].lineno, 1);
    EXPECT_EQ(stacktrace.frames[12].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[13].colno, 124512);
    EXPECT_EQ(stacktrace.frames[13].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[13].lineno, 1);
    EXPECT_EQ(stacktrace.frames[13].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSOnlineError2, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSOnlineError2.message);
    EXPECT_EQ(stacktrace.frames.size(), 9);

    EXPECT_EQ(stacktrace.frames[0].colno, 60526);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 1);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "call");
    EXPECT_EQ(stacktrace.frames[1].lineno, 0);
    EXPECT_EQ(stacktrace.frames[1].filename, "native");

    EXPECT_EQ(stacktrace.frames[2].colno, 82749);
    EXPECT_EQ(stacktrace.frames[2].function, "d");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 82536);
    EXPECT_EQ(stacktrace.frames[3].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 83201);
    EXPECT_EQ(stacktrace.frames[4].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[5].colno, 3027);
    EXPECT_EQ(stacktrace.frames[5].function, "de");
    EXPECT_EQ(stacktrace.frames[5].lineno, 1);
    EXPECT_EQ(stacktrace.frames[5].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[6].colno, 3248);
    EXPECT_EQ(stacktrace.frames[6].function, "i");
    EXPECT_EQ(stacktrace.frames[6].lineno, 1);
    EXPECT_EQ(stacktrace.frames[6].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[7].colno, 987);
    EXPECT_EQ(stacktrace.frames[7].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[7].lineno, 1);
    EXPECT_EQ(stacktrace.frames[7].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[8].colno, 1023);
    EXPECT_EQ(stacktrace.frames[8].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[8].lineno, 1);
    EXPECT_EQ(stacktrace.frames[8].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSOnlineError3, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSOnlineError3.message);
    EXPECT_EQ(stacktrace.frames.size(), 7);

    EXPECT_EQ(stacktrace.frames[0].colno, 351931);
    EXPECT_EQ(stacktrace.frames[0].function, "dispatchException");
    EXPECT_EQ(stacktrace.frames[0].lineno, 1);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 348108);
    EXPECT_EQ(stacktrace.frames[1].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[1].lineno, 1);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 348848);
    EXPECT_EQ(stacktrace.frames[2].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 1477);
    EXPECT_EQ(stacktrace.frames[3].function, "f");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 1696);
    EXPECT_EQ(stacktrace.frames[4].function, "a");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1);
    EXPECT_EQ(stacktrace.frames[4].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[5].colno, 987);
    EXPECT_EQ(stacktrace.frames[5].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[5].lineno, 1);
    EXPECT_EQ(stacktrace.frames[5].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[6].colno, 1023);
    EXPECT_EQ(stacktrace.frames[6].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[6].lineno, 1);
    EXPECT_EQ(stacktrace.frames[6].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kQuickJSCommonChunkError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kQuickJSCommonChunkError.message);
    EXPECT_EQ(stacktrace.frames.size(), 4);

    EXPECT_EQ(stacktrace.frames[0].colno, 24);
    EXPECT_EQ(stacktrace.frames[0].function, "f");
    EXPECT_EQ(stacktrace.frames[0].lineno, 42);
    EXPECT_EQ(stacktrace.frames[0].filename,
              "file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 37);
    EXPECT_EQ(stacktrace.frames[1].function, "componentDidMount");
    EXPECT_EQ(stacktrace.frames[1].lineno, 214);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 141518);
    EXPECT_EQ(stacktrace.frames[2].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[2].lineno, 1);
    EXPECT_EQ(stacktrace.frames[2].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 141848);
    EXPECT_EQ(stacktrace.frames[3].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");
  }
}

TEST(JSErrorReporterTest, ComputeStackTraceV8Test) {
  TestJSErrorReporter test_error_reporter;
  auto stacktrace = test_error_reporter.ComputeStackTrace(kV8TypeError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kV8TypeError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 0);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 445);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 414);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 0);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.not_a_func");
    EXPECT_EQ(stacktrace.frames[2].lineno, 444);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 0);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1133);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 0);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1054);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kV8ReferenceError1, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kV8ReferenceError1.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 0);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 460);
    EXPECT_EQ(stacktrace.frames[0].filename, "file://app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[1].lineno, 414);
    EXPECT_EQ(stacktrace.frames[1].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 0);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.refError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 459);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 0);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1133);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 0);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1054);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kV8ErrorWithNative, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kV8ErrorWithNative.message);
    EXPECT_EQ(stacktrace.frames.size(), 5);

    EXPECT_EQ(stacktrace.frames[0].colno, 0);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 467);
    EXPECT_EQ(stacktrace.frames[0].filename, "file://app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "Array.forEach");
    EXPECT_EQ(stacktrace.frames[1].lineno, 0);
    EXPECT_EQ(stacktrace.frames[1].filename, "<anonymous>");

    EXPECT_EQ(stacktrace.frames[2].colno, 0);
    EXPECT_EQ(stacktrace.frames[2].function, "_Page.inNative");
    EXPECT_EQ(stacktrace.frames[2].lineno, 466);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 0);
    EXPECT_EQ(stacktrace.frames[3].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[3].lineno, 1133);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 0);
    EXPECT_EQ(stacktrace.frames[4].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1054);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");
  }

  stacktrace = test_error_reporter.ComputeStackTrace(kV8EvalError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kV8EvalError.message);
    EXPECT_EQ(stacktrace.frames.size(), 6);

    EXPECT_EQ(stacktrace.frames[0].colno, 0);
    EXPECT_EQ(stacktrace.frames[0].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[0].lineno, 474);
    EXPECT_EQ(stacktrace.frames[0].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 0);
    EXPECT_EQ(stacktrace.frames[1].function, "<anonymous>");
    EXPECT_EQ(stacktrace.frames[1].lineno, 474);
    EXPECT_EQ(stacktrace.frames[1].filename, "file://app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 0);
    EXPECT_EQ(stacktrace.frames[2].function, "wrapError");
    EXPECT_EQ(stacktrace.frames[2].lineno, 414);
    EXPECT_EQ(stacktrace.frames[2].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 0);
    EXPECT_EQ(stacktrace.frames[3].function, "_Page.withEval");
    EXPECT_EQ(stacktrace.frames[3].lineno, 473);
    EXPECT_EQ(stacktrace.frames[3].filename, "file:///app-service.js");

    EXPECT_EQ(stacktrace.frames[4].colno, 0);
    EXPECT_EQ(stacktrace.frames[4].function, "App.handlerEvent");
    EXPECT_EQ(stacktrace.frames[4].lineno, 1133);
    EXPECT_EQ(stacktrace.frames[4].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[5].colno, 0);
    EXPECT_EQ(stacktrace.frames[5].function, "App.publishEvent");
    EXPECT_EQ(stacktrace.frames[5].lineno, 1054);
    EXPECT_EQ(stacktrace.frames[5].filename, "lynx_core.js");
  }

  stacktrace =
      test_error_reporter.ComputeStackTrace(kV8CommonChunkError, false);
  stacktrace.frames =
      std::vector(stacktrace.frames.rbegin(), stacktrace.frames.rend());
  {
    EXPECT_EQ(stacktrace.message, kV8CommonChunkError.message);
    EXPECT_EQ(stacktrace.frames.size(), 4);

    EXPECT_EQ(stacktrace.frames[0].colno, 9);
    EXPECT_EQ(stacktrace.frames[0].function, "Object.f");
    EXPECT_EQ(stacktrace.frames[0].lineno, 42);
    EXPECT_EQ(
        stacktrace.frames[0].filename,
        "file://view4/http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");

    EXPECT_EQ(stacktrace.frames[1].colno, 34);
    EXPECT_EQ(stacktrace.frames[1].function, "_Page.componentDidMount");
    EXPECT_EQ(stacktrace.frames[1].lineno, 213);
    EXPECT_EQ(stacktrace.frames[1].filename, "file://view4/app-service.js");

    EXPECT_EQ(stacktrace.frames[2].colno, 24);
    EXPECT_EQ(stacktrace.frames[2].function, "App.updateComponent");
    EXPECT_EQ(stacktrace.frames[2].lineno, 15504);
    EXPECT_EQ(stacktrace.frames[2].filename, "lynx_core.js");

    EXPECT_EQ(stacktrace.frames[3].colno, 16);
    EXPECT_EQ(stacktrace.frames[3].function, "App.didUpdate");
    EXPECT_EQ(stacktrace.frames[3].lineno, 15538);
    EXPECT_EQ(stacktrace.frames[3].filename, "lynx_core.js");
  }
}

TEST(JSErrorReporterTest, SetGetSourceMapReleaseTest) {
  JSErrorReporter reporter;
  JSErrorInfo source_map_release;
  source_map_release.message = "foo";
  source_map_release.stack = R"(
    at App (file://view3/foo.js:1865:33)
    at <anonymous> (file://view3/foo.js:389:31)
    )";
  reporter.SetSourceMapRelease(source_map_release);
  auto ret = reporter.GetSourceMapRelease("file://view3/foo.js");
  EXPECT_EQ(ret, "foo");

  ret = reporter.GetSourceMapRelease("file://view3/bar.js");
  EXPECT_EQ(ret, "");

  source_map_release.message = "bar";
  source_map_release.stack = "at <anonymous> (default:1:1)";
  reporter.SetSourceMapRelease(source_map_release);
  ret = reporter.GetSourceMapRelease("default");
  EXPECT_EQ(ret, "bar");

  JSErrorInfo kLepusNgError{
      .stack =
          "    at <anonymous> (file://lepus.js:1:18)\n    at <eval> "
          "(file://lepus.js:30:386)\n",
      .message = "d73160119ef7e77776246caca2a7b98e",
  };
  reporter.SetSourceMapRelease(kLepusNgError);
  EXPECT_EQ(reporter.GetSourceMapRelease("file://lepus.js"),
            kLepusNgError.message);

  reporter.SetSourceMapRelease(kV8SetSourceMapReleaseError);
  ret = reporter.GetSourceMapRelease("file://view4/app-service.js");
  EXPECT_EQ(ret, kV8SetSourceMapReleaseError.message);

  reporter.SetSourceMapRelease(kV8SetSourceMapReleaseError2);
  ret = reporter.GetSourceMapRelease(
      "file://view4/http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");
  EXPECT_EQ(ret, kV8SetSourceMapReleaseError2.message);

  reporter.SetSourceMapRelease(kQuickJSSetSourceMapReleaseError);
  ret = reporter.GetSourceMapRelease("file:///app-service.js");
  EXPECT_EQ(ret, kQuickJSSetSourceMapReleaseError.message);

  reporter.SetSourceMapRelease(kQuickJSSetSourceMapReleaseError2);
  ret = reporter.GetSourceMapRelease(
      "file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");
  EXPECT_EQ(ret, kQuickJSSetSourceMapReleaseError2.message);

  reporter.SetSourceMapRelease(kJSCSetSourceMapReleaseError);
  ret = reporter.GetSourceMapRelease("file:///app-service.js");
  EXPECT_EQ(ret, kJSCSetSourceMapReleaseError.message);

  reporter.SetSourceMapRelease(kJSCSetSourceMapReleaseError2);
  ret = reporter.GetSourceMapRelease(
      "file://http://foo.bar:8787/dist/lynx_common_chunks/d.ts.js");
  EXPECT_EQ(ret, kJSCSetSourceMapReleaseError2.message);
}

TEST(JSErrorReporterTest, SendMTErrorTest) {
  JSErrorReporter reporter;
  std::unordered_map<std::string, std::string> info;
  info["foo"] = "bar";
  reporter.AddCustomInfoToError(info);
  auto opt_error = reporter.SendMTError(kLepusNgRuntimeOriginErr,
                                        kLynxErrorCodeJavascript, 1);
  EXPECT_TRUE(opt_error.has_value());
  EXPECT_EQ(opt_error->error_code_, kLynxErrorCodeJavascript);
  EXPECT_EQ(opt_error->error_level_, base::LynxErrorLevel::Error);
  EXPECT_EQ(
      opt_error->error_message_,
      R"({"rawError":{"stack":"backtrace:\n    at <anonymous> (file://lepus.js:24:40)\n    at <anonymous> (file://lepus.js:26:334)\n    at render (file://lepus.js:27:118)\n    at entryPoint (file://lepus.js:28:47)\n    at $renderPage0 (file://lepus.js:29:184)\n\n","message":"\"lepusng exception: TypeError: not a function ","cause":{"cause":"\"lepusng exception: TypeError: not a function backtrace:\n    at <anonymous> (file://lepus.js:24:40)\n    at <anonymous> (file://lepus.js:26:334)\n    at render (file://lepus.js:27:118)\n    at entryPoint (file://lepus.js:28:47)\n    at $renderPage0 (file://lepus.js:29:184)\n\ntemplate_debug_url:http:/foo.bar:8787/dist/00-counter-2/intermediate/debug-info.json\""}},"pid":"INTERNAL_ERROR","url":"file://lynx_core.js","dynamicComponentPath":"","sentry":{"platform":"javascript","sdk":{"name":"sentry.javascript.lynx","version":"9.0.0","packages":[{"name":"npm:@sentry/browser","version":"9.0.0"}],"integrations":["InboundFilters","FunctionToString","Breadcrumbs","GlobalHandlers","LinkedErrors","UserAgent"]},"level":"error","exception":{"values":[{"type":"Error","value":"\"lepusng exception: TypeError: not a function ","stacktrace":{"frames":[{"colno":184,"filename":"file://lepus.js","function":"$renderPage0","in_app":true,"release":"","lineno":29},{"colno":47,"filename":"file://lepus.js","function":"entryPoint","in_app":true,"release":"","lineno":28},{"colno":118,"filename":"file://lepus.js","function":"render","in_app":true,"release":"","lineno":27},{"colno":334,"filename":"file://lepus.js","function":"<anonymous>","in_app":true,"release":"","lineno":26},{"colno":40,"filename":"file://lepus.js","function":"<anonymous>","in_app":true,"release":"","lineno":24}]},"mechanism":{"handled":true,"type":"generic"}}]},"tags":{"error_type":"Error","extra":"\"lepusng exception: TypeError: not a function ","lib_version":"","run_type":"lynx_core","version_code":""}}})");
  EXPECT_EQ(opt_error->custom_info_["foo"], "bar");
}

TEST(JSErrorReporterTest, GetFileNameFromStackTest) {
  TestJSErrorReporter test_error_reporter;
  auto ret = test_error_reporter.GetFileNameFromStack("");
  EXPECT_EQ(ret, "");

  std::string error_stack = R"(
    at App (file://view3/foo.js:1872:44)
    )";
  ret = test_error_reporter.GetFileNameFromStack(error_stack);
  EXPECT_EQ(ret, "");

  error_stack = R"(
    at Foo (dynamic-component/foo//app-service.js:12:4)
    )";
  ret = test_error_reporter.GetFileNameFromStack(error_stack);
  EXPECT_EQ(ret, "foo");
}

TEST(JSErrorReporterTest, SendBTErrorTest) {
  JSErrorReporter reporter;
  JSErrorInfo error_info;
  error_info.error_code = kLynxErrorCodeJavascript;
  error_info.error_level = base::LynxErrorLevel::Error;
  error_info.build_version = "0.0.1";
  error_info.version_code = "0.0.1";
  error_info.cause = "cause";
  error_info.name = "TypeError";
  error_info.kind = "USER_ERROR";
  error_info.message = "not a function";
  error_info.release = "foobar";
  error_info.stack =
      R"(TypeError: not a function    at <anonymous> (file://view2/app-service.js:445:21)
    at wrapError (file://view2/app-service.js:414:8)
    at not_a_func (file://view2/app-service.js:446:9)
    at apply (native)
    at handlerEvent (file://shared/lynx_core.js:5913:62)
    at publishEvent (file://shared/lynx_core.js:5846:73))";
  // error_info.slot = "";
  std::unordered_map<std::string, std::string> info;
  info["foo"] = "bar";
  reporter.AddCustomInfoToError(info);
  auto opt_error = reporter.SendBTError(error_info);
  EXPECT_TRUE(opt_error.has_value());
  EXPECT_EQ(opt_error->error_code_, kLynxErrorCodeJavascript);
  EXPECT_EQ(opt_error->error_level_, base::LynxErrorLevel::Error);

  EXPECT_EQ(
      opt_error->error_message_,
      "{\"rawError\":{\"stack\":\"TypeError: not a function    at <anonymous> "
      "(file://view2/app-service.js:445:21)\\n    at wrapError "
      "(file://view2/app-service.js:414:8)\\n    at not_a_func "
      "(file://view2/app-service.js:446:9)\\n    at apply (native)\\n    at "
      "handlerEvent (file://shared/lynx_core.js:5913:62)\\n    at publishEvent "
      "(file://shared/lynx_core.js:5846:73)\",\"message\":\"not a "
      "function\",\"cause\":{\"cause\":\"cause\"}},\"pid\":\"USER_ERROR\","
      "\"url\":\"file://"
      "lynx_core.js\",\"dynamicComponentPath\":\"\",\"sentry\":{\"platform\":"
      "\"javascript\",\"sdk\":{\"name\":\"sentry.javascript.lynx\","
      "\"version\":\"9.0.0\",\"packages\":[{\"name\":\"npm:@sentry/"
      "browser\",\"version\":\"9.0.0\"}],\"integrations\":[\"InboundFilters\","
      "\"FunctionToString\",\"Breadcrumbs\",\"GlobalHandlers\","
      "\"LinkedErrors\",\"UserAgent\"]},\"level\":\"error\",\"exception\":{"
      "\"values\":[{\"type\":\"TypeError\",\"value\":\"not a "
      "function\",\"stacktrace\":{\"frames\":[{\"colno\":73,\"filename\":"
      "\"lynx_core.foobar.js\",\"function\":\"publishEvent\",\"in_app\":true,"
      "\"release\":\"foobar\",\"lineno\":5846},{\"colno\":62,\"filename\":"
      "\"lynx_core.foobar.js\",\"function\":\"handlerEvent\",\"in_app\":true,"
      "\"release\":\"foobar\",\"lineno\":5913},{\"colno\":0,\"filename\":"
      "\"native\",\"function\":\"apply\",\"in_app\":true,\"release\":\"\","
      "\"lineno\":0},{\"colno\":9,\"filename\":\"file://view2/"
      "app-service.js\",\"function\":\"not_a_func\",\"in_app\":true,"
      "\"release\":"
      "\"\",\"lineno\":446},{\"colno\":8,\"filename\":\"file://view2/"
      "app-service.js\",\"function\":\"wrapError\",\"in_app\":true,\"release\":"
      "\"\",\"lineno\":414},{\"colno\":21,\"filename\":\"file://view2/"
      "app-service.js\",\"function\":\"<anonymous>\",\"in_app\":true,"
      "\"release\":\"\",\"lineno\":445}]},\"mechanism\":{\"handled\":true,"
      "\"type\":\"generic\"}}]},\"tags\":{\"error_type\":\"TypeError\","
      "\"extra\":\"not a "
      "function\",\"lib_version\":\"0.0.1\",\"run_type\":\"lynx_core\","
      "\"version_code\":\"0.0.1\"}}}");
  EXPECT_EQ(opt_error->custom_info_["foo"], "bar");
}

TEST(JSErrorReporterTest, StackLimitTest) {
  JSErrorReporter reporter;
  JSErrorInfo error_info;
  for (int i = 0; i < 100; i++) {
    error_info.stack += "foo bar\n";
  }
  reporter.SendBTError(error_info);
  std::vector<std::string> lines;
  base::SplitString(error_info.stack, '\n', lines);
  EXPECT_EQ(lines.size(), 50);
}

TEST(JSErrorReporterTest, FormatErrorUrl) {
  constexpr auto check_path_and_query =
      [](const std::string& url, const std::optional<std::string>& expect_path,
         const std::optional<std::string>& expect_query) {
        constexpr auto check_res =
            [](const auto& map, const std::string& key,
               const std::optional<std::string>& expect_res) {
              auto res = map.find(key);
              if (expect_res) {
                EXPECT_NE(res, map.end());
                EXPECT_EQ(res->second, *expect_res);
              } else {
                EXPECT_EQ(res, map.end());
              }
            };

        base::LynxError error{100, ""};
        FormatErrorUrl(error, url);
        // check path
        check_res(error.custom_info_, "lynx_context_component_url",
                  expect_path);
        // check query
        check_res(error.custom_info_, "lynx_context_component_url_query",
                  expect_query);
      };

  constexpr char kTestUrl[] = "https://lynx.js";
  check_path_and_query("", std::nullopt, std::nullopt);
  check_path_and_query("https://lynx.js", kTestUrl, std::nullopt);
  check_path_and_query("https://lynx.js?", kTestUrl, "");
  check_path_and_query("https://lynx.js?lynx=1&dynamic=1", kTestUrl,
                       "lynx=1&dynamic=1");
}

TEST(JSErrorReporterTest, GetDynamicComponentPath) {
  JSErrorReporter reporter;
  JSErrorInfo error_info;

  // stack doesn't contain dynamic component path
  error_info.stack =
      R"(TypeError: not a function    at <anonymous> (file://view2/app-service.js:445:21)
    at wrapError (file://view2/app-service.js:414:8)
    at not_a_func (file://view2/app-service.js:446:9)
    at apply (native)
    at handlerEvent (file://shared/lynx_core.js:5913:62)
    at publishEvent (file://shared/lynx_core.js:5846:73))";
  auto opt_error = reporter.SendBTError(error_info);
  EXPECT_TRUE(opt_error);
  EXPECT_TRUE(opt_error->custom_info_.empty());

  // stack contains dynamic component path
  error_info.stack =
      R"(TypeError: not a function    at <anonymous> (file://view2/app-service.js:445:21)
    at _App (dynamic-component/https://test/dynamic-component.js//app-service.js:106:5)
    at onReactComponentCreated (\/lynx_core.js:7:29529)\\n 
    at apply (native)
    at publishEvent (file://shared/lynx_core.js:5846:73))";
  opt_error = reporter.SendBTError(error_info);
  EXPECT_TRUE(opt_error);
  EXPECT_EQ(opt_error->custom_info_["lynx_context_component_url"],
            "https://test/dynamic-component.js");
}

TEST(JSErrorReporterTest, CustomInfo) {
  JSErrorReporter reporter;
  std::unordered_map<std::string, std::string> info;
  info["foo"] = "bar";
  reporter.AddCustomInfoToError(info);
  base::LynxError error(0, "");
  reporter.AppendCustomInfo(error);
  EXPECT_EQ(error.custom_info_["foo"], "bar");
}

}  // namespace testing
}  // namespace common
}  // namespace lynx
