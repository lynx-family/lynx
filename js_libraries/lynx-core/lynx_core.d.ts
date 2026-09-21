export declare function loadCard(
  nativeApp: any,
  config: {
    initData?: unknown;
    entryName?: string;
    initConfig?: object;
    cardType: string;
  },
  lynx: any,
): void;

export declare function destroyCard(id: string, pageGlobal?: typeof globalThis): void;

export declare function callDestroyLifetimeFun(id: string, pageGlobal?: typeof globalThis): void;

export declare const nativeGlobal: any;

export declare function loadDynamicComponent<T>(tt: any, componentUrl: string): T;
