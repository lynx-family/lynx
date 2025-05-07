/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/* @flow */
import { IResourceProvider } from '@lynx-dev/logbox-types';
import StackFrame from '../common/stackFrame';
import { getSourceMap } from './getSourceMap';
import { getLinesAround } from './getLinesAround';
import { settle } from 'settle-promise';

function getFilesInFrames(frames: StackFrame[]): string[] {
  let files: string[] = [];
  frames.forEach((frame) => {
    const { fileName } = frame;
    if (fileName == null) {
      return;
    }
    if (files.indexOf(fileName) !== -1) {
      return;
    }
    files.push(fileName);
  });
  return files;
}

/**
 * Enhances a set of <code>StackFrame</code>s with their original positions and code (when available).
 * @param {StackFrame[]} frames A set of <code>StackFrame</code>s which contain (generated) code positions.
 * @param {number} [contextLines=3] The number of lines to provide before and after the line specified in the <code>StackFrame</code>.
 */
export async function map(frames: StackFrame[], contextLines: number, resProvider: IResourceProvider): Promise<StackFrame[]> {
  const cache: any = {};
  const files = getFilesInFrames(frames);
  await settle(
    files.map(async (fileName) => {
      const fileSource = await resProvider.getResource(fileName);
      const map = await getSourceMap(fileName, fileSource ?? '');
      cache[fileName] = { fileSource, map };
    }),
  );
  return frames.map((frame) => {
    const { functionName, fileName, lineNumber, columnNumber } = frame;
    if (fileName == null) {
      return frame;
    }
    let { map, fileSource } = cache[fileName] || {};
    if (map == null || lineNumber == null) {
      return frame;
    }
    const { source, line, column } = map.getOriginalPosition(lineNumber, columnNumber);
    if (line == null) {
      console.warn('failed to parse the stack frame, please check if the sourcemap is valid');
    }
    const originalSource = source == null ? [] : map.getSource(source);
    return new StackFrame(
      functionName,
      fileName,
      lineNumber,
      columnNumber,
      getLinesAround(lineNumber, contextLines, fileSource),
      functionName,
      source,
      line,
      column,
      getLinesAround(line, contextLines, originalSource),
    );
  });
}
