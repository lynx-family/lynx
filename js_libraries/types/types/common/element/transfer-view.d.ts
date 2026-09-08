// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

/**
 * A container whose children can be mounted in an external native host while
 * retaining their logical position in the Lynx UI tree.
 * @Android
 * @iOS
 */
export interface TransferViewProps extends StandardProps {
  /**
   * Identifies the transfer for native host listeners. A listener can claim the
   * transfer and mount its content in an external host. Changing this identifier
   * removes the previous transfer and requests a new one. An empty identifier
   * does not request a transfer.
   * @Android
   * @iOS
   */
  'transfer-id': string;
}
