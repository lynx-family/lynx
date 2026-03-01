// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useState } from '@lynx-js/react';
import { AppContextProvider } from '@explorer/lib';

import HomePage from '@components/homepage';
import Navigator from '@components/navigator';
import SettingsPage from '@components/settingspage';

export default function Explorer() {
  const [page, setPage] = useState<'home' | 'settings'>('home');

  return (
    <AppContextProvider>
      <view clip-radius="true" style={{ height: '100%' }}>
        <HomePage showPage={page === 'home'} />
        <SettingsPage showPage={page === 'settings'} />
        <Navigator activePage={page} onNavigate={setPage} />
      </view>
    </AppContextProvider>
  );
}

root.render(<Explorer />);
