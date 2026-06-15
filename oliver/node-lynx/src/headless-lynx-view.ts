import * as fs from 'fs';
import * as http from 'http';
import * as https from 'https';
import * as path from 'path';
import * as zlib from 'zlib';

export type HeadlessRenderer = 'software';

export interface HeadlessLynxViewOptions {
  width?: number;
  height?: number;
  devicePixelRatio?: number;
  timeoutMs?: number;
  renderer?: HeadlessRenderer;
  resourcesPath?: string;
  onErrorOccurred?: NativeErrorHandler;
}

export interface LoadTemplateOptions {
  url?: string;
  initialData?: object | string;
  globalProps?: object | string;
  processor?: string;
  readOnly?: boolean;
  enableRecycleTemplateBundle?: boolean;
}

export interface UpdateDataOptions {
  globalProps?: object | string;
  processor?: string;
  readOnly?: boolean;
}

export interface ScreenshotOptions {
  settleMs?: number;
}

export type NativeFrame = {
  width: number;
  height: number;
  data: Buffer;
};

type NativeHeadlessLynxView = {
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
  _sendTouchEvent(eventName: string, tag: number): void;
  _setErrorHandler(handler?: NativeErrorHandler): void;
  _waitForFrame(): Promise<void>;
  _forceFrame(): void;
  _captureFrame(): NativeFrame;
  _destroy(): void;
};

export type NativeErrorHandler = (
  errorLevel: string,
  errorCode: number,
  message: string,
  fixSuggestion: string,
  customInfo: Record<string, string>,
  logboxOnly: boolean
) => void;

export type NativeBinding = {
  HeadlessLynxViewNative: new (
    resourcesPath: string,
    width: number,
    height: number,
    devicePixelRatio: number,
    resourceFetcher: ResourceFetcher
  ) => NativeHeadlessLynxView;
  initGlobalEnv(): void;
  LynxEnv: {
    connectDevtools(schema: string): boolean;
    setAppInfo(optionKeys: string[], optionValues: string[]): void;
    setClosePageCallback(callback?: (() => void) | null): void;
    setDevtoolSwitch(key: string, value: boolean): void;
    setOpenCardCallback(callback?: ((url: string) => void) | null): void;
  };
};

export type ResourceFetcher = (
  url: string,
  type: number,
  callback: (error: Error | null, data: Buffer | null) => void
) => void;

const DEFAULT_WIDTH = 390;
const DEFAULT_HEIGHT = 844;
const DEFAULT_DPR = 2;
const DEFAULT_TIMEOUT_MS = 10000;
const DEFAULT_SCREENSHOT_SETTLE_MS = 100;

const PNG_SIGNATURE = Buffer.from([
  0x89,
  0x50,
  0x4e,
  0x47,
  0x0d,
  0x0a,
  0x1a,
  0x0a,
]);

let crcTable: number[] | undefined;

function makeCrcTable(): number[] {
  if (crcTable) {
    return crcTable;
  }
  crcTable = [];
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    crcTable[n] = c >>> 0;
  }
  return crcTable;
}

function crc32(buffer: Buffer): number {
  const table = makeCrcTable();
  let c = 0xffffffff;
  for (const byte of buffer) {
    c = table[(c ^ byte) & 0xff] ^ (c >>> 8);
  }
  return (c ^ 0xffffffff) >>> 0;
}

function pngChunk(type: string, data: Buffer): Buffer {
  const typeBuffer = Buffer.from(type, 'ascii');
  const length = Buffer.allocUnsafe(4);
  length.writeUInt32BE(data.length, 0);
  const crc = Buffer.allocUnsafe(4);
  crc.writeUInt32BE(crc32(Buffer.concat([typeBuffer, data])), 0);
  return Buffer.concat([length, typeBuffer, data, crc]);
}

export function encodeRgbaPng(
  width: number,
  height: number,
  rgba: Buffer
): Buffer {
  const expectedLength = width * height * 4;
  if (rgba.length !== expectedLength) {
    throw new Error(
      `invalid frame size: expected ${expectedLength}, got ${rgba.length}`
    );
  }

  const pngHeader = Buffer.allocUnsafe(13);
  pngHeader.writeUInt32BE(width, 0);
  pngHeader.writeUInt32BE(height, 4);
  pngHeader[8] = 8;
  pngHeader[9] = 6;
  pngHeader[10] = 0;
  pngHeader[11] = 0;
  pngHeader[12] = 0;

  const stride = width * 4;
  const pngData = Buffer.allocUnsafe((stride + 1) * height);
  for (let y = 0; y < height; y++) {
    const outOffset = y * (stride + 1);
    pngData[outOffset] = 0;
    for (let x = 0; x < width; x++) {
      const sourceOffset = y * stride + x * 4;
      const targetOffset = outOffset + 1 + x * 4;
      const alpha = rgba[sourceOffset + 3];
      if (alpha === 255) {
        pngData[targetOffset] = rgba[sourceOffset];
        pngData[targetOffset + 1] = rgba[sourceOffset + 1];
        pngData[targetOffset + 2] = rgba[sourceOffset + 2];
      } else {
        const inverseAlpha = 255 - alpha;
        pngData[targetOffset] = Math.round(
          (rgba[sourceOffset] * alpha + 255 * inverseAlpha) / 255
        );
        pngData[targetOffset + 1] = Math.round(
          (rgba[sourceOffset + 1] * alpha + 255 * inverseAlpha) / 255
        );
        pngData[targetOffset + 2] = Math.round(
          (rgba[sourceOffset + 2] * alpha + 255 * inverseAlpha) / 255
        );
      }
      pngData[targetOffset + 3] = 255;
    }
  }

  return Buffer.concat([
    PNG_SIGNATURE,
    pngChunk('IHDR', pngHeader),
    pngChunk('IDAT', zlib.deflateSync(pngData)),
    pngChunk('IEND', Buffer.alloc(0)),
  ]);
}

export function normalizeJson(value: object | string | undefined): string {
  if (value === undefined) {
    return '';
  }
  return typeof value === 'string' ? value : JSON.stringify(value);
}

export function normalizeBuffer(
  input: ArrayBuffer | Uint8Array | Buffer
): Buffer {
  if (Buffer.isBuffer(input)) {
    return input;
  }
  if (input instanceof Uint8Array) {
    return Buffer.from(input.buffer, input.byteOffset, input.byteLength);
  }
  return Buffer.from(input);
}

export function withTimeout<T>(
  promise: Promise<T>,
  timeoutMs: number,
  action: string
): Promise<T> {
  let timer: NodeJS.Timeout | undefined;
  const timeout = new Promise<never>((_, reject) => {
    timer = setTimeout(
      () => reject(new Error(`${action} timed out after ${timeoutMs}ms`)),
      timeoutMs
    );
  });
  return Promise.race([promise, timeout]).finally(() => {
    if (timer) {
      clearTimeout(timer);
    }
  });
}

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

function resolvePackageRoot(): string {
  return path.resolve(__dirname, '..');
}

function resolvePackageName(): string {
  const override = process.env.LYNX_NODE_LYNX_NATIVE_PACKAGE_NAME;
  if (override) {
    return override;
  }
  const packageJsonPath = path.join(resolvePackageRoot(), 'package.json');
  try {
    const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf8'));
    if (typeof packageJson.name === 'string' && packageJson.name.length > 0) {
      return packageJson.name;
    }
  } catch {
    // Fall through to the public package name when running from a partial build tree.
  }
  return '@lynx-js/node-lynx';
}

function resolveNativeAddonPathOverride(): string | undefined {
  const override = process.env.LYNX_NODE_LYNX_NATIVE_ADDON_PATH;
  return override ? path.resolve(override) : undefined;
}

export function defaultResourcesPath(): string {
  const override = process.env.LYNX_NODE_LYNX_RESOURCES_PATH;
  if (override) {
    return path.resolve(override);
  }
  return path.join(resolvePackageRoot(), 'resources');
}

export function loadNodeLynxNativeBinding(): NativeBinding {
  const nativeAddonPath = resolveNativeAddonPathOverride();
  if (nativeAddonPath) {
    try {
      return require(nativeAddonPath);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      throw new Error(
        `Unable to load native addon from LYNX_NODE_LYNX_NATIVE_ADDON_PATH=${nativeAddonPath}: ${message}`
      );
    }
  }

  const packageName = `${resolvePackageName()}-${process.platform}-${
    process.arch
  }`;
  try {
    return require(packageName);
  } catch (packageError) {
    const packageRoot = resolvePackageRoot();
    const localPaths = [
      path.join(
        packageRoot,
        'platform',
        `${process.platform}-${process.arch}`,
        'node_lynx.node'
      ),
      path.join(
        packageRoot,
        'build',
        process.platform,
        'Release',
        'node_lynx.node'
      ),
      path.join(
        packageRoot,
        'build',
        process.platform,
        'Debug',
        'node_lynx.node'
      ),
    ];
    let localError: unknown;
    for (const localPath of localPaths) {
      if (!fs.existsSync(localPath)) {
        continue;
      }
      try {
        return require(localPath);
      } catch (error) {
        localError = error;
      }
    }
    if (localError) {
      throw localError;
    }
    throw packageError;
  }
}

function readHttpUrl(url: string, redirects = 5): Promise<Buffer> {
  return new Promise((resolve, reject) => {
    let completed = false;
    const resolveOnce = (value: Buffer): void => {
      if (completed) {
        return;
      }
      completed = true;
      resolve(value);
    };
    const rejectOnce = (error: unknown): void => {
      if (completed) {
        return;
      }
      completed = true;
      reject(error instanceof Error ? error : new Error(String(error)));
    };

    const client = url.startsWith('https:') ? https : http;
    const request = client.get(url, (response) => {
      response.on('error', rejectOnce);
      const statusCode = response.statusCode ?? 0;
      const location = response.headers.location;
      if (statusCode >= 300 && statusCode < 400 && location && redirects > 0) {
        response.resume();
        const next = new URL(location, url).toString();
        readHttpUrl(next, redirects - 1).then(resolveOnce, rejectOnce);
        return;
      }
      if (statusCode < 200 || statusCode >= 300) {
        response.resume();
        rejectOnce(
          new Error(`request failed with status ${statusCode}: ${url}`)
        );
        return;
      }
      const chunks: Buffer[] = [];
      response.on('data', (chunk) =>
        chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk))
      );
      response.on('end', () => resolveOnce(Buffer.concat(chunks)));
    });
    request.on('error', rejectOnce);
  });
}

export async function fetchBuffer(url: string): Promise<Buffer> {
  if (url.startsWith('file://')) {
    return fs.promises.readFile(new URL(url));
  }
  if (path.isAbsolute(url)) {
    return fs.promises.readFile(url);
  }
  if (url.startsWith('http://') || url.startsWith('https://')) {
    if (typeof fetch === 'function') {
      const response = await fetch(url);
      if (!response.ok) {
        throw new Error(
          `request failed with status ${response.status}: ${url}`
        );
      }
      return Buffer.from(await response.arrayBuffer());
    }
    return readHttpUrl(url);
  }
  throw new Error(`unsupported resource url: ${url}`);
}

export const defaultResourceFetcher: ResourceFetcher = (
  url,
  _type,
  callback
) => {
  fetchBuffer(url)
    .then((buffer) => callback(null, buffer))
    .catch((error) =>
      callback(error instanceof Error ? error : new Error(String(error)), null)
    );
};

export class HeadlessLynxView {
  private readonly native: NativeHeadlessLynxView;
  private readonly timeoutMs: number;
  private destroyed = false;

  constructor(options: HeadlessLynxViewOptions = {}) {
    const renderer = options.renderer ?? 'software';
    if (renderer !== 'software') {
      throw new Error(`unsupported headless renderer: ${renderer}`);
    }
    const width = options.width ?? DEFAULT_WIDTH;
    const height = options.height ?? DEFAULT_HEIGHT;
    const devicePixelRatio = options.devicePixelRatio ?? DEFAULT_DPR;
    if (width <= 0 || height <= 0 || devicePixelRatio <= 0) {
      throw new Error('width, height, and devicePixelRatio must be positive');
    }

    const binding = loadNodeLynxNativeBinding();
    this.timeoutMs = options.timeoutMs ?? DEFAULT_TIMEOUT_MS;
    this.native = new binding.HeadlessLynxViewNative(
      options.resourcesPath ?? defaultResourcesPath(),
      width,
      height,
      devicePixelRatio,
      defaultResourceFetcher
    );
    if (options.onErrorOccurred) {
      this.native._setErrorHandler(options.onErrorOccurred);
    }
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

  sendTouchEvent(eventName: string, tag: number): void {
    this.assertAlive();
    this.native._sendTouchEvent(eventName, tag);
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
      throw new Error('HeadlessLynxView has been destroyed');
    }
  }
}

export default HeadlessLynxView;
