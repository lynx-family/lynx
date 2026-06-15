import {
  HeadlessLynxView,
  HeadlessLynxViewOptions,
  ScreenshotOptions,
} from './headless-lynx-view';
import { LynxError, LynxErrorLevel, LynxErrorType } from './lynx-error';
import { LynxViewClient } from './lynx-view-client';

export enum LynxTouchEvent {
  START = 'touchstart',
  MOVE = 'touchmove',
  UP = 'touchend',
  CANCEL = 'touchcancel',
  TAP = 'tap',
  LONGPRESS = 'longpress',
}

function toBuffer(input: ArrayBuffer | Uint8Array | Buffer): Buffer {
  if (Buffer.isBuffer(input)) {
    return input;
  }
  if (input instanceof Uint8Array) {
    return Buffer.from(input.buffer, input.byteOffset, input.byteLength);
  }
  return Buffer.from(input);
}

function normalizeJson(
  value: object | string | undefined
): object | string | undefined {
  return value;
}

function parseObject(
  value: object | string | undefined
): Record<string, unknown> | undefined {
  if (value === undefined) {
    return undefined;
  }
  if (typeof value === 'string') {
    try {
      const parsed = JSON.parse(value);
      return parsed && typeof parsed === 'object' && !Array.isArray(parsed)
        ? parsed
        : undefined;
    } catch {
      return undefined;
    }
  }
  return value && typeof value === 'object' && !Array.isArray(value)
    ? (value as Record<string, unknown>)
    : undefined;
}

export class LynxTemplateRenderer {
  private readonly view: HeadlessLynxView;
  private readonly viewClients: LynxViewClient[] = [];
  private globalProps: object | string | undefined;

  constructor(options: HeadlessLynxViewOptions = {}) {
    this.view = new HeadlessLynxView({
      width: options.width ?? 720,
      height: options.height ?? 1280,
      devicePixelRatio: options.devicePixelRatio ?? 1,
      timeoutMs: options.timeoutMs,
      renderer: options.renderer,
      resourcesPath: options.resourcesPath,
      onErrorOccurred: (
        errorLevel,
        errorCode,
        message,
        fixSuggestion,
        customInfo,
        logboxOnly
      ) =>
        this.onErrorOccurred(
          errorLevel,
          errorCode,
          message,
          fixSuggestion,
          customInfo,
          logboxOnly
        ),
    });
  }

  public nativeAttach(_resourcesPath: string, _provider: unknown): void {
    // Kept for source compatibility. The embedder headless view is attached in the constructor.
  }

  public loadTemplate(
    url: string,
    buffer: ArrayBuffer | Uint8Array | Buffer,
    processor?: string,
    templateData?: object | string,
    readOnly?: boolean,
    enableRecycleTemplateBundle?: boolean
  ): void {
    this.view
      .loadTemplate(toBuffer(buffer), {
        url,
        initialData: normalizeJson(templateData),
        globalProps: normalizeJson(this.globalProps),
        processor,
        readOnly,
        enableRecycleTemplateBundle,
      })
      .catch((error) => this.emitLocalError(error));
  }

  public invokeCDPFromSDK(cdpMsg: string): Promise<string> {
    return this.view.invokeCDPFromSDK(cdpMsg);
  }

  public sendTouchEvent(eventName: LynxTouchEvent, tag: number): void {
    this.view.sendTouchEvent(eventName, tag);
  }

  public updateGlobalProps(props?: object | string): void {
    const previous = parseObject(this.globalProps);
    const next = parseObject(props);
    this.globalProps = previous && next ? { ...previous, ...next } : props;
    if (this.globalProps !== undefined) {
      this.view.updateGlobalProps(this.globalProps);
    }
  }

  public updateMetaData(
    processor?: string,
    data?: object | string,
    readOnly?: boolean,
    props?: object | string
  ): void {
    if (props !== undefined) {
      this.updateGlobalProps(props);
    }
    if (data !== undefined) {
      this.view.updateData(data, {
        globalProps: normalizeJson(this.globalProps),
        processor,
        readOnly,
      });
    }
  }

  public waitForFrame(timeoutMs?: number): Promise<void> {
    return this.view.waitForFrame(timeoutMs);
  }

  public screenshot(options: ScreenshotOptions = {}): Promise<Buffer> {
    return this.view.screenshot(options);
  }

  public destroy(): void {
    this.view.destroy();
  }

  public addLynxViewClient(client: LynxViewClient): void {
    this.viewClients.push(client);
  }

  public removeLynxViewClient(client: LynxViewClient): void {
    const index = this.viewClients.indexOf(client);
    if (index !== -1) {
      this.viewClients.splice(index, 1);
    }
  }

  public onErrorOccurred(
    errorLevel: string,
    errorCode: number,
    message: string,
    fixSuggestion: string,
    customInfo: Record<string, unknown>,
    logboxOnly: boolean
  ): void {
    if (logboxOnly || this.viewClients.length === 0) {
      return;
    }

    let errLevel: LynxErrorLevel;
    switch (errorLevel) {
      case LynxErrorLevel.Fatal:
        errLevel = LynxErrorLevel.Fatal;
        break;
      case LynxErrorLevel.Error:
        errLevel = LynxErrorLevel.Error;
        break;
      case LynxErrorLevel.Warn:
        errLevel = LynxErrorLevel.Warn;
        break;
      case LynxErrorLevel.Undecided:
        errLevel = LynxErrorLevel.Undecided;
        break;
      default:
        errLevel = LynxErrorLevel.Undecided;
        break;
    }
    this.viewClients.forEach((client) => {
      client.onReceivedError({
        errorLevel: errLevel,
        errorCode,
        message,
        fixSuggestion,
        errorType: LynxErrorType.NativeError,
        customInfo,
      } as LynxError);
    });
  }

  private emitLocalError(error: unknown): void {
    this.onErrorOccurred(
      LynxErrorLevel.Error,
      -1,
      error instanceof Error ? error.message : String(error),
      '',
      {},
      false
    );
  }
}
