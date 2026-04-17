// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Harmony *intentionally* keeps DevToolLifecycle as a thin facade over the native
// state machine instead of mirroring the state in ArkTS.
//
// Android has a Java mirror because Java can observe DevTool events while
// lynx.so is not loaded yet, and JNI reads/sync failures should not leak into
// every Java caller. The Java mirror is therefore a practical platform cache
// with best-effort native synchronization.
// iOS takes the opposite shape: ObjC and C++ are compiled together and calling
// the native lifecycle directly is cheap and reliable, so no platform mirror
// is needed.
//
// Harmony sits between those two platforms. The @lynx/lynx ArkTS module already
// depends on liblynx.so for core Lynx APIs, and DevToolLifecycle is registered
// by that same native library. There is no real "platform present, native
// lifecycle absent" problem today. If we also keep lifecycle state in ArkTS,
// we would need extra code to keep ArkTS and native state consistent.
//
// By Occam's Razor, this wrapper stays closer to iOS for now and avoids a
// platform mirror until Harmony has a concrete need for one.
//
// If Harmony later grows ArkTS-first lifecycle events that must be readable
// without native calls, or a real native-to-ArkTS sync requirement, prefer using
// DevToolLifecycle::Delegate in the native wrapper to add an explicit mirror.
// Until then, native DevToolLifecycle remains the single source of truth.
import lynx from 'liblynx.so';

export enum DevToolState {
  UNAVAILABLE = 0,
  ATTACHED = 1,
  ENABLED = 2,
  INITIALIZED = 3,
  CONNECTED = 4,
}

export class DevToolLifecycle {
  public static getState(): DevToolState {
    return lynx.DevToolLifecycleHarmony.getState() as DevToolState;
  }

  public static isAttached(): boolean {
    return lynx.DevToolLifecycleHarmony.isAttached();
  }

  public static isInitialized(): boolean {
    return lynx.DevToolLifecycleHarmony.isInitialized();
  }

  public static isEnabled(): boolean {
    return lynx.DevToolLifecycleHarmony.isEnabled();
  }

  public static isConnected(): boolean {
    return lynx.DevToolLifecycleHarmony.isConnected();
  }

  public static onAttached(): void {
    lynx.DevToolLifecycleHarmony.onAttached();
  }

  public static onEnabled(): void {
    lynx.DevToolLifecycleHarmony.onEnabled();
  }

  public static onDisabled(): void {
    lynx.DevToolLifecycleHarmony.onDisabled();
  }

  public static onInitialized(): void {
    lynx.DevToolLifecycleHarmony.onInitialized();
  }

  public static onConnected(): void {
    lynx.DevToolLifecycleHarmony.onConnected();
  }

  public static onDisconnected(): void {
    lynx.DevToolLifecycleHarmony.onDisconnected();
  }
}
