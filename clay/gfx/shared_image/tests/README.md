<!-- cspell:ignore libepoxy -->

# Fence synchronization regression tests

These standalone tests compile the production C API and fence/representation
source units with flags from an already configured GN output. Dependencies must
already be synchronized. Compilation writes only to `--out` and records source
hashes and commands there. No application is installed or replaced.

```sh
python3 clay/gfx/shared_image/tests/build.py \
  --root /path/to/checkout --gn-out /path/to/configured/android-output \
  --platform android --out /tmp/clay-egl-tests
adb push /tmp/clay-egl-tests/egl-tests /data/local/tmp/clay-egl-tests
adb shell chmod 755 /data/local/tmp/clay-egl-tests
adb shell /data/local/tmp/clay-egl-tests
adb shell /data/local/tmp/clay-egl-tests native-proc
adb shell rm /data/local/tmp/clay-egl-tests
adb push /tmp/clay-egl-tests/vulkan-tests /data/local/tmp/clay-vulkan-tests
adb push /tmp/clay-egl-tests/vulkan-device-tests /data/local/tmp/clay-vulkan-device-tests
adb shell chmod 755 /data/local/tmp/clay-vulkan-tests /data/local/tmp/clay-vulkan-device-tests
adb shell /data/local/tmp/clay-vulkan-tests
adb shell /data/local/tmp/clay-vulkan-tests no-wait-proc
adb shell /data/local/tmp/clay-vulkan-device-tests
adb shell rm /data/local/tmp/clay-vulkan-tests /data/local/tmp/clay-vulkan-device-tests
```

The root may contain `clay/` directly or under `lynx/`. EGL tests simulate driver
entry points and backing allocation; fence, public API and representation
behavior is real. Separate processes check optional FD procedure absence,
initial probing without a current display, per-display capability changes,
extension token boundaries, pending/signaled/errors, independent FD ownership,
invalid context and preservation of failed proofs across representation reuse.
Real EGL accessors and managed sinks also exercise failed read/write slot
returns, sequential advancement, destructor cleanup, and preservation of the
original failure fence across accessor recreation.
The `glFinish` fallback distinguishes recoverable errors left by earlier GL
commands from new synchronization errors, rejects prior context loss or
out-of-memory, and bounds error draining when a driver repeatedly reports errors.
The simulated EGL tests do not measure driver ordering or performance.

Vulkan tests compile the real fence, image import, command pool, representation
and C API with simulated driver calls. Image import tests inject failures in
the buffer-property query, image creation, memory allocation, memory binding
and image-view creation. They check that partially created resources are
released, failed imports block access and no invalid image barrier is submitted.
They also cover nonblocking
pending/signaled/error queries, cached SYNC_FD exports, independent descriptor
ownership, EGL import/wait failures, device loss, concurrent legacy CPU waits,
and exported fences surviving command pool cleanup. Representation tests check
that failed imports/submissions retain the exact dependency, completed fences
still trigger image layout transitions, and failed PostDraw submissions never
publish an earlier successful fence.

`vulkan-device-tests` uses real Vulkan/EGL drivers and requires external
semaphore SYNC_FD export and EGL native fence support. A host-controlled Vulkan
event holds the producer pending while repeated FD exports and EGL GPU waits
return. The test checks pending status in Vulkan, the FD, and the EGL consumer;
then releases the event, verifies completion and a 256-byte GPU buffer write,
and checks that an exported FD survives destruction of the original fence.
CPU waits occur only after releasing the gate, with bounded timeouts. This
tests driver synchronization, not AHardwareBuffer image rendering or frame
performance. Both Vulkan executables have an overall watchdog.

```sh
python3 clay/gfx/shared_image/tests/build.py \
  --root /path/to/checkout --gn-out /path/to/configured/mac-output \
  --platform macosx --surface-texture-host --out /tmp/clay-metal-tests
/tmp/clay-metal-tests/metal-tests
/tmp/clay-metal-tests/representation-tests
/tmp/clay-metal-tests/optional-exports
/tmp/clay-metal-tests/blocking-tests
/tmp/clay-metal-tests/gl-tests
/tmp/clay-metal-tests/gl-contract-tests
/tmp/clay-metal-tests/surface-texture-tests
```

Metal tests use two real GPU queues, a controlled producer dependency, and a
256-byte data check for direct server wait and exported-handle import. Negative
status and reference-count inputs use small Objective-C test objects. Nil,
error and uncommitted producer buffers fail without CPU waiting. Exported
handles must outlive the original fence and balance exactly one release.
Representation tests also exercise real accessors and managed sinks: failed
reads/writes return slots while preserving the original fence, sequential reads
advance, and explicit release and destruction both clean up. Successful
handoffs use a real Metal queue. Raw C acquisition tests retain failed waits
and leave outputs unchanged across retries; sequence/latest tests return the
slot and preserve its dependency when representation creation fails. Symbol tests anchor only a base
API and check optional discovery after Apple dead stripping. GPU readback waits
occur only in tests.

Detached swap tests remove the frame callback and check rejection retains the
original fence and current slot, null-fence retries preserve that dependency,
and the same or a recreated accessor can return the slot after reattachment.

`--surface-texture-host` adds `surface-texture-tests` on macOS with a Skity GN
output; omit it to build the original suite without host EGL headers. It compiles the
complete SurfaceTexture backing and Android EGL fence/representation source
units, replacing only Android/JNI, driver and renderer boundaries with host
fixtures. EGL/GLES headers default to `third_party/angle/include`; override
with `--egl-include` when needed. It checks strict try-update never invokes a
CPU wait or `glFinish`, legacy callback-only waits, wait/delete callbacks
enqueuing dependencies through both setters, nested read/update rejection,
context changes, frame epochs, attach/update failures, and shared completion
snapshots surviving backing destruction. A watchdog bounds deadlock failures.
This is state-machine validation, not Android BufferQueue or device validation.

`blocking-tests` checks the common blocking-consumer contract: successful
access, failed waits blocking texture/framebuffer access, null dependencies
preserving failure, and failure proofs surviving slot return and recreation.
It is also built for Android and iOS.

`--platform iphoneos` checks arm64 compilation/linking; unsigned CLI output is
not a physical-device test. `--platform iphonesimulator` produces executables
runnable with `xcrun simctl spawn BOOTED_DEVICE_ID EXECUTABLE`.

Apple GL tests compile the real CGL/EAGL representations. CGL checks IOSurface
pixels, shared and unrelated consumer contexts, restoring the caller's current
context, and completion queries after representation destruction. EAGL runs
both ES2 (`GL_APPLE_sync`) and ES3 paths and releases the producer contexts
before its final query. The GL contract test injects a pending driver boundary
to verify GPU waits do not imply completion, polling returns during mutex
contention and concurrent legacy waits, destruction does not wait for pending
work, and errors remain failed. Run the real GL driver tests under a bounded external timeout. The injected
GL contract test includes a watchdog.

## Nonblocking capability contract

| Producer | Poll | GPU wait | Native export |
| --- | --- | --- | --- |
| Android EGL | Zero-timeout EGL query | Compatible EGL display with wait-sync | Native fence FD when supported |
| Android Vulkan | Vulkan fence status | EGL native-FD import plus wait-sync | Duplicated SYNC_FD payload |
| iOS/macOS Metal | Command-buffer status | Shared event on consumer command buffer | Shared event handle and timeline value |
| macOS CGL | Private shared-context query | Same GL share group | Unsupported |
| iOS EAGL | Private shared-context query; ES2 APPLE_sync or ES3 | Same GL share group | Unsupported |
| Linux libepoxy/ANGLE EGL | Zero-timeout EGL query | Same EGL implementation/display; KHR or EGL 1.5 | Native fence FD when supported |
| SurfaceTexture without EGL sync | Signaled after successful legacy completion fallback | Already complete | Signaled |

Native sync creation avoids GPU completion waits. Legacy production uses
`glFinish` if sync creation is unavailable, retaining blocking compatibility.
Apple GL retains a
private shared context per representation, shared by its outstanding fences;
queries and destruction never bind the producer context on another thread.
CGL requires core sync support or ARB_sync, EAGL requires ES3 or APPLE_sync.
Missing capabilities alone are not reported as completion; only a successful
legacy completion fallback can establish it. Vulkan GPU wait checks
both the current display's extension and the function pointer before exporting
or importing an FD. Its missing-procedure regression runs in a separate process.

These guarantees apply to the nonblocking fence operations, not every existing
image operation. Legacy `ClientWait` and representation `ConsumeFence` may
block; callers requiring nonblocking handoff must poll or enqueue an appropriate
GPU wait before accessing the image. GL-to-Metal and unrelated Apple GL share
groups cannot directly import the GL sync object: retain the backing and poll
until Signaled. SurfaceTexture's platform `updateTexImage` can still wait inside
Android; its completion snapshot is not the BufferQueue producer acquire fence.

## Linux EGL backends

Both libepoxy and ANGLE/SwiftShader GL representations produce an EGL fence
without `glFinish` when sync creation is available. Legacy production retains
a blocking completion fallback when creation is unavailable or fails, so
ClientWait compatibility does not depend on optional fence support. Poll,
ServerWait and Export never invoke that fallback. Fences retain their producer's EGL entry points, display,
and backend identity. Internal GL consumers enqueue a GPU wait only for the
same backend; the legacy consumer path otherwise performs its explicit CPU
wait. Public ServerWait requires the caller to make the producing EGL
implementation's compatible consumer context current. EGL 1.5 core sync is
used when KHR fence entry points/extensions are unavailable. KHR-created fences
also use the core GPU wait when KHR wait support is absent. Native FD export
is optional and never implied by generic EGL fence support. Explicit nonblocking creation reports missing features as Unsupported and
creation/query errors as Failed; neither is ready.
The EGL display and its library must outlive all returned fences.

CPU SHM representations continue publishing a null (already complete) fence
only after their synchronous operations return. `CopyPixelsToShm`/`glReadPixels`
and legacy ConsumeFence are not asynchronous readback APIs. Poll/ServerWait
must not be interpreted as making CPU memory available before completion.

On a Linux host with EGL/GLES development headers and libraries (Mesa's
surfaceless EGL implementation can run without a window server):

```sh
python3 clay/gfx/shared_image/tests/build_linux.py --out /tmp/linux-fence-tests
```

The script works with either repository layout, uses `c++` by default, and has
`--cxx`, `--egl-include`, and `--unit-only` options. The unit test injects only
driver entry points, covering two independent backend dispatch tables, KHR and
core-only and mixed KHR/core EGL 1.5, missing capabilities/procedures,
pending/signaled/failure, independent FDs, and context-free destruction. A
concurrent legacy CPU wait must not block polling or GPU wait submission. The device test checks real
producer-to-consumer GPU ordering, RGBA pixels, context-free queries and FD
lifetime. Both include watchdogs. It can also run on another EGL platform, but
that is not a substitute for native Linux/Mesa or ANGLE driver validation.

EGL contracts follow the [EGL 1.5 specification](https://registry.khronos.org/EGL/specs/eglspec.1.5.pdf)
and [native fence extension](https://registry.khronos.org/EGL/extensions/ANDROID/EGL_ANDROID_native_fence_sync.txt).

## Blocking compatibility

Legacy SharedImage fence production preserves producer-side completion fallback
when native sync creation is unavailable. Successful fallback returns a completed
fence, including for a later ClientWait on another thread. Actual GPU completion
errors still fail closed. Explicit nonblocking construction does not use this
fallback and reports Unsupported or Failed. Nonblocking query, GPU wait and
export APIs never call ClientWait or glFinish. Supported native fences retain
their blocking ClientWait behavior alongside the nonblocking APIs.

Concurrent ClientWait regressions hold completion pending while the same fence
is polled, server-waited and exported through the public Android EGL and Metal
APIs. Metal covers shared-event fences and command-buffer-only fences using a
real GPU gate. Android EGL injects an infinite driver wait and checks both
present and absent native-FD support. After releasing the gate, ClientWait must
succeed. Linux's injected driver also exports a native FD while ClientWait is
blocked. Watchdogs fail the tests if a nonblocking operation waits for release.
