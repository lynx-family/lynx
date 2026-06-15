import {
  NativeFrame,
  LoadTemplateOptions,
  NativeBinding,
  NativeErrorHandler,
  ResourceFetcher,
  ScreenshotOptions,
  UpdateDataOptions,
  defaultResourceFetcher,
  defaultResourcesPath,
  encodeRgbaPng,
  fetchBuffer,
  loadNodeLynxNativeBinding,
  normalizeBuffer,
  normalizeJson,
  withTimeout,
} from './headless-lynx-view';

export interface WindowedLynxViewOptions {
  width?: number;
  height?: number;
  devicePixelRatio?: number;
  timeoutMs?: number;
  resourcesPath?: string;
  title?: string;
  resizable?: boolean;
  visible?: boolean;
  onErrorOccurred?: NativeErrorHandler;
}

export type WindowedKeyboardKey =
  | 'Backspace'
  | 'Delete'
  | 'Enter'
  | 'ArrowLeft'
  | 'ArrowRight'
  | 'ArrowUp'
  | 'ArrowDown';

type NativeWindowedLynxView = {
  _loadTemplate(
    url: string,
    buffer: Buffer,
    initialData: string,
    globalProps: string,
    processor: string,
    readOnly: boolean,
    enableRecycleTemplateBundle: boolean
  ): Promise<void>;
  _updateData(
    data: string,
    globalProps: string,
    processor: string,
    readOnly: boolean
  ): void;
  _updateGlobalProps(globalProps: string): void;
  _invokeCDPFromSDK(cdpMsg: string): Promise<string>;
  _setErrorHandler(handler?: NativeErrorHandler): void;
  _waitForFrame(): Promise<void>;
  _forceFrame(): void;
  _captureFrame(): NativeFrame;
  _show(): void;
  _close(): void;
  _waitUntilClosed(): Promise<void>;
  _click(x: number, y: number): void;
  _typeText(text: string): void;
  _pressKey(key: WindowedKeyboardKey): void;
  _destroy(): void;
};

type WindowedNativeBinding = NativeBinding & {
  WindowedLynxViewNative: new (
    resourcesPath: string,
    width: number,
    height: number,
    devicePixelRatio: number,
    resourceFetcher: ResourceFetcher,
    title: string,
    resizable: boolean,
    visible: boolean
  ) => NativeWindowedLynxView;
};

const DEFAULT_WIDTH = 390;
const DEFAULT_HEIGHT = 844;
const DEFAULT_DPR = 2;
const DEFAULT_TIMEOUT_MS = 10000;
const DEFAULT_SCREENSHOT_SETTLE_MS = 100;

function delay(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function resolveScreenshotSettleMs(settleMs: number | undefined): number {
  const resolved =
    settleMs === undefined ? DEFAULT_SCREENSHOT_SETTLE_MS : settleMs;
  if (!Number.isFinite(resolved) || resolved < 0) {
    throw new Error('settleMs must be a non-negative number');
  }
  return resolved;
}

export class WindowedLynxView {
  private readonly native: NativeWindowedLynxView;
  private readonly timeoutMs: number;
  private destroyed = false;

  constructor(options: WindowedLynxViewOptions = {}) {
    if (process.platform !== 'darwin') {
      throw new Error('WindowedLynxView is only supported on macOS');
    }

    const width = options.width ?? DEFAULT_WIDTH;
    const height = options.height ?? DEFAULT_HEIGHT;
    const devicePixelRatio = options.devicePixelRatio ?? DEFAULT_DPR;
    if (width <= 0 || height <= 0 || devicePixelRatio <= 0) {
      throw new Error('width, height, and devicePixelRatio must be positive');
    }

    const binding = loadNodeLynxNativeBinding() as WindowedNativeBinding;
    if (!binding.WindowedLynxViewNative) {
      throw new Error('native binding does not expose WindowedLynxViewNative');
    }

    this.timeoutMs = options.timeoutMs ?? DEFAULT_TIMEOUT_MS;
    this.native = new binding.WindowedLynxViewNative(
      options.resourcesPath ?? defaultResourcesPath(),
      width,
      height,
      devicePixelRatio,
      defaultResourceFetcher,
      options.title ?? 'Node Lynx',
      options.resizable ?? true,
      options.visible ?? true
    );
    if (options.onErrorOccurred) {
      this.native._setErrorHandler(options.onErrorOccurred);
    }
    if (options.visible !== false) {
      this.show();
    }
  }

  show(): void {
    this.assertAlive();
    this.native._show();
  }

  close(): void {
    this.assertAlive();
    this.native._close();
  }

  async waitUntilClosed(timeoutMs?: number): Promise<void> {
    this.assertAlive();
    const promise = this.native._waitUntilClosed();
    if (timeoutMs === undefined) {
      await promise;
      return;
    }
    await withTimeout(promise, timeoutMs, 'waitUntilClosed');
  }

  click(x: number, y: number): void {
    this.assertAlive();
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      throw new Error('click coordinates must be finite numbers');
    }
    this.native._click(x, y);
  }

  typeText(text: string): void {
    this.assertAlive();
    if (typeof text !== 'string') {
      throw new Error('typeText requires text');
    }
    if (text.length === 0) {
      return;
    }
    this.native._typeText(text);
  }

  pressKey(key: WindowedKeyboardKey): void {
    this.assertAlive();
    const supportedKeys = new Set<WindowedKeyboardKey>([
      'Backspace',
      'Delete',
      'Enter',
      'ArrowLeft',
      'ArrowRight',
      'ArrowUp',
      'ArrowDown',
    ]);
    if (!supportedKeys.has(key)) {
      throw new Error(`unsupported key: ${key}`);
    }
    this.native._pressKey(key);
  }

  async loadTemplate(
    input: ArrayBuffer | Uint8Array | Buffer,
    options: LoadTemplateOptions = {}
  ): Promise<void> {
    this.assertAlive();
    await withTimeout(
      this.native._loadTemplate(
        options.url ?? '',
        normalizeBuffer(input),
        normalizeJson(options.initialData),
        normalizeJson(options.globalProps),
        options.processor ?? '',
        options.readOnly ?? false,
        options.enableRecycleTemplateBundle ?? false
      ),
      this.timeoutMs,
      'loadTemplate'
    );
  }

  async loadTemplateFromUrl(
    url: string,
    options: Omit<LoadTemplateOptions, 'url'> = {}
  ): Promise<void> {
    const buffer = await withTimeout(
      fetchBuffer(url),
      this.timeoutMs,
      `download ${url}`
    );
    await this.loadTemplate(buffer, { ...options, url });
  }

  updateData(data: object | string, options: UpdateDataOptions = {}): void {
    this.assertAlive();
    this.native._updateData(
      normalizeJson(data),
      normalizeJson(options.globalProps),
      options.processor ?? '',
      options.readOnly ?? false
    );
  }

  updateGlobalProps(globalProps: object | string): void {
    this.assertAlive();
    this.native._updateGlobalProps(normalizeJson(globalProps));
  }

  invokeCDPFromSDK(cdpMsg: string): Promise<string> {
    this.assertAlive();
    return withTimeout(
      this.native._invokeCDPFromSDK(cdpMsg),
      this.timeoutMs,
      'invokeCDPFromSDK'
    );
  }

  async waitForFrame(timeoutMs = this.timeoutMs): Promise<void> {
    this.assertAlive();
    await withTimeout(this.native._waitForFrame(), timeoutMs, 'waitForFrame');
  }

  async flushFrame(settleMs = DEFAULT_SCREENSHOT_SETTLE_MS): Promise<void> {
    this.assertAlive();
    settleMs = resolveScreenshotSettleMs(settleMs);
    this.native._forceFrame();
    await delay(settleMs);
  }

  async screenshot(options: ScreenshotOptions = {}): Promise<Buffer> {
    this.assertAlive();
    const settleMs = resolveScreenshotSettleMs(options.settleMs);
    await this.waitForFrame();
    await this.flushFrame(settleMs);
    const frame = this.native._captureFrame();
    return encodeRgbaPng(frame.width, frame.height, frame.data);
  }

  destroy(): void {
    if (this.destroyed) {
      return;
    }
    this.destroyed = true;
    this.native._destroy();
  }

  private assertAlive(): void {
    if (this.destroyed) {
      throw new Error('WindowedLynxView has been destroyed');
    }
  }
}

export default WindowedLynxView;
