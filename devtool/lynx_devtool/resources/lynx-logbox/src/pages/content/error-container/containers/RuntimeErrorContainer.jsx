/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
import React, { PureComponent } from 'react';
import ErrorOverlay from '../components/ErrorOverlay';
import RuntimeError from './RuntimeError';

class RuntimeErrorContainer extends PureComponent {
  constructor(props) {
    super(props);
    this.state = {
      currentIndex: 0,
    };

    this.previous = () => {
      this.setState((state, props) => ({
        currentIndex: state.currentIndex > 0 ? state.currentIndex - 1 : props.errorRecords.length - 1,
      }));
    };

    this.next = () => {
      this.setState((state, props) => ({
        // eslint-disable-next-line
        currentIndex: state.currentIndex < props.errorRecords.length - 1 ? state.currentIndex + 1 : 0,
      }));
    };

    this.shortcutHandler = (key) => {
      if (key === 'Escape') {
        this.props.close();
      } else if (key === 'ArrowLeft') {
        this.previous();
      } else if (key === 'ArrowRight') {
        this.next();
      }
    };
  }

  render() {
    const { errorRecords } = this.props;
    const totalErrors = errorRecords.length;
    return (
      <ErrorOverlay shortcutHandler={this.shortcutHandler}>
        <RuntimeError errorRecord={errorRecords[this.state.currentIndex]} editorHandler={this.props.editorHandler} />
      </ErrorOverlay>
    );
  }
}

export default RuntimeErrorContainer;
