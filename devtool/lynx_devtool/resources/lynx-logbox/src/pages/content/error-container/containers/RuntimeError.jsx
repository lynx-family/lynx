/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import React from 'react';
import { UnControlled as CodeMirror } from 'react-codemirror2';
import { useState } from 'react';
import StackTrace from './StackTrace';
import 'codemirror/lib/codemirror.css';
// eslint-disable-next-line
require('codemirror/mode/clike/clike');

const wrapperStyle = {
  display: 'flex',
  flexDirection: 'column',
};

function RuntimeError({ errorRecord, editorHandler }) {
  const [showRawErrorStack, setShowRawErrorStack] = useState(false);
  const { error, contextSize, stackFrames } = errorRecord;

  return (
    <div style={wrapperStyle}>
      <StackTrace stackFrames={stackFrames} errorName={error.name} contextSize={contextSize} editorHandler={editorHandler} />
      <div
        onClick={() => {
          setShowRawErrorStack(!showRawErrorStack);
        }}
        style={{
          color: '#9e9e9e',
          background: '#e6e6e6',
          textAlign: 'center',
          border: '1px solid #d4d4d4',
          borderRadius: '5px',
          height: '28px',
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
        }}
      >
        Show/Hide Raw Stack
      </div>
      {showRawErrorStack ? (
        <CodeMirror
          autoScroll={false}
          value={error.stack}
          options={{
            mode: 'clike',
            theme: 'default',
            lineNumbers: false,
            readOnly: true,
          }}
        />
      ) : null}
    </div>
  );
}

export default RuntimeError;
