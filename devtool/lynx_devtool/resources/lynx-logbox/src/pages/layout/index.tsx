// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import styles from './index.less';
import React from 'react';

export interface IRedBoxLayoutProps {
  footer: React.ReactElement;
  header: React.ReactElement;
  content: React.ReactElement;
}

export function Layout(props: IRedBoxLayoutProps): JSX.Element {
  return (
    <div className={styles.container}>
      {props.header}
      {props.content}
      <div className={styles.expand}></div>
      {props.footer}
    </div>
  );
}
