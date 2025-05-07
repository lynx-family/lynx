import { registerErrorParser } from '@/parser';
import { map } from '@/utils/mapper';
import parseStack from '@/utils/stackParser';
import { getBridge } from '@/jsbridge';
import { IErrorParser, IResourceProvider } from '@lynx-dev/logbox-types';
import type { StackFrame } from '@lynx-dev/logbox-types';

export function initGlobalAPIs() {
  window.logBoxCore = {
    registerErrorParser: (errNamespace: string, parser: IErrorParser) => {
      registerErrorParser(errNamespace, parser);
    },
    map: (frames: StackFrame[], contextLines: number, resProvider: IResourceProvider): Promise<StackFrame[]> => {
      return map(frames, contextLines, resProvider);
    },
    queryResource: (url: string): Promise<string> => {
      return getBridge().queryResource(url);
    },
    parseStack: (error: any | string | string[]): StackFrame[] => {
      return parseStack(error);
    },
  };
}
