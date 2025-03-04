// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Provider } from 'react-redux';
import { getAppStore } from '@/models/store';
import { DisplayError } from './content/displayError';
import { Layout } from './layout';
import { Footer } from './footer';
import { Header } from './header';
import React, { useEffect } from 'react';
import { getBridge } from '@/jsbridge';

function IndexPage(): JSX.Element {
  useEffect(() => {
    const fetchData = async () => {
      await getBridge().getErrorData();
    };
    fetchData();
  }, []);

  return (
    <Provider store={getAppStore()}>
      <Layout content={<DisplayError />} header={<Header />} footer={<Footer />} />
    </Provider>
  );
}

export default IndexPage;
