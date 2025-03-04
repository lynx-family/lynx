/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import React, { useContext, useEffect } from 'react';
import { ThemeContext } from '..';
const overlayStyle = (theme) => ({
  position: 'relative',
  display: 'inline-flex',
  flexDirection: 'column',
  width: '1024px',
  maxWidth: '100%',
  overflowX: 'hidden',
  overflowY: 'auto',
  padding: '0.5rem',
  boxSizing: 'border-box',
  textAlign: 'left',
  fontFamily: 'Consolas, Menlo, monospace',
  fontSize: '11px',
  whiteSpace: 'pre-wrap',
  wordBreak: 'break-word',
  lineHeight: 1.5,
  color: theme.color,
});

let iframeWindow = null;

function ErrorOverlay(props) {
  const theme = useContext(ThemeContext);

  const getIframeWindow = (element) => {
    if (element) {
      const document = element.ownerDocument;
      iframeWindow = document.defaultView;
    }
  };
  const { shortcutHandler } = props;

  useEffect(() => {
    const onKeyDown = (e) => {
      if (shortcutHandler) {
        shortcutHandler(e.key);
      }
    };
    window.addEventListener('keydown', onKeyDown);
    if (iframeWindow) {
      iframeWindow.addEventListener('keydown', onKeyDown);
    }
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      if (iframeWindow) {
        iframeWindow.removeEventListener('keydown', onKeyDown);
      }
    };
  }, [shortcutHandler]);

  return (
    <div style={overlayStyle(theme)} ref={getIframeWindow}>
      {props.children}
    </div>
  );
}

export default ErrorOverlay;
