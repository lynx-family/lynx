// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useState } from '@lynx-js/react';

import HomePage from '@components/homepage';
import Navigator from '@components/navigator';
import SettingsPage from '@components/settingspage';

interface ContainerPageState {
  showHomePage: boolean;
  showSettingsPage: boolean;
  themes: string[];
  currentTheme: string;
}

export default function Explorer() {
  const [state, setState] = useState<ContainerPageState>({
    showHomePage: true,
    showSettingsPage: false,
    themes: ['Auto', 'Light', 'Dark'],
    currentTheme: lynx.__globalProps.preferredTheme || 'Auto',
  });

  const openHomePage = () => {
    if (state.showHomePage) {
      return;
    }
    setState((prev) => ({
      ...prev,
      showHomePage: true,
      showSettingsPage: false,
    }));
  };

  const openSettingsPage = () => {
    if (state.showSettingsPage) {
      return;
    }
    setState((prev) => ({
      ...prev,
      showHomePage: false,
      showSettingsPage: true,
    }));
  };

  const setTheme = (theme: string) => {
    if (state.currentTheme === theme) {
      return;
    }

    setState((prev) => ({
      ...prev,
      currentTheme: theme,
    }));

    NativeModules.ExplorerModule.saveThemePreferences('preferredTheme', theme);
  };

  const withTheme = (className: string) => {
    const { currentTheme } = state;
    if (currentTheme !== 'Auto') {
      return `${className}__${currentTheme.toLowerCase()}`;
    }
    return `${className}__${lynx.__globalProps.theme.toLowerCase()}`;
  };

  const withNotchScreen = (className: string) => {
    return lynx.__globalProps.isNotchScreen ? `${className}__notch` : className;
  };

  return (
    <view clip-radius="true" style={{ height: '100%' }}>
      <HomePage
        currentTheme={state.currentTheme}
        withNotchScreen={withNotchScreen}
        withTheme={withTheme}
        showPage={state.showHomePage}
      />
      <SettingsPage
        themes={state.themes}
        currentTheme={state.currentTheme}
        withNotchScreen={withNotchScreen}
        withTheme={withTheme}
        setTheme={setTheme}
        showPage={state.showSettingsPage}
      />
      <Navigator
        {...state}
        withTheme={withTheme}
        openHomePage={openHomePage}
        openSettingsPage={openSettingsPage}
      />
    </view>
  );
}

root.render(<Explorer />);
