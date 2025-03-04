// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import StackFrame from '@/common/stackFrame';

export enum LogLevels {
  Verbose = 'verbose',
  Info = 'info',
  Warning = 'warn',
  Error = 'error',
  Fatal = 'fatal',
}

export interface IErrorProps {
  code: number;
  level?: string;
  stack?: string;
  fixSuggestion?: string;
  criticalInfo?: Object;
}

export interface IErrorRecord {
  contextSize: number;
  message: string;
  errorProps: IErrorProps;
  stackFrames?: StackFrame[];
  rawErrorText?: string;
}
