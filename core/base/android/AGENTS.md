# AGENTS.md

## Scope

This directory contains Android-specific core-base glue such as JNI helpers, Java value wrappers, Android VSync/message-loop integration, and Android-only utility helpers.

## Module Map

- `android_jni.*`, `jni_helper.*`, `java_value.*`, `java_only_*`: JNI and Java-value bridge utilities.
- `message_loop_android_vsync.*`, `vsync_monitor_android.*`: Android-specific frame scheduling and VSync integration.
- `device_utils_android.*`, Android stack-trace helpers, `lynx_error_android.*`, `piper_data.*`: Android-only support helpers.
- `lynx_white_board_android.cc`: Android-side bridge for shared white-board related behavior.

## Key Files And Types

- `jni_helper.*` and `java_value.*` are the most reused contract points here. Small type-conversion changes can fan out broadly.
- `message_loop_android_vsync.*` and `vsync_monitor_android.*` sit on the boundary between Android platform scheduling and shared base VSync semantics.
- The local unittests cover JNI and Java-value basics, but not every Android integration path.

## Typical Change Patterns

- If the issue is Android-only JNI ownership, object conversion, or Java bridge behavior, start in `jni_helper.*` or `java_value.*`.
- If the issue is Android-only frame cadence or task-loop behavior, inspect `message_loop_android_vsync.*` and `vsync_monitor_android.*` together.
- If the issue is shared thread or VSync semantics across platforms, the parent `base/` or `base/threading/` layers may own the real fix.

## Edit Rules

- Keep Android JNI and Java-value conversion logic here; shared threading or base semantics belong in the parent `base/` directory.
- JNI helper and Java-value changes often affect many call sites indirectly. Be careful with ownership and type conversion semantics.

## Invariants And Pitfalls

- JNI utilities here are bridge code, so ownership mistakes often show up as lifecycle bugs instead of compile failures.
- Android VSync files must stay aligned with the shared `VSyncMonitor` contract rather than inventing Android-only callback semantics.

## Common Regression Symptoms

- Android-only crashes or bad type conversion after `java_value` or JNI helper edits.
- Frame scheduling regresses only on Android after `message_loop_android_vsync` or `vsync_monitor_android` changes.

## Validate

Use `lynx-cpp-test` and start with:

- `lynx_base_unittests_exec`

## Notes

- `lynx_base_unittests_exec` includes Android JNI and Java-value unit coverage, but deeper Android framework integration still depends on the surrounding platform stack.
