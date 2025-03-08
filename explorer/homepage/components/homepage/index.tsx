// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Component } from '@lynx-js/react';
import './index.scss';

import ExplorerIcon from '@assets/images/explorer.png?inline';
import ExplorerIconDark from '@assets/images/explorer-dark.png?inline';
import ForwardIcon from '@assets/images/forward.png?inline';
import ForwardIconDark from '@assets/images/forward-dark.png?inline';
import ScanIcon from '@assets/images/scan.png?inline';
import ScanIconDark from '@assets/images/scan-dark.png?inline';
import ShowcaseIcon from '@assets/images/showcase.png?inline';

interface HomePageProps {
  showPage: boolean;
  currentTheme: string;
  withTheme: (className: string) => string;
  withNotchScreen: (className: string) => string;
}

export default class HomePage extends Component<HomePageProps> {
  icons = {
    Scan: {
      Dark: ScanIconDark,
      Light: ScanIcon,
    },
    Forward: {
      Dark: ForwardIconDark,
      Light: ForwardIcon,
    },
    Explorer: {
      Dark: ExplorerIconDark,
      Light: ExplorerIcon,
    },
  };

  inputValue: string = '';

  constructor(props: HomePageProps) {
    super(props);
  }

  openScan = () => {
    NativeModules.ExplorerModule.openScan();
  };

  openSchema = () => {
    NativeModules.ExplorerModule.openSchema(this.inputValue);
  };

  openShowcasePage = () => {
    const theme =
      this.props.currentTheme == 'Auto'
        ? lynx.__globalProps.theme
        : this.props.currentTheme;
    const titleColor = theme == 'Dark' ? 'FFFFFF' : '000000';
    const barColor = theme == 'Dark' ? '181D25' : 'F0F2F5';
    const backButtonStyle = theme.toLowerCase();

    const query = `title=Showcase&title_color=${titleColor}&bar_color=${barColor}&back_button_style=${backButtonStyle}`;
    NativeModules.ExplorerModule.openSchema(
      `file://lynx?local://showcase/menu/main.lynx.bundle?${query}`
    );
  };

  handleInput = (event: any) => {
    const currentValue = event.detail.value.trim();
    this.inputValue = currentValue;
  };

  icon(name: string) {
    const { currentTheme } = this.props;
    if (currentTheme !== 'Auto') {
      return this.icons[name][currentTheme];
    }
    return this.icons[name][lynx.__globalProps.theme];
  }

  textColor() {
    const { currentTheme } = this.props;
    if (currentTheme !== 'Auto') {
      return currentTheme == 'Dark' ? 'FFFFFF' : '000000';
    }
    return lynx.__globalProps.theme == 'Dark' ? 'FFFFFF' : '000000';
  }

  render() {
    const { showPage, withTheme, withNotchScreen } = this.props;
    if (!showPage) {
      return <></>;
    }

    return (
      <view clip-radius="true" className={withTheme('page')}>
        <view className={withNotchScreen('page-header')}>
          <image
            src={this.icon('Explorer')}
            className="logo"
            mode="aspectFit"
          />
          <text className={withTheme('home-title')}>Lynx Explorer</text>
          <view className="scan">
            {(() => {
              if (lynx.__globalProps.platform == 'iOS') {
                return <></>;
              }
              return (
                <image
                  src={this.icon('Scan')}
                  className="scan-icon"
                  bindtap={this.openScan}
                />
              );
            })()}
          </view>
        </view>

        <view className={withTheme('input-card-url')}>
          <text className={withTheme('bold-text')}>Card URL</text>
          <input
            className="input-box"
            bindinput={this.handleInput}
            placeholder="Enter Card URL"
            text-color={this.textColor()}
          />
          <view
            className={withTheme('connect-button')}
            bindtap={this.openSchema}
          >
            <text style="line-height: 22px; color: #ffffff; font-size: 16px">
              Go
            </text>
          </view>
        </view>

        <view className={withTheme('showcase')} bindtap={this.openShowcasePage}>
          <image src={ShowcaseIcon} className="showcase-icon" />
          <text className={withTheme('text')}>Showcase</text>
          <view style="margin: auto 5% auto auto; justify-content: center">
            <image src={this.icon('Forward')} className="forward-icon" />
          </view>
        </view>
      </view>
    );
  }
}
