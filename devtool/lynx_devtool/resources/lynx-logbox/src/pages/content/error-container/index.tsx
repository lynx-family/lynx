/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
import React, { createContext } from 'react';
import RuntimeErrorContainer from './containers/RuntimeErrorContainer';
import { lightTheme } from './styles';

export const ThemeContext = createContext();
export function ErrorContainer({ currentRuntimeErrorRecords, dismissRuntimeErrors }) {
  if (currentRuntimeErrorRecords.length > 0) {
    return (
      <ThemeContext.Provider value={lightTheme}>
        <RuntimeErrorContainer errorRecords={currentRuntimeErrorRecords} close={dismissRuntimeErrors} />
      </ThemeContext.Provider>
    );
  }
  return null;
}
