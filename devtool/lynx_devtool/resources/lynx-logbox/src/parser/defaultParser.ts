// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IErrorParser, DEFAULT_CONTEXT_SIZE } from './base';
import { IErrorProps, IErrorRecord } from '@/common/interface';

const PREFIX_FOR_CRITICAL_INFO_KEY = 'lynx_context_';

interface ICriticalInfo {
  [key: string]: string;
}

export class DefaultErrorParser implements IErrorParser {
  async parse(rawError: any): Promise<IErrorRecord | null> {
    const errorProps: IErrorProps = {
      code: rawError.sub_code,
      level: rawError.level,
      stack: rawError.error_stack,
      fixSuggestion: rawError.fix_suggestion,
    };
    const criticalInfo: ICriticalInfo = {};
    if (rawError.root_cause) {
      criticalInfo['root_cause'] = rawError.root_cause;
    }
    if (rawError.context) {
      Object.keys(rawError.context).forEach((key) => {
        const realKey = key.slice(PREFIX_FOR_CRITICAL_INFO_KEY.length, key.length);
        criticalInfo[realKey] = rawError.context[key];
      });
    }
    errorProps.criticalInfo = criticalInfo;
    return {
      message: rawError.error,
      contextSize: DEFAULT_CONTEXT_SIZE,
      errorProps,
    };
  }
}
