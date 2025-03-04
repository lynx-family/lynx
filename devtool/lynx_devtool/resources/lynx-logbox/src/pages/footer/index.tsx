// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import styles from './index.less';
import React from 'react';
export function Footer(): JSX.Element {
  return (
    <>
      <div className={styles['power-container']}>
        <img className={styles.icon} src="./LynxIcon.svg"></img>
        <span> Lynx </span>
      </div>
    </>
  );
}
