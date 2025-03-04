/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/* @flow */
import { SourceMapConsumer } from 'source-map';
import { getBridge } from '@/jsbridge';

/**
 * A wrapped instance of a <code>{@link https://github.com/mozilla/source-map SourceMapConsumer}</code>.
 *
 * This exposes methods which will be indifferent to changes made in <code>{@link https://github.com/mozilla/source-map source-map}</code>.
 */
class SourceMap {
  __source_map: SourceMapConsumer;

  // $FlowFixMe
  constructor(sourceMap: SourceMapConsumer) {
    this.__source_map = sourceMap;
  }

  /**
   * Returns the original code position for a generated code position.
   * @param {number} line The line of the generated code position.
   * @param {number} column The column of the generated code position.
   */
  getOriginalPosition(line: number, column: number): { source: string | null; line: number | null; column: number | null } {
    const { line: l, column: c, source: s } = this.__source_map.originalPositionFor({
      line,
      column,
    });
    return { line: l, column: c, source: s };
  }

  /**
   * Returns the generated code position for an original position.
   * @param {string} source The source file of the original code position.
   * @param {number} line The line of the original code position.
   * @param {number} column The column of the original code position.
   */
  getGeneratedPosition(source: string, line: number, column: number): { line: number | null; column: number | null } {
    const { line: l, column: c } = this.__source_map.generatedPositionFor({
      source,
      line,
      column,
    });
    return {
      line: l,
      column: c,
    };
  }

  /**
   * Returns the code for a given source file name.
   * @param {string} sourceName The name of the source file.
   */
  getSource(sourceName: string): string | null {
    return this.__source_map.sourceContentFor(sourceName);
  }

  // getSources(): string[] {
  //   return this.__source_map.sources;
  // }
}

function extractSourceMapUrl(fileUri: string, fileContents: string): string {
  const regex = /\/\/[#@] ?sourceMappingURL=([^\s'"]+)\s*$/gm;
  let match: RegExpExecArray | null = null;
  for (;;) {
    let next = regex.exec(fileContents);
    if (next == null) {
      break;
    }
    match = next;
  }
  if (!(match && match[1])) {
    // return Promise.reject(`Cannot find a source map directive for ${fileUri}.`);
    return '';
  }
  // return Promise.resolve(match[1].toString());
  return match[1].toString();
}

/**
 * Returns an instance of <code>{@link SourceMap}</code> for a given fileUri and fileContents.
 * @param {string} fileUri The URI of the source file.
 * @param {string} fileContents The contents of the source file.
 */
async function getSourceMap(fileUri: string, fileContents: string): Promise<SourceMap> {
  // SourceMapConsumer.initialize({
  //   'lib/mappings.wasm': getMappingsWasm(),
  // });
  let sm: any = extractSourceMapUrl(fileUri, fileContents);
  if (sm.indexOf('data:') === 0) {
    const base64 = /^data:application\/json;([\w=:"-]+;)*base64,/;
    const match2 = sm.match(base64);
    if (!match2) {
      throw new Error('Sorry, non-base64 inline source-map encoding is not supported.');
    }
    sm = sm.substring(match2[0].length);
    sm = window.atob(sm);
    sm = JSON.parse(sm);
    return new SourceMap(await new SourceMapConsumer(sm));
  } else {
    const obj = await getBridge().queryResource(sm);
    if (obj) {
      sm = JSON.parse(obj);
    } else {
      sm = {};
    }
    return new SourceMap(await new SourceMapConsumer(sm));
  }
}

export { extractSourceMapUrl, getSourceMap };
export default getSourceMap;
