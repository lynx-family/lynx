import { release } from 'os';
import { loadNodeLynxNativeBinding } from './headless-lynx-view';

// eslint-disable-next-line @typescript-eslint/no-require-imports
const packageJson = require('../package.json');

export type OpenCardCallback = (url: string) => void;
export type ClosePageCallback = () => void;
export type LynxLogLevel =
  | 'verbose'
  | 'debug'
  | 'info'
  | 'warning'
  | 'error'
  | 'fatal'
  | 'silent';

const LOG_LEVEL_VALUES: Record<LynxLogLevel, number> = {
  verbose: 0,
  debug: 1,
  info: 2,
  warning: 3,
  error: 4,
  fatal: 5,
  silent: 6,
};

export class LynxEnv {
  static init(): void {
    const lynx = loadNodeLynxNativeBinding();
    lynx.initGlobalEnv();
    lynx.LynxEnv.setDevtoolSwitch('enable_devtool', true);
    lynx.LynxEnv.setDevtoolSwitch('enable_quickjs_debug', true);

    // Set default clientInfo for debug router
    const defaultKeys = [
      'App',
      'deviceModel',
      'osType',
      'osVersion',
      'sdkVersion',
    ];
    const defaultValues = [
      'node-lynx',
      `nodejs-${process.version}`,
      process.platform,
      release(),
      packageJson.version ?? '0.0.0',
    ];
    lynx.LynxEnv.setAppInfo(defaultKeys, defaultValues);
  }

  static setAppInfo(
    optionKeys: Array<string>,
    optionValues: Array<string>
  ): void {
    loadNodeLynxNativeBinding().LynxEnv.setAppInfo(optionKeys, optionValues);
  }

  static setLogLevel(level: LynxLogLevel): void {
    if (!Object.prototype.hasOwnProperty.call(LOG_LEVEL_VALUES, level)) {
      throw new Error(`unsupported Lynx log level: ${String(level)}`);
    }
    loadNodeLynxNativeBinding().LynxEnv.setLogLevel(LOG_LEVEL_VALUES[level]);
  }

  static connectDevtools(schema: string): boolean {
    return loadNodeLynxNativeBinding().LynxEnv.connectDevtools(schema);
  }

  static setOpenCardCallback(callback?: OpenCardCallback | null): void {
    loadNodeLynxNativeBinding().LynxEnv.setOpenCardCallback(callback);
  }

  static clearOpenCardCallback(): void {
    loadNodeLynxNativeBinding().LynxEnv.setOpenCardCallback(null);
  }

  static setClosePageCallback(callback?: ClosePageCallback | null): void {
    loadNodeLynxNativeBinding().LynxEnv.setClosePageCallback(callback);
  }

  static clearClosePageCallback(): void {
    loadNodeLynxNativeBinding().LynxEnv.setClosePageCallback(null);
  }
}
