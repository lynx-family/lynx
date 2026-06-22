export interface LynxNapiAddonLoaderNativeModule {
  requireNodeAddon: (env: number[], addonName: string) => void;
  putEnvByToken: (tokenId: number, env: number[]) => void;
  removeEnvByToken: (tokenId: number) => void;
  requireNodeAddonByToken: (tokenId: number, addonName: string) => void;
}

declare const nativeModule: LynxNapiAddonLoaderNativeModule;
export default nativeModule;
