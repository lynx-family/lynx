// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxVideoAttributes, LynxVideoMethodMode } from './VideoOptions';

export interface LynxVideoPlayable {
  setAttributes(attributes: LynxVideoAttributes): void;

  play(): Promise<void>;

  pause(): Promise<void>;

  stop(emitStopped?: boolean): Promise<void>;

  seek(position: number): Promise<void>;
}

export interface LynxVideoCommand {
  name: string;
  run: () => Promise<void>;
}

class PendingVideoCommand {
  command: LynxVideoCommand;
  resolve: () => void;
  reject: (reason: Error) => void;

  constructor(command: LynxVideoCommand, resolve: () => void, reject: (reason: Error) => void) {
    this.command = command;
    this.resolve = resolve;
    this.reject = reject;
  }
}

export class LynxVideoCommandScheduler {
  private mode: LynxVideoMethodMode = LynxVideoMethodMode.Queue;
  private running: boolean = false;
  private current: PendingVideoCommand | undefined = undefined;
  private queue: PendingVideoCommand[] = [];
  private latest: PendingVideoCommand | undefined = undefined;

  setMode(mode: LynxVideoMethodMode): void {
    if (this.mode === mode) {
      return;
    }
    this.mode = mode;
    this.rejectPending('video operation canceled by mode change');
  }

  cancelAll(reason: string): void {
    const error = new Error(reason);
    if (this.current !== undefined) {
      this.current.reject(error);
    }
    this.rejectPending(reason);
    if (this.current === undefined) {
      this.running = false;
    }
  }

  schedule(command: LynxVideoCommand): Promise<void> {
    if (this.mode === LynxVideoMethodMode.Direct) {
      return this.execute(command);
    }
    return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
      const pending = new PendingVideoCommand(command, resolve, reject);
      if (this.mode === LynxVideoMethodMode.Latest) {
        this.scheduleLatest(pending);
      } else {
        this.queue.push(pending);
        this.drainQueue();
      }
    });
  }

  private scheduleLatest(pending: PendingVideoCommand): void {
    if (!this.running) {
      this.startPending(pending);
      return;
    }

    if (this.latest !== undefined) {
      this.latest.reject(new Error('video operation was overwritten'));
    }
    this.latest = pending;
  }

  private drainLatest(): void {
    if (this.latest === undefined || this.running) {
      return;
    }
    const pending = this.latest;
    this.latest = undefined;
    this.startPending(pending);
  }

  private drainQueue(): void {
    if (this.running || this.queue.length === 0) {
      return;
    }
    const pending = this.queue.shift();
    if (pending === undefined) {
      return;
    }
    this.startPending(pending);
  }

  private startPending(pending: PendingVideoCommand): void {
    this.running = true;
    this.current = pending;
    this.runPending(pending).finally(() => {
      if (this.current !== pending) {
        return;
      }
      this.current = undefined;
      this.running = false;
      this.drain();
    });
  }

  private drain(): void {
    if (this.mode === LynxVideoMethodMode.Latest) {
      this.drainLatest();
      return;
    }
    if (this.mode === LynxVideoMethodMode.Queue) {
      this.drainQueue();
    }
  }

  private rejectPending(reason: string): void {
    const error = new Error(reason);
    const pendingCommands = this.queue.splice(0, this.queue.length);
    for (const pending of pendingCommands) {
      pending.reject(error);
    }
    if (this.latest !== undefined) {
      this.latest.reject(error);
      this.latest = undefined;
    }
  }

  private runPending(pending: PendingVideoCommand): Promise<void> {
    return this.execute(pending.command)
      .then(() => {
        pending.resolve();
      })
      .catch((error: Error) => {
        pending.reject(error);
      });
  }

  private execute(command: LynxVideoCommand): Promise<void> {
    try {
      return command.run();
    } catch (error) {
      return Promise.reject(error as Error);
    }
  }
}
