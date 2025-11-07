// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxDevToolServiceType } from './ILynxDevToolService';
import { LynxEventReporterServiceType } from './ILynxEventReporterService';
import { LynxExtensionServiceType } from './ILynxExtensionService';
import { LynxHttpServiceType } from './ILynxHttpService';
import { LynxSecurityServiceType } from './ILynxSecurityService';
import { LynxTrailServiceType } from './ILynxTrailService';

export class LynxServiceType {
  static Trail = LynxTrailServiceType;
  static EventReporter = LynxEventReporterServiceType;
  static Http = LynxHttpServiceType;
  static Security = LynxSecurityServiceType;
  static Extension = LynxExtensionServiceType;
  static DevTool = LynxDevToolServiceType;
}
