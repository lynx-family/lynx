# High Memory Host Callbacks

Register a process-wide callback with `LynxEnv.setHighMemoryCallback` on Android
or Harmony, `[LynxEnv setHighMemoryCallback:]` on Darwin, or
`lynx_env_set_high_memory_callback` / `lynx::pub::LynxEnv::SetHighMemoryCallback`
on embedder. The embedder callback receives `user_data` and a borrowed
`const lynx_value*` dictionary, valid until the callback returns. Retain the
value with `lynx_value_add_reference(nullptr, *event, nullptr)` or construct a
`lynx::pub::LynxValue` from it to keep it afterward. Callbacks are retained
globally. Each non-null setter call appends a callback;
null/nil/undefined clears all callbacks. Callbacks run asynchronously on the
platform main thread in registration order. Each delivery snapshots the
callback list when its main-thread task begins. Registration or clearing
during a callback affects later events, while all callbacks in the current
snapshot still receive the current event.
On embedder, registration and clearing must also run on the UI thread.

Each event is sent individually as a dictionary. Detection consumes its
one-shot budget even without a registered callback. Events are not replayed.
All native event assembly and dispatch run on the reporter thread.

## Configuration

Set `memory_monitor_config` before memory settings are first initialized.
Comma-separated key/value pairs support the following thresholds in MiB:

| Key | Default | Scope |
| --- | ---: | --- |
| `large_vm_threshold_mb` | 1024 | One event per shared VM |
| `large_bts_threshold_mb` | 512 | One event per page instance |
| `large_ui_threshold_mb` | 128 | One event per page instance |

The comparison is inclusive (`bytes >= threshold * 1024 * 1024`). Zero
disables that event. Non-integer values are ignored; parsed integers use the
same `uint32_t` conversion as other integer settings.
Page BTS and UI events have separate budgets; reload does not reset them.

## Event Dictionaries

All events contain `type` (`large_vm`, `large_bts`, or `large_ui`), `key`
(string), and `bytes` (integer).

| Type | Field meanings |
| --- | --- |
| `large_vm` | `key`: VM name; `bytes`: VM heap size; `vm_type`: runtime type string |
| `large_bts` | `key`: page key; `bytes`: page BTS bytes; `vm_type`: runtime type string |
| `large_ui` | `key`: page key; `bytes`: page UI bytes; `counts`: alternating category names and instance counts |

`vm_type` is `quickjs`, `v8`, `jsvm`, or `jsc`; it is `unknown` when the
runtime type is unavailable. `large_bts` uses the page's cached runtime type.

With monitoring enabled, VM events also contain `total_page_count` (lifetime
accepted page-to-VM bindings, including pages without slots). QuickJS events
with slot data contain `slots`, a flat array of alternating names (strings) and
byte counts (integers), such as `["pageA", 2222, "unknown", 111]`. Pairs are
sorted by descending bytes and then slot index.
Only occupied slots are returned. Slots 0/1/2 use `unknown`/`common`/`overflow`;
page slots use `page_key`, including exited pages. Equal page keys remain
separate entries. An occupied slot whose page has not yet bound has an empty
name. The event's `bytes` remains the VM heap size; slot values are not summed
into it.

`counts` is a flat array such as `["view", 3, "lynxTasmElement", 7]`. Pair order
is unspecified. It includes all cached categories, including
`mainThreadScriptingEngine`, `backgroundThreadScriptingEngine`, and
`lynxTasmElement`. `bytes` for `large_ui` still measures platform UI bytes
only.

## Detection

QuickJS checks its VM heap in `QuickjsRuntimeInstance::OnGC`, independently
of memory monitoring. Every existing callback can detect high memory.
Without monitoring, existing full-GC callbacks suffice; high-memory reporting
does not enable allocation notifications. At the first
threshold crossing it consumes its flag, even if the VM has no shared group.
The QuickJS VM stores its group name when the realm manager creates its first
context, including when the VM comes from the pool.
Standalone VMs are not registered and do not emit `large_vm`.

V8 and JSVM check successful `RefreshVmHeap` reads only while monitoring is
enabled. JSC does not expose a supported heap measurement.

Page events require monitoring and run only in `RefreshMemoryUsage`, after
shared-slot attribution is applied. A standalone page uses its VM heap as BTS
bytes. A shared page uses its slot. Exited pages do not emit high-memory events.
Producer updates refresh every 16ms while Trace is active and every 60s
otherwise, including builds without Perfetto. Other actual refreshes reset
this interval; high-memory reporting does not introduce a separate timer.
