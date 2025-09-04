// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxGroup {
  public static SINGLE_GROUP: string = "-1";
  private static sNextID: number = 0;
  public preloadJSPaths: string[] = [];
  // On HarmonyOS, we recommend using the system's built-in JSVM engine.
  // Therefore, enableDynamicV8 and enableHarmonyJSVM have the same effect, and
  // the HarmonyOS JSVM will ultimately be used as the JS Runtime.
  public enableDynamicV8: boolean = false;
  public enableHarmonyJSVM: boolean = false;
  public enableJSGroupThread: boolean = false;
  private id: string;

  // id used for copy.
  private constructor(isSingle: boolean, id?: string) {
    if (id) {
      this.id = id;
    } else {
      this.id = isSingle ? LynxGroup.SINGLE_GROUP : LynxGroup.generateID();
    }
  }

  public static copy(other?: LynxGroup): LynxGroup | undefined {
    let group: LynxGroup | undefined = undefined;
    if (other) {
      group = new LynxGroup(false, other.id);
      group.preloadJSPaths = other.preloadJSPaths;
      group.enableDynamicV8 = other.enableDynamicV8;
      group.enableHarmonyJSVM = other.enableHarmonyJSVM;
      group.enableJSGroupThread = other.enableJSGroupThread;
    }
    return group;
  }

  static createSingleLynxGroup(): LynxGroup {
    return new LynxGroup(true);
  }

  static createSharedLynxGroup(): LynxGroup {
    return new LynxGroup(false);
  }

  private static generateID() {
    let id: string = "" + LynxGroup.sNextID;
    LynxGroup.sNextID++;
    return id;
  }

  public isSingleGroup(): boolean {
    return this.id == LynxGroup.SINGLE_GROUP;
  }

  public getGroupID(): string {
    return this.id;
  }
}
