import EventEmitter from '../event';

interface StreamDelegate {
  onData(data: ArrayBuffer): void;
  onEnd(): void;
  onError(error: string): void;
}

type PromiseProvider =
  | PromiseConstructor
  | {
      getPromise: () => PromiseConstructor;
    };
/**
 * Serves as a stable type identifier across different Promise constructor environments
 *
 * This class is used to ensure type recognition works when same-class instances come from
 * different Promise constructor environments (e.g. different lynx instances)
 */
export abstract class LynxReadableStream {}

function resolvePromiseCtor(provider: PromiseProvider): PromiseConstructor {
  if (typeof provider === 'function') {
    return provider;
  }
  return provider.getPromise();
}

export function createReadableStreamClass(provider: PromiseProvider): any {
  return class ReadableStream
    extends LynxReadableStream
    implements StreamDelegate {
    private __Promise: PromiseConstructor;
    private __eventCenter: EventEmitter;
    private __dataReceived: ArrayBuffer[];
    private __done: boolean;
    private __cancelled: boolean;
    private __locked: boolean;
    private __error: Error;
    constructor() {
      super();
      this.__Promise = resolvePromiseCtor(provider);
      this.__dataReceived = [];
      this.__done = false;
      this.__cancelled = false;
      this.__locked = false;
      this.__eventCenter = new EventEmitter();
    }
    onData(data: ArrayBuffer): void {
      if (this.__cancelled) {
        return;
      }
      this.__dataReceived.push(data);
      this.__eventCenter.emit('waitSignal', null);
    }
    onEnd(): void {
      this.__done = true;
      this.__eventCenter.emit('waitSignal', null);
    }
    onError(error: string): void {
      this.__error = new Error(error);
      this.__eventCenter.emit('waitSignal', null);
    }
    private processRead(resolve, reject) {
      if (this.__error) {
        return reject(this.__error);
      }
      if (
        this.__cancelled ||
        (this.__done && this.__dataReceived.length == 0)
      ) {
        return resolve({ done: true, value: undefined });
      }
      if (this.__dataReceived.length > 0) {
        const currData = this.__dataReceived.shift();
        return resolve({ done: false, value: currData });
      }
      // wait for signals
      const waitSignal = () => {
        this.__eventCenter.removeListener('waitSignal', waitSignal);
        this.processRead(resolve, reject);
      };

      this.__eventCenter.addListener('waitSignal', waitSignal, this);
    }
    public __read() {
      return new this.__Promise((resolve, reject) => {
        this.processRead(resolve, reject);
      });
    }
    public get locked() {
      return this.__locked;
    }
    public cancel(reason?: any) {
      this.__cancelled = true;
      this.__dataReceived = null;
      this.__eventCenter.emit('waitSignal', null);
      return this.__Promise.resolve(reason);
    }
    public getReader() {
      if (this.__locked) {
        return null;
      }
      this.__locked = true;
      return new ReadableStreamDefaultReader(this as any);
    }
  };
}
class ReadableStreamDefaultReader {
  private __stream;
  constructor(stream: ReadableStream) {
    this.__stream = stream;
  }
  public cancel(reason?: any) {
    return this.__stream.cancel(reason);
  }
  public read() {
    return this.__stream.__read();
  }
}
