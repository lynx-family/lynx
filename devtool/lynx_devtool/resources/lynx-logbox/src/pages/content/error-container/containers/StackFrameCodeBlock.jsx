/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { codeFrameColumns } from '@babel/code-frame';
import React, { useContext } from 'react';
import { ThemeContext } from '..';
import CodeBlock from '../components/CodeBlock';
import { absolutifyCaret } from '@/utils/dom/absolutifyCaret';
import generateAnsiHTML from '@/utils/generateAnsiHTML';

function StackFrameCodeBlock(props) {
  const theme = useContext(ThemeContext);
  const { lines, lineNum, columnNum, contextSize, main } = props;
  const sourceCode = [];
  let whiteSpace = Infinity;
  lines.forEach(function (e) {
    const { content: text } = e;
    const m = text.match(/^\s*/);
    if (text === '') {
      return;
    }
    if (m && m[0]) {
      whiteSpace = Math.min(whiteSpace, m[0].length);
    } else {
      whiteSpace = 0;
    }
  });
  lines.forEach(function (e) {
    let { content: text } = e;
    const { lineNumber: line } = e;

    if (isFinite(whiteSpace)) {
      text = text.substring(whiteSpace);
    }
    sourceCode[line - 1] = text;
  });
  const ansiHighlight = codeFrameColumns(
    sourceCode.join('\n'),
    {
      start: {
        line: lineNum,
        column: columnNum == null ? 0 : columnNum - (isFinite(whiteSpace) ? whiteSpace : 0),
      },
    },
    {
      forceColor: true,
      linesAbove: contextSize,
      linesBelow: contextSize,
    },
  );
  const htmlHighlight = generateAnsiHTML(ansiHighlight, theme);
  const code = document.createElement('code');
  code.innerHTML = htmlHighlight;
  absolutifyCaret(code);

  const ccn = code.childNodes;
  // eslint-disable-next-line
  oLoop: for (let index = 0; index < ccn.length; ++index) {
    const node = ccn[index];
    const ccn2 = node.childNodes;
    for (let index2 = 0; index2 < ccn2.length; ++index2) {
      const lineNode: any = ccn2[index2];
      const text = lineNode.innerText;
      if (text == null) {
        continue;
      }
      // eslint-disable-next-line
      if (text.indexOf(' ' + lineNum + ' |') === -1) {
        continue;
      }
      // eslint-disable-next-line
      break oLoop;
    }
  }

  return <CodeBlock main={main} codeHTML={code.innerHTML} />;
}

export default StackFrameCodeBlock;
