// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './index.scss';
import { openSchema, useTheme } from '@explorer/lib';

export interface ItemProps {
  title: string;
  description: string;
  url: string;
  icon?: string;
}

function withQuery(item: ItemProps, resolved: string): string {
  const title = item.title;
  const titleColor = resolved == 'dark' ? 'FFFFFF' : '000000';
  const barColor = resolved == 'dark' ? '181D25' : 'F0F2F5';
  const backButtonStyle = resolved;

  return `title=${title}&title_color=${titleColor}&bar_color=${barColor}&back_button_style=${backButtonStyle}`;
}

export function MenuItem(props: ItemProps) {
  const { resolved, withTheme } = useTheme();

  const openCard = (url: string) => {
    openSchema(`${url}?${withQuery(props, resolved)}`);
  };

  const onClick = (item) => {
    openCard(item.url);
  };

  return (
    <view
      className={withTheme('box')}
      bindtap={onClick.bind(this, this.props)}
      accessibility-element={true}
      accessibility-label={`{Open ${props.title} Show Cases}`}
      accessibility-traits="button"
    >
      {(() => {
        if (props.icon && props.icon[resolved] != undefined) {
          return <image src={props.icon[resolved]} className="icon" />;
        }
        return (
          <view className={withTheme('circle')}>
            <text
              className={withTheme('placeholder-text')}
              accessibility-element={false}
            >
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
