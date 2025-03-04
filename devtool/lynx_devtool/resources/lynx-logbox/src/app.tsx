// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './global.css';
import '@/utils/vconsole';
import { getBridgeReadyPromise, getBridge } from '@/jsbridge';
import React from 'react';
import { createRoot } from 'react-dom/client';
import IndexPage from '@/pages';

getBridgeReadyPromise();
getBridge().addEventListeners();

const root = createRoot(document.getElementById('root'));
root.render(<IndexPage></IndexPage>);
