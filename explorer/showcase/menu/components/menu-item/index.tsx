// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './index.scss';

export interface ItemProps {
  title: string;
  description: string;
  url: string;
  icon?: string;
}

export function withTheme(className: string): string {
  return lynx.__globalProps.frontendTheme == 'dark'
    ? `${className}__dark`
    : className;
}

const theme = lynx.__globalProps.frontendTheme;

function withQuery(item: ItemProps): string {
  const title = item.title;
  const titleColor = theme == 'dark' ? 'FFFFFF' : '000000';
  const barColor = theme == 'dark' ? '181D25' : 'F0F2F5';
  const backButtonStyle = theme;

  return `title=${title}&title_color=${titleColor}&bar_color=${barColor}&back_button_style=${backButtonStyle}`;
}

export function MenuItem(props: ItemProps) {
  const openCard = (url: string) => {
    NativeModules.ExplorerModule.openSchema(`${url}?${withQuery(props)}`);
  };

  const onClick = (item) => {
    openCard(item.url);
  };

  return (
    <view className={withTheme('box')} bindtap={onClick.bind(this, this.props)}>
      {(() => {
        if (props.icon && props.icon[theme] != undefined) {
          return <image src={props.icon[theme]} className="icon" />;
        }
        return (
          <view className={withTheme('circle')}>
            <text className={withTheme('placeholder-text')}>
              {props.title[0]}
            </text>
          </view>
        );
      })()}
      <view className="content">
        <text className={withTheme('title')}>{props.title}</text>
        <text className={withTheme('text')}>{props.description}</text>
      </view>
    </view>
  );
}
