// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements, TransferViewProps } from '../../types';

let value: unknown;
assertType<string>(value as IntrinsicElements['transfer-view']['transfer-id']);
assertType<TransferViewProps>({ 'transfer-id': 'external-host' });

<transfer-view transfer-id="external-host" id="content" style={{ width: '100px' }} />;

<transfer-view
  transfer-id="external-host"
  bindtap={(event) => {
    assertType<string>(event.type);
  }}
/>;

// @ts-expect-error transfer-id is required.
<transfer-view />;

// @ts-expect-error transfer-id must be a string.
<transfer-view transfer-id={123} />;

// @ts-expect-error transfer-id cannot be undefined.
assertType<TransferViewProps>({ 'transfer-id': undefined });
