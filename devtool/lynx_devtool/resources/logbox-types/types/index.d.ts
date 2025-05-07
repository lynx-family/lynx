// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export enum LogLevels {
    Verbose = 'verbose',
    Info = 'info',
    Warning = 'warn',
    Error = 'error',
    Fatal = 'fatal',
}

export interface IErrorProps {
    code: number;
    level?: string;
    stack?: string;
    fixSuggestion?: string;
    criticalInfo?: Object;
}

declare class StackFrame {
    functionName: string | null;
    fileName: string | null;
    lineNumber: number | null;
    columnNumber: number | null;
    
    _originalFunctionName: string | null;
    _originalFileName: string | null;
    _originalLineNumber: number | null;
    _originalColumnNumber: number | null;
    
    _scriptCode: ScriptLine[] | null;
    _originalScriptCode: ScriptLine[] | null;

    getFunctionName(): string;
    getSource(): string;
}
  
declare class ScriptLine {
    lineNumber: number;
    content: string;
    highlight: boolean;
}

export interface IErrorRecord {
    contextSize: number;
    message: string;
    errorProps: IErrorProps;
    stackFrames?: StackFrame[];
    rawErrorText?: string;
}

export interface IErrorParser {
    parse(rawData: string): Promise<IErrorRecord | null>;
}

export interface IResourceProvider {
    getResource: (name: string) => Promise<string>;
}

declare global {
    interface Window {
        logBoxCore: {
            registerErrorParser: (errNamespace: string, parser: IErrorParser) => void;
            /**
             * Enhances a set of <code>StackFrame</code>s with their original positions and code (when available).
             * @param {StackFrame[]} frames A set of <code>StackFrame</code>s which contain (generated) code positions.
             */
            map: (frames: StackFrame[], contextLines: number, resProvider: IResourceProvider) => Promise<StackFrame[]>;
            /**
             * Query resource for given url
             * @param {string} url
             */
            queryResource: (url: string) => Promise<string>;
            /**
             * Parse stack from string to StackFrame[]
             */
            parseStack: (error: any | string | string[]) => StackFrame[];
        };
    }
}

