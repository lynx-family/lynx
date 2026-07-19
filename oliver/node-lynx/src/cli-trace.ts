import * as fs from 'fs';
import * as path from 'path';
import { loadNodeLynxNativeBinding } from './headless-lynx-view';
import type { NativeBinding } from './headless-lynx-view';

export type CliTraceOptions = {
  outputPath: string;
  durationSeconds: number;
};

const SIGNAL_EXIT_CODES: Partial<Record<NodeJS.Signals, number>> = {
  SIGINT: 130,
  SIGTERM: 143,
};

function formatError(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export class CliTraceCapture {
  private readonly binding: NativeBinding;
  private readonly sessionId: number;
  private readonly stopPromise: Promise<void>;
  private readonly stopPromiseResolve: () => void;
  private readonly signalHandlers: Partial<Record<NodeJS.Signals, () => void>>;
  private stopTimer?: NodeJS.Timeout;
  private stopped = false;

  private constructor(
    binding: NativeBinding,
    sessionId: number,
    durationSeconds: number
  ) {
    this.binding = binding;
    this.sessionId = sessionId;
    let resolveStop!: () => void;
    this.stopPromise = new Promise<void>((resolve) => {
      resolveStop = resolve;
    });
    this.stopPromiseResolve = resolveStop;
    this.signalHandlers = {
      SIGINT: () => this.stopForSignal('SIGINT'),
      SIGTERM: () => this.stopForSignal('SIGTERM'),
    };
    this.installSignalHandlers();
    this.stopTimer = setTimeout(() => {
      try {
        this.stop();
      } catch (error: unknown) {
        fs.writeSync(
          process.stderr.fd,
          `failed to stop Lynx trace: ${formatError(error)}\n`
        );
        process.exitCode = 1;
      }
    }, durationSeconds * 1000);
  }

  static async start(options: CliTraceOptions): Promise<CliTraceCapture> {
    const outputPath = path.resolve(options.outputPath);
    await fs.promises.mkdir(path.dirname(outputPath), { recursive: true });
    const binding = loadNodeLynxNativeBinding();
    const sessionId = binding.startTracing(outputPath);
    if (!Number.isInteger(sessionId) || sessionId < 0) {
      throw new Error('failed to start Lynx trace');
    }
    return new CliTraceCapture(binding, sessionId, options.durationSeconds);
  }

  waitForStop(): Promise<void> {
    return this.stopPromise;
  }

  stop(): void {
    if (this.stopped) {
      return;
    }
    this.stopped = true;
    if (this.stopTimer) {
      clearTimeout(this.stopTimer);
      this.stopTimer = undefined;
    }
    this.removeSignalHandlers();
    let stopError: unknown;
    try {
      const stopped = this.binding.stopTracing(this.sessionId);
      if (!stopped) {
        stopError = new Error(
          `failed to stop Lynx trace session ${this.sessionId}`
        );
      }
    } catch (error: unknown) {
      stopError = error;
    } finally {
      this.stopPromiseResolve();
    }
    if (stopError) {
      throw stopError;
    }
  }

  private installSignalHandlers(): void {
    for (const signal of Object.keys(this.signalHandlers) as NodeJS.Signals[]) {
      const handler = this.signalHandlers[signal];
      if (handler) {
        process.once(signal, handler);
      }
    }
  }

  private removeSignalHandlers(): void {
    for (const signal of Object.keys(this.signalHandlers) as NodeJS.Signals[]) {
      const handler = this.signalHandlers[signal];
      if (handler) {
        process.off(signal, handler);
      }
    }
  }

  private stopForSignal(signal: NodeJS.Signals): void {
    try {
      this.stop();
    } catch (error: unknown) {
      fs.writeSync(
        process.stderr.fd,
        `failed to stop Lynx trace: ${formatError(error)}\n`
      );
    } finally {
      if (SIGNAL_EXIT_CODES[signal] !== undefined) {
        process.exitCode = SIGNAL_EXIT_CODES[signal];
      }
      if (process.listenerCount(signal) === 0) {
        process.kill(process.pid, signal);
      }
    }
  }
}

export async function startCliTraceCapture(
  options?: CliTraceOptions
): Promise<CliTraceCapture | undefined> {
  if (!options) {
    return undefined;
  }
  return CliTraceCapture.start(options);
}
