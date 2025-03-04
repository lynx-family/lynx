# What is lynx-trace?

lynx-trace is an independent and shareable instrumentation tool that implements Android and Darwin (iOS and macOS) APIs based on perfetto.

For performance reasons, perfetto uses a large number of static variables to store information, which results in multiple dynamic libraries being unable to share a single copy of the perfetto code. lynx-trace encapsulates the global static variables and APIs of perfetto within the module, exposing only the platform layer interfaces, including [Android APIs](./android/src/main/java/com/lynx/tasm/base/TraceEvent.java), [Darwin APIs](./darwin/LynxTraceEvent.h), and [C++ shortcut trace event macros](./native/trace_event.h) for developers to utilize.

We don't need to worry about the initialization, configuration, or other setup tasks of perfetto; we can directly call the trace controller interfaces ([Android APIs](./android/src/main/java/com/lynx/tasm/base/TraceController.java), [Darwin APIs](./darwin/LynxTraceController.h), and [C++ APIs](./native/trace_controller_impl.h)) when we need to start or stop tracing.

Additionally, lynx-trace supports switching the backend to the system trace tool provided by Android for recording instrumentation information. By setting enable_trace="systrace" in GN during the compilation process, the resulting lynxtrace.so will use the Android system trace as the backend to record performance instrumentation data.