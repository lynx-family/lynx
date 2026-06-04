// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { BaseMethod, Callback } from '../events';
import { ContentEditable } from './contenteditable';

/**
 * Native Markdown Component
 */
export interface MarkdownProps extends StandardProps {
  /**
   * Enables editing for supported Markdown source. In v1, only `true` and
   * empty string are editable; `false`, missing, and unsupported values are
   * not editable.
   * @defaultValue false
   * @Android
   */
  contenteditable?: ContentEditable;
}

interface ApplySourcePatchMethod extends BaseMethod {
  method: 'applySourcePatch';
  params: {
    /**
     * Source character offset where the replacement starts.
     */
    start: number;
    /**
     * Source character offset where the replacement ends.
     */
    end: number;
    /**
     * Markdown source inserted for the [start, end) range.
     */
    replacement: string;
  };
  success?: Callback<{
    value: string;
    selectionStart: number;
    selectionEnd: number;
    applied: boolean;
    fallback: boolean;
    error?: string;
  }>;
}

export type MarkdownUIMethods = ApplySourcePatchMethod;
