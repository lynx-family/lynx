/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/* @flow */
import React, { Component } from 'react';
import StackFrame from './StackFrame';
import Collapsible from '../components/Collapsible';
import { isInternalFile } from '@/utils/isInternalFile';
import { isBuiltinErrorName } from '@/utils/isBuiltinErrorName';

const traceStyle = {
  fontSize: '1em',
  flex: '0 1 auto',
  minHeight: '0px',
  overflow: 'auto',
};

class StackTrace extends Component<any> {
  renderFrames() {
    const { stackFrames, errorName, contextSize, editorHandler } = this.props;
    const renderedFrames = [];
    let hasReachedAppCode = false,
      currentBundle = [],
      bundleCount = 0;

    stackFrames &&
      stackFrames.forEach((frame, index) => {
        const { fileName, _originalFileName: sourceFileName } = frame;
        const isInternalUrl = isInternalFile(sourceFileName, fileName);
        const isThrownIntentionally = !isBuiltinErrorName(errorName);
        const shouldCollapse = isInternalUrl && (isThrownIntentionally || hasReachedAppCode);

        if (!isInternalUrl) {
          hasReachedAppCode = true;
        }

        const frameEle = (
          <StackFrame
            // eslint-disable-next-line
            key={'frame-' + index}
            frame={frame}
            contextSize={contextSize}
            critical={index === 0}
            showCode={!shouldCollapse}
            editorHandler={editorHandler}
          />
        );
        const lastElement = index === stackFrames.length - 1;

        if (shouldCollapse) {
          currentBundle.push(frameEle);
        }

        if (!shouldCollapse || lastElement) {
          if (currentBundle.length === 1) {
            renderedFrames.push(currentBundle[0]);
          } else if (currentBundle.length > 1) {
            // eslint-disable-next-line
            bundleCount++;
            renderedFrames.push(<Collapsible key={'bundle-' + bundleCount}>{currentBundle}</Collapsible>);
          }
          currentBundle = [];
        }

        if (!shouldCollapse) {
          renderedFrames.push(frameEle);
        }
      });

    return renderedFrames;
  }

  render() {
    return <div style={traceStyle}>{this.renderFrames()}</div>;
  }
}

export default StackTrace;
