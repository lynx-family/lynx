// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxExtraTiming {
  public static readonly OPEN_TIME = 'openTime';
  public static readonly CONTAINER_INIT_START = 'containerInitStart';
  public static readonly CONTAINER_INIT_END = 'containerInitEnd';
  public static readonly PREPARE_TEMPLATE_START = 'prepareTemplateStart';
  public static readonly PREPARE_TEMPLATE_END = 'prepareTemplateEnd';
  public openTime: number = 0;
  public containerInitStart: number = 0;
  public containerInitEnd: number = 0;
  public prepareTemplateStart: number = 0;
  public prepareTemplateEnd: number = 0;
}