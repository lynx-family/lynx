// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Component, root } from '@lynx-js/react';

import HomePage from '@components/homepage';
import Navigator from '@components/navigator';
import SettingsPage from '@components/settingspage';
import ImageGenerator from '@components/ImageGenerator'; // 1. Import

interface ContainerPageState {
  showHomePage: boolean;
  showSettingsPage: boolean;
  showImageGeneratorPage: boolean; // 2. Add to state interface

  themes: string[];
  currentTheme: string;
}

export default class Explorer extends Component<unknown, ContainerPageState> {
  constructor() {
    super(null);
    this.state = {
      showHomePage: true,
      showSettingsPage: false,
      showImageGeneratorPage: false, // 2. Initialize in state
      themes: ['Auto', 'Light', 'Dark'],
      currentTheme: lynx.__globalProps.preferredTheme || 'Auto',
    };
  }

  openHomePage = () => {
    if (this.state.showHomePage) {
      return;
    }
    this.setState({
      showHomePage: true,
      showSettingsPage: false,
      showImageGeneratorPage: false, // Ensure others are false
    });
  };

  openSettingsPage = () => {
    if (this.state.showSettingsPage) {
      return;
    }
    this.setState({
      showHomePage: false,
      showSettingsPage: true,
      showImageGeneratorPage: false, // Ensure others are false
    });
  };

  // 3. Add new toggle function
  openImageGeneratorPage = () => {
    if (this.state.showImageGeneratorPage) {
      return;
    }
    this.setState({
      showHomePage: false,
      showSettingsPage: false,
      showImageGeneratorPage: true,
    });
  };

  setTheme = (theme: string) => {
    if (this.state.currentTheme == theme) {
      return;
    }

    this.setState({
      currentTheme: theme,
    });

    NativeModules.ExplorerModule.saveThemePreferences('preferredTheme', theme);
  };

  withTheme = (className: string) => {
    const { currentTheme } = this.state;
    if (currentTheme !== 'Auto') {
      return `${className}__${currentTheme.toLowerCase()}`;
    }
    return `${className}__${lynx.__globalProps.theme.toLowerCase()}`;
  };

  withNotchScreen = (className: string) => {
    return lynx.__globalProps.isNotchScreen ? `${className}__notch` : className;
  };

  render() {
    return (
      <view clip-radius="true" style="height: 100%">
        <HomePage
          currentTheme={this.state.currentTheme}
          withNotchScreen={this.withNotchScreen}
          withTheme={this.withTheme}
          showPage={this.state.showHomePage}
        />
        <SettingsPage
          themes={this.state.themes}
          currentTheme={this.state.currentTheme}
          withNotchScreen={this.withNotchScreen}
          withTheme={this.withTheme}
          setTheme={this.setTheme}
          showPage={this.state.showSettingsPage}
        />
        {/* 5. Render ImageGenerator */}
        {this.state.showImageGeneratorPage && (
          <ImageGenerator />
        )}
        <Navigator
          {...this.state} // showImageGeneratorPage will be passed via spread
          withTheme={this.withTheme}
          openHomePage={this.openHomePage}
          openSettingsPage={this.openSettingsPage}
          openImageGeneratorPage={this.openImageGeneratorPage} // 4. Pass toggle function
        />
      </view>
    );
  }
}

root.render(<Explorer />);
