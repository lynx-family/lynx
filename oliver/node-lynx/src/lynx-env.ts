import { loadNodeLynxNativeBinding } from './headless-lynx-view';

export type OpenCardCallback = (url: string) => void;
export type ClosePageCallback = () => void;

export class LynxEnv {
  static init(): void {
    const lynx = loadNodeLynxNativeBinding();
    lynx.initGlobalEnv();
    lynx.LynxEnv.setDevtoolSwitch('enable_devtool', true);
    lynx.LynxEnv.setDevtoolSwitch('enable_quickjs_debug', true);
  }

  static setAppInfo(
    optionKeys: Array<string>,
    optionValues: Array<string>
  ): void {
    loadNodeLynxNativeBinding().LynxEnv.setAppInfo(optionKeys, optionValues);
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
