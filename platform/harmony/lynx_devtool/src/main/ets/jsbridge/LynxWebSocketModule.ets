// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BusinessError } from '@kit.BasicServicesKit';
import { webSocket } from '@kit.NetworkKit';
import { buffer, util } from '@kit.ArkTS';
import { LLog, LynxContext, LynxModule } from '@lynx/lynx';

const TAG = 'LynxWebSocketModule';

interface WebSocketConnection {
  socket: webSocket.WebSocket;
  onOpen: (err: BusinessError, value: Object) => void;
  onMessage: (err: BusinessError, value: string | ArrayBuffer) => void;
  onClose: (err: BusinessError, value: webSocket.CloseResult) => void;
  onError: (err: BusinessError) => void;
}

export class LynxWebSocketModule extends LynxModule {
  private webSocketConnections: Map<number, WebSocketConnection>;

  constructor(context: LynxContext, param?: Object) {
    super(context, param);
    this.webSocketConnections = new Map<number, WebSocketConnection>();
  }

  public connect(url: string, protocols: Object[], options: Object[], id: number) {
    LLog.d(TAG, 'connect: ' + url);
    this.closeConnection(id);
    const ws = webSocket.createWebSocket();
    const onOpen = (err: BusinessError, value: Object) => {
      if (!this.isCurrentConnection(id, ws)) {
        return;
      }
      if (!err) {
        const arg: Record<string, Object> = {
          'id': id,
          'protocol': '',
        };
        this.sendEvent('websocketOpen', arg);
      }
    };
    const onMessage = (err: BusinessError, value: string | ArrayBuffer) => {
      if (!this.isCurrentConnection(id, ws)) {
        return;
      }
      if (!err) {
        let type = 'text';
        if (buffer.isBuffer(value)) {
          const base64 = new util.Base64Helper();
          const binaryValue: ArrayBuffer = value as ArrayBuffer;
          value = base64.encodeToStringSync(new Uint8Array(binaryValue));
          type = 'binary';
        }
        const arg: Record<string, Object> = {
          'id': id,
          'type': type,
          'data': value,
        };
        this.sendEvent('websocketMessage', arg);
      }
    };
    const onClose = (err: BusinessError, value: webSocket.CloseResult) => {
      if (!this.isCurrentConnection(id, ws)) {
        return;
      }
      if (!err) {
        const arg: Record<string, Object> = {
          'id': id,
          'code': value.code,
          'reason': value.reason,
        };
        this.sendEvent('websocketClosed', arg);
      }
      this.cleanupConnection(id, ws, false);
    };
    const onError = (err: BusinessError) => {
      if (!this.isCurrentConnection(id, ws)) {
        return;
      }
      this.notifyWebSocketFailed(id, err.message, true);
      this.cleanupConnection(id, ws, true);
    };
    this.webSocketConnections.set(id, {
      socket: ws,
      onOpen: onOpen,
      onMessage: onMessage,
      onClose: onClose,
      onError: onError,
    });
    ws.on('open', onOpen);
    ws.on('message', onMessage);
    ws.on('close', onClose);
    ws.on('error', onError);
    ws.connect(url, (err: BusinessError, value: boolean) => {
      if (err && this.isCurrentConnection(id, ws)) {
        this.notifyWebSocketFailed(id, err.message, true);
        this.cleanupConnection(id, ws, true);
      }
    });
  }

  public close(code: number, reason: string, id: number): void {
    const connection = this.webSocketConnections.get(id);
    if (connection) {
      const ws = connection.socket;
      ws.close({ code: code, reason: reason }, (err: BusinessError, value: boolean) => {
        if (err && this.isCurrentConnection(id, ws)) {
          LLog.e(TAG, 'Could not close WebSocket connection for id ' + id + ', e:' + err.message);
          this.cleanupConnection(id, ws, false);
        }
      });
    }
  }

  public send(message: string, id: number): void {
    const connection = this.webSocketConnections.get(id);
    if (connection) {
      const ws = connection.socket;
      ws.send(message, (err: BusinessError, value: boolean) => {
        if (err && this.isCurrentConnection(id, ws)) {
          this.notifyWebSocketFailed(id, err.message);
        }
      });
    }
  }

  public ping(id: number) {
    const connection = this.webSocketConnections.get(id);
    if (connection) {
      const ws = connection.socket;
      ws.send('', (err: BusinessError, value: boolean) => {
        if (err && this.isCurrentConnection(id, ws)) {
          this.notifyWebSocketFailed(id, err.message, true);
          this.cleanupConnection(id, ws, true);
        }
      });
    } else {
      this.notifyWebSocketFailed(id, 'client is null', true);
    }
  }

  public sendEvent(name: string, data: Object) {
    this.context.sendGlobalEvent(name, [data]);
  }

  private notifyWebSocketFailed(id: number, message: string, withClose = false) {
    const arg: Record<string, Object> = {
      'id': id,
      'message': message,
    };
    this.sendEvent('websocketFailed', arg);
    if (withClose) {
      const arg: Record<string, Object> = {
        'id': id,
        'code': 0,
        'reason': message,
      };
      this.sendEvent('websocketClosed', arg);
    }
  }

  private isCurrentConnection(id: number, socket: webSocket.WebSocket): boolean {
    return this.webSocketConnections.get(id)?.socket === socket;
  }

  private closeConnection(id: number): void {
    const connection = this.webSocketConnections.get(id);
    if (connection) {
      this.cleanupConnection(id, connection.socket, true);
    }
  }

  private cleanupConnection(id: number, socket: webSocket.WebSocket, close: boolean): void {
    const connection = this.webSocketConnections.get(id);
    if (!connection || connection.socket !== socket) {
      return;
    }
    socket.off('open', connection.onOpen);
    socket.off('message', connection.onMessage);
    socket.off('close', connection.onClose);
    socket.off('error', connection.onError);
    this.webSocketConnections.delete(id);

    if (close) {
      socket.close((err: BusinessError, value: boolean) => {
        if (err) {
          LLog.e(TAG, 'Could not close WebSocket connection for id ' + id + ', e:' + err.message);
        }
      });
    }
  }
}
