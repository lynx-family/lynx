/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

const lightTheme = {
  // Colors for components styles
  background: 'white',
  color: 'black',
  headerColor: '#ce1126',
  primaryPreBackground: 'rgba(206, 17, 38, 0.05)',
  primaryPreColor: 'inherit',
  secondaryPreBackground: 'rgba(251, 245, 180, 0.3)',
  secondaryPreColor: 'inherit',
  footer: '#878e91',
  anchorColor: '#878e91',
  toggleBackground: 'transparent',
  toggleColor: '#878e91',
  closeColor: '#293238',
  navBackground: 'rgba(206, 17, 38, 0.05)',
  navArrow: '#ce1126',
  // Light color scheme inspired by https://chriskempson.github.io/base16/css/base16-github.css
  // base00: '#ffffff',
  base01: '#f5f5f5',
  // base02: '#c8c8fa',
  base03: '#6e6e6e',
  // base04: '#e8e8e8',
  base05: '#333333',
  // base06: '#ffffff',
  // base07: '#ffffff',
  base08: '#881280',
  // base09: '#0086b3',
  // base0A: '#795da3',
  base0B: '#1155cc',
  base0C: '#994500',
  // base0D: '#795da3',
  base0E: '#c80000',
  // base0F: '#333333',
};

const darkTheme = {
  // Colors for components styles
  background: '#353535',
  color: 'white',
  headerColor: '#e83b46',
  primaryPreBackground: 'rgba(206, 17, 38, 0.1)',
  primaryPreColor: '#fccfcf',
  secondaryPreBackground: 'rgba(251, 245, 180, 0.1)',
  secondaryPreColor: '#fbf5b4',
  footer: '#878e91',
  anchorColor: '#878e91',
  toggleBackground: 'transparent',
  toggleColor: '#878e91',
  closeColor: '#ffffff',
  navBackground: 'rgba(206, 17, 38, 0.2)',
  navArrow: '#ce1126',
  // Dark color scheme inspired by https://github.com/atom/base16-tomorrow-dark-theme/blob/master/styles/colors.less
  // base00: '#1d1f21',
  base01: '#282a2e',
  // base02: '#373b41',
  base03: '#969896',
  // base04: '#b4b7b4',
  base05: '#c5c8c6',
  // base06: '#e0e0e0',
  // base07: '#ffffff',
  base08: '#cc6666',
  // base09: '#de935f',
  // base0A: '#f0c674',
  base0B: '#b5bd68',
  base0C: '#8abeb7',
  // base0D: '#81a2be',
  base0E: '#b294bb',
  // base0F: '#a3685a',
};

const iframeStyle = {
  position: 'fixed',
  top: '0',
  left: '0',
  width: '100%',
  height: '100%',
  border: 'none',
  'z-index': 2147483647,
};

const overlayStyle = (theme) => ({
  width: '100%',
  height: '100%',
  'box-sizing': 'border-box',
  'text-align': 'center',
  'background-color': theme.background,
});

export { iframeStyle, overlayStyle, lightTheme, darkTheme };
