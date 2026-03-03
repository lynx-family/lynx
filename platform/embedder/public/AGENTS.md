# Public Embedder API — Architecture Guide

This directory (`platform/embedder/public/`) is the **public API surface** of the
Lynx embedder. It is consumed by external projects (e.g. Lynxtron) either via
source inclusion (`BUILD_WITH_LYNX`) or as a prebuilt shared library (LynxSDK).
Every file here ships to consumers, so strict rules apply.

## Directory Structure

```
public/
├── capi/              # Pure-C ABI layer (the real shared-library boundary)
│   ├── lynx_export.h  # LYNX_CAPI_EXPORT / LYNX_EXTERN_C macros
│   ├── *_capi.h       # C function declarations & opaque typedefs
│   └── ...
├── *.h                # C++ convenience wrappers over the CAPI
└── AGENTS.md          # (this file)
```

## Two-Layer Design

| Layer | Location | Language | Purpose |
|-------|----------|----------|---------|
| **CAPI** | `capi/*.h` | **Pure C** | Stable ABI boundary. All symbols exported from the shared library live here. |
| **C++ Wrappers** | `*.h` (this dir) | C++ | Thin, header-only wrappers that call CAPI functions. Provide ergonomic C++ API (classes, shared_ptr, std::string) to consumers. |

Data flows: **Consumer C++ code → C++ wrapper (inline) → CAPI function → internal implementation**

## Hard Rules

### CAPI headers (`capi/*.h`)

1. **Pure C only.** No `namespace`, `class`, `template`, `std::*`, references,
   or any C++ keyword. Must compile with a C compiler.
2. **No C++ forward declarations**, even behind `#ifdef __cplusplus`.
   This is a C header — C++ types do not belong here in any form.
3. **No `pub/` C++ types across the CAPI boundary.** C++ classes defined in
   `public/*.h` (e.g. `LynxEventSimulationProxy`, `LynxViewClient`, `LynxView`)
   must **never** appear in CAPI function signatures or CAPI implementation code
   — not as `shared_ptr`, not as raw pointers, not as forward declarations, not
   as `void*` that gets cast back. The CAPI is the LynxSDK binary boundary;
   consumers using the prebuilt shared library do not compile these C++ class
   definitions, so the CAPI implementation side (`.cc`) cannot depend on their
   ABI layout. If a `void*` context passes through CAPI, the CAPI `.cc` must
   treat it as opaque and never cast it to a `pub/` C++ type. Only the
   consumer-side inline code (in `public/*.h` wrappers) may cast it back.
4. **CAPI implementation (`.cc`) must not `#include` or inherit from `pub/` C++
   types.** The dependency direction is: CAPI `.cc` → internal code only. If the
   CAPI needs to create an adapter (e.g. wrapping a C callback into a C++
   interface), that adapter must live in internal code and inherit from an
   internal type, not a `pub/` type. The CAPI `.cc` should pass raw C types to
   an internal function that handles the C++ wrapping.
5. **All exported functions** must use `LYNX_CAPI_EXPORT` for visibility and be
   inside `LYNX_EXTERN_C_BEGIN` / `LYNX_EXTERN_C_END` for C linkage.
6. **Implementation files** (`.cc`) must use `LYNX_EXTERN_C` (not
   `LYNX_CAPI_EXPORT`) on function definitions. `LYNX_CAPI_EXPORT` controls
   symbol visibility (dllexport/visibility), `LYNX_EXTERN_C` controls linkage
   (`extern "C"`). The declaration in the header (inside the
   `LYNX_EXTERN_C_BEGIN` block) already carries both. The definition only needs
   `LYNX_EXTERN_C`.
7. **Opaque types** use the `typedef struct foo_t foo_t;` pattern.
8. **Callbacks** use C function pointers with a `void* context` parameter for
   user data. Never use C++ callable objects (std::function, etc.) in CAPI
   signatures.

### C++ wrapper headers (`*.h`)

1. **Header-only.** All method bodies are inline and delegate to CAPI functions.
   No non-inline function declarations (they would require a .cc and cause
   linker errors for shared-library consumers since they are not exported).
2. **No `static const std::string` or other members requiring `.cc` definitions.**
   `static const std::string` needs a definition in a `.cc` file, cannot be
   exported safely across shared-library boundaries (ABI mismatch, static
   initialization order), and violates the header-only principle. Use
   `static constexpr const char*` instead — it is truly header-only, has no
   ABI issues, and needs no `.cc` definition.
3. **No struct/class definitions for CAPI opaque types.** The C++ wrappers see
   CAPI opaque types only through the forward declaration in the CAPI header.
4. **Lifetime management** of C++ objects (e.g. `shared_ptr`) that must cross
   the CAPI boundary is done in the wrapper layer, not the CAPI. Store the
   `shared_ptr` as a class member, pass a raw pointer or C callback through
   CAPI.

## Patterns

### Passing callbacks across the CAPI

When C++ code has a virtual interface that needs to cross the CAPI boundary:

**CAPI side** — declare a C function pointer type + accept `void* context`:
```c
// capi/lynx_view_capi.h
typedef void (*lynx_emulate_touch_fn)(void* context, const char* event_type,
                                      int x, int y, ...);
LYNX_CAPI_EXPORT void lynx_view_set_event_simulation_proxy(
    lynx_view_t* view, lynx_emulate_touch_fn callback, void* context);
```

**C++ wrapper side** — store the C++ object, pass trampoline + raw pointer:
```cpp
// lynx_view.h
void SetEventSimulationProxy(std::shared_ptr<LynxEventSimulationProxy> proxy) {
  event_proxy_ = std::move(proxy);
  if (event_proxy_) {
    lynx_view_set_event_simulation_proxy(
        lynx_view_,
        [](void* ctx, const char* event_type, int x, int y, ...) {
          static_cast<LynxEventSimulationProxy*>(ctx)->EmulateTouch(
              event_type, x, y, ...);
        },
        event_proxy_.get());
  } else {
    lynx_view_set_event_simulation_proxy(lynx_view_, NULL, NULL);
  }
}
```

**Implementation side** — adapter lives in **internal** code (not CAPI `.cc`),
inherits from an **internal** interface, not a `pub/` type:
```cpp
// internal code (e.g. lynx_template_renderer.cc), NOT lynx_view.cc
// The CAPI .cc just passes the raw callback + void* to an internal function.
// The internal function creates the adapter.
void SetEventSimulationCallback(lynx_emulate_touch_fn cb, void* ctx) {
  SetProxy(std::make_shared<CallbackAdapter>(cb, ctx));
}
```

### Passing objects with lifecycle across the CAPI

See `LynxViewClient` for the canonical pattern:
- `capi/lynx_view_client_capi.h` defines an opaque `lynx_view_client_t` with
  `create`/`release`/`bind_*` CAPI functions (all pure C).
- `lynx_view_client.h` defines a C++ class that owns a `lynx_view_client_t*`,
  binds stateless lambdas as C callbacks in the constructor.
- Consumer subclasses the C++ class; virtual dispatch happens through
  C callback → `Unwrap` → virtual call.

## Common Mistakes to Avoid

- Putting `std::shared_ptr`, `std::string`, or any C++ type in a CAPI header
  (even behind `#ifdef __cplusplus`). It is still a C header — do not mix.
- Casting a `void*` context back to a `pub/` C++ class inside CAPI
  implementation (`.cc`) files. The CAPI `.cc` runs inside the shared library;
  `pub/` C++ classes are compiled by consumers, so ABI mismatch will occur.
  Only consumer-side inline code (in `public/*.h` wrappers) may cast `void*`
  back to `pub/` C++ types. The CAPI `.cc` should either treat `void*` as
  fully opaque (just store and pass it back via callback), or cast it to an
  **internal** C++ type defined within the library.
- Including `pub/` C++ headers or inheriting from `pub/` types in CAPI `.cc`
  files. The CAPI implementation layer depends on internal code only. If you
  need an adapter class, put it in internal code and have the CAPI `.cc` call
  an internal function.
- Using `static const std::string` members in `pub/` headers. These require a
  `.cc` definition, cannot be safely exported from a shared library (ABI risk,
  static init order fiasco), and break the header-only contract. Use
  `static constexpr const char*` instead.
- Declaring non-inline, non-exported functions in public C++ headers. Shared
  library consumers will get linker errors.
- Using `LYNX_CAPI_EXPORT` on function definitions in `.cc` files instead of
  `LYNX_EXTERN_C`. `LYNX_CAPI_EXPORT` is visibility only; `LYNX_EXTERN_C`
  provides `extern "C"` linkage.
- Creating opaque handle types with C++ internals in public headers. Opaque
  struct definitions belong in private implementation files only.
