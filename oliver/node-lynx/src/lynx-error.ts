export const LynxErrorLevel = {
  Fatal: 'fatal',
  Error: 'error',
  Warn: 'warn',
  Undecided: 'undecided',
} as const;

export type LynxErrorLevel = typeof LynxErrorLevel[keyof typeof LynxErrorLevel];

export const LynxErrorType = {
  NativeError: -1,
  JSError: -2,
  PlatformError: -3,
} as const;

export type LynxErrorType = typeof LynxErrorType[keyof typeof LynxErrorType];

export interface LynxError {
  errorLevel: LynxErrorLevel;
  errorCode: number;
  message: string;
  fixSuggestion: string;
  errorType: LynxErrorType;
  customInfo: Record<string, unknown>;
}
