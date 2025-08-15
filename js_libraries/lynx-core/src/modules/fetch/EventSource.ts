import { Lynx } from '@lynx-js/types';

type EventSourceEvent = {
  data: string;
  event?: string;
  id?: string;
  [key: string]: any;
};

interface FetchEventSourceOptions extends RequestInit {}

export function createEventSource(fetch: Lynx['fetch']): any {
  return class EventSource {
    private url: string;
    private options: FetchEventSourceOptions;
    private listeners: Record<string, EventListener[]> = {};
    private _closed: boolean;
    onmessage: (event: EventSourceEvent) => void;
    onerror: (event: Event) => void;
    onopen: (event: Event) => void;

    constructor(url: string, options: FetchEventSourceOptions = {}) {
      this.url = url;
      this.options = options;
      this._closed = false;
      this._connect();
    }

    public close(): void {
      this._closed = true;
    }

    private _dispatchEvent(type: string, event: EventSourceEvent): void {
      if (type === 'message' && this.onmessage) {
        this.onmessage(event);
      } else if (type === 'error' && this.onerror) {
        this.onerror(new Event('SSE error'));
      } else if (type === 'open' && this.onopen) {
        this.onopen(new Event('SSE open'));
      }
      const listeners = this.listeners[type] || [];
      listeners.forEach((listener) => listener((event as any) as Event));
    }

    public addEventListener(type: string, listener: EventListener): void {
      this.listeners[type] = this.listeners[type] || [];
      this.listeners[type].push(listener);
    }

    public removeEventListener(type: string, listener: EventListener): void {
      this.listeners[type] = this.listeners[type] || [];
      this.listeners[type] = this.listeners[type].filter((l) => l !== listener);
    }

    private async _connect(): Promise<void> {
      try {
        const response = await fetch(this.url, {
          ...this.options,
          lynxExtension: {
            useStreaming: true,
          },
        });
        this._dispatchEvent('open', { data: '' });
        const reader = response.body.getReader();
        while (true) {
          const { done, value } = await reader.read();
          if (done) break;
          const rawEvent = globalThis.TextCodecHelper.decode(value);
          const event = this._parseEvent(rawEvent);
          if (event) {
            this._dispatchEvent(event.event || 'message', event);
          }
        }
      } catch (err: any) {
        this._dispatchEvent('error', { data: '', error: err });
      }
    }

    private _parseEvent(raw: string): EventSourceEvent | null {
      const lines = raw.split('\n');
      let event: EventSourceEvent = { data: '' };
      for (const line of lines) {
        if (line.startsWith('data:')) {
          event.data += line.slice(5).trim() + '\n';
        } else if (line.startsWith('event:')) {
          event.event = line.slice(6).trim();
        } else if (line.startsWith('id:')) {
          event.id = line.slice(3).trim();
        }
      }
      // remove last newline
      if (event.data) event.data = event.data.slice(0, -1);
      return event.data ? event : null;
    }
  };
}
