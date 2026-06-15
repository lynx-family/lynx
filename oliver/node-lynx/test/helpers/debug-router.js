const assert = require('assert');
const net = require('net');
const { spawn } = require('child_process');

const REQUEST_TIMEOUT_MS = 30000;
const SESSION_TIMEOUT_MS = 30000;
const SESSION_REQUEST_TIMEOUT_MS = 5000;
const CDP_REQUEST_TIMEOUT_MS = 5000;
const DOM_READY_TIMEOUT_MS = 60000;
const DOM_READY_POLL_INTERVAL_MS = 250;
const CHILD_OUTPUT_TAIL_CHARS = 4000;
const DEBUG_ROUTER_READY_TEXT = 'Node Lynx debug-router initialized.';
const MAX_CACHED_DEBUG_ROUTER_MESSAGES = 100;

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function encodeMessage(message) {
  const body = Buffer.from(JSON.stringify(message));
  const frame = Buffer.alloc(20 + body.length);
  frame.writeUInt32BE(1, 0);
  frame.writeUInt32BE(101, 4);
  frame.writeUInt32BE(0, 8);
  frame.writeUInt32BE(body.length + 4, 12);
  frame.writeUInt32BE(body.length, 16);
  body.copy(frame, 20);
  return frame;
}

function decodeFrames(buffer) {
  const messages = [];
  let offset = 0;
  while (buffer.length - offset >= 20) {
    const length = buffer.readUInt32BE(offset + 16);
    if (buffer.length - offset < 20 + length) {
      break;
    }
    const body = buffer.subarray(offset + 20, offset + 20 + length);
    messages.push(JSON.parse(body.toString('utf8')));
    offset += 20 + length;
  }
  return {
    messages,
    remaining: buffer.subarray(offset),
  };
}

function trimOutput(output) {
  if (output.length <= CHILD_OUTPUT_TAIL_CHARS) {
    return output;
  }
  return output.slice(output.length - CHILD_OUTPUT_TAIL_CHARS);
}

function captureChildOutput(child) {
  let stdout = '';
  let stderr = '';
  const onStdout = (chunk) => {
    stdout = trimOutput(stdout + chunk.toString());
  };
  const onStderr = (chunk) => {
    stderr = trimOutput(stderr + chunk.toString());
  };

  child.stdout.on('data', onStdout);
  child.stderr.on('data', onStderr);

  return {
    get stdout() {
      return stdout;
    },
    get stderr() {
      return stderr;
    },
    stop() {
      child.stdout.off('data', onStdout);
      child.stderr.off('data', onStderr);
    },
  };
}

function formatChildOutput(childOutput) {
  return [
    'child stdout tail:',
    childOutput.stdout || '<empty>',
    'child stderr tail:',
    childOutput.stderr || '<empty>',
  ].join('\n');
}

function formatError(error) {
  return error instanceof Error ? error.stack || error.message : String(error);
}

function childStatus(child) {
  if (child.exitCode !== null || child.signalCode !== null) {
    return `exited code=${child.exitCode} signal=${child.signalCode}`;
  }
  return 'running';
}

class DebugRouterConnection {
  constructor(port) {
    this.port = port;
    this.socket = net.createConnection({ host: '127.0.0.1', port });
    this.buffer = Buffer.alloc(0);
    this.pending = [];
    this.cachedMessages = [];
    this.closing = false;

    this.ready = new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        cleanup();
        this.socket.destroy();
        reject(new Error(`timed out connecting to DebugRouter port ${port}`));
      }, REQUEST_TIMEOUT_MS);
      const cleanup = () => {
        clearTimeout(timer);
        this.socket.off('connect', onConnect);
        this.socket.off('error', onError);
        this.socket.off('close', onClose);
      };
      const onConnect = () => {
        cleanup();
        this.socket.setNoDelay(true);
        resolve();
      };
      const onError = (error) => {
        cleanup();
        reject(error);
      };
      const onClose = () => {
        cleanup();
        reject(new Error('DebugRouter connection closed before ready'));
      };

      this.socket.once('connect', onConnect);
      this.socket.once('error', onError);
      this.socket.once('close', onClose);
    });

    this.socket.on('data', (chunk) => this.handleData(chunk));
    this.socket.on('error', (error) => this.rejectPending(error));
    this.socket.on('close', () => {
      if (!this.closing) {
        this.rejectPending(new Error('DebugRouter connection closed'));
      }
    });
  }

  static async connect(port) {
    const connection = new DebugRouterConnection(port);
    try {
      await connection.ready;
      return connection;
    } catch (error) {
      connection.close();
      throw error;
    }
  }

  sendMessage(message, predicate, timeoutMs = REQUEST_TIMEOUT_MS) {
    const cachedIndex = this.cachedMessages.findIndex(predicate);
    if (cachedIndex !== -1) {
      const [response] = this.cachedMessages.splice(cachedIndex, 1);
      return Promise.resolve(response);
    }

    if (this.closing || this.socket.destroyed) {
      return Promise.reject(new Error('DebugRouter connection is closed'));
    }

    return new Promise((resolve, reject) => {
      const pending = {
        message,
        predicate,
        resolve,
        reject,
        timer: null,
      };
      pending.timer = setTimeout(() => {
        this.removePending(pending);
        reject(
          new Error(
            `timed out waiting for DebugRouter response: ${JSON.stringify(
              message
            )}`
          )
        );
      }, timeoutMs);

      this.pending.push(pending);
      this.socket.write(encodeMessage(message), (error) => {
        if (error) {
          this.finishPending(pending, reject, error);
        }
      });
    });
  }

  close() {
    this.closing = true;
    this.rejectPending(new Error('DebugRouter connection closed'));
    this.socket.destroy();
  }

  handleData(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    const decoded = decodeFrames(this.buffer);
    this.buffer = decoded.remaining;
    for (const message of decoded.messages) {
      this.handleMessage(message);
    }
  }

  handleMessage(message) {
    for (const pending of this.pending) {
      if (pending.predicate(message)) {
        this.finishPending(pending, pending.resolve, message);
        return;
      }
    }
    this.cachedMessages.push(message);
    if (this.cachedMessages.length > MAX_CACHED_DEBUG_ROUTER_MESSAGES) {
      this.cachedMessages.shift();
    }
  }

  finishPending(pending, callback, value) {
    if (!this.removePending(pending)) {
      return;
    }
    clearTimeout(pending.timer);
    callback(value);
  }

  removePending(pending) {
    const index = this.pending.indexOf(pending);
    if (index === -1) {
      return false;
    }
    this.pending.splice(index, 1);
    return true;
  }

  rejectPending(error) {
    const pendingMessages = this.pending.splice(0);
    for (const pending of pendingMessages) {
      clearTimeout(pending.timer);
      pending.reject(error);
    }
  }
}

function sendMessage(connection, message, predicate, timeoutMs) {
  return connection.sendMessage(message, predicate, timeoutMs);
}

async function waitForCliReady(child, childOutput) {
  let settled = false;

  return new Promise((resolve, reject) => {
    if (childOutput.stdout.includes(DEBUG_ROUTER_READY_TEXT)) {
      resolve();
      return;
    }

    const timer = setTimeout(() => {
      settled = true;
      cleanup();
      reject(
        new Error(
          `node-lynx CLI did not initialize DebugRouter\n${formatChildOutput(
            childOutput
          )}`
        )
      );
    }, REQUEST_TIMEOUT_MS);

    const cleanup = () => {
      clearTimeout(timer);
      child.stdout.off('data', onStdout);
      child.off('exit', onExit);
    };
    const onStdout = (chunk) => {
      if (
        chunk.toString().includes(DEBUG_ROUTER_READY_TEXT) ||
        childOutput.stdout.includes(DEBUG_ROUTER_READY_TEXT)
      ) {
        settled = true;
        cleanup();
        resolve();
      }
    };
    const onExit = (code, signal) => {
      if (settled) {
        return;
      }
      settled = true;
      cleanup();
      reject(
        new Error(
          `node-lynx CLI exited before DebugRouter was ready: code=${code} signal=${signal}\n${formatChildOutput(
            childOutput
          )}`
        )
      );
    };

    child.stdout.on('data', onStdout);
    child.once('exit', onExit);
  });
}

async function waitForChildExit(child) {
  if (child.exitCode !== null || child.signalCode !== null) {
    return { code: child.exitCode, signal: child.signalCode };
  }
  return new Promise((resolve) => {
    child.once('exit', (code, signal) => resolve({ code, signal }));
  });
}

async function initialize(connection) {
  const response = await sendMessage(
    connection,
    { event: 'Initialize', data: connection.port },
    (message) => message.event === 'Register'
  );
  assert.strictEqual(response.data.info.App, 'NodeLynxCLI');
  return response;
}

async function sendAppMessage(connection, clientId, method, params) {
  const id = Math.floor(Math.random() * 40000) + 10000;
  const response = await sendMessage(
    connection,
    {
      event: 'Customized',
      data: {
        type: 'App',
        sender: clientId,
        data: {
          client_id: clientId,
          session_id: -1,
          message: { id, method, params: { ...params } },
        },
      },
    },
    (message) => {
      if (message.event !== 'Customized' || message.data.type !== 'App') {
        return false;
      }
      const payload = JSON.parse(message.data.data.message);
      return payload.id === id;
    }
  );
  const payload = JSON.parse(response.data.data.message);
  const result = JSON.parse(payload.result);
  assert.strictEqual(result.code, 0, `${method} failed: ${payload.result}`);
  return result;
}

async function listSessions(connection, clientId) {
  const response = await sendMessage(
    connection,
    {
      event: 'Customized',
      data: {
        type: 'ListSession',
        sender: clientId,
        data: {
          client_id: clientId,
        },
      },
    },
    (message) =>
      message.event === 'Customized' && message.data.type === 'SessionList',
    SESSION_REQUEST_TIMEOUT_MS
  );
  return response.data.data;
}

async function waitForSession(connection, clientId) {
  const start = Date.now();
  let lastError;
  while (Date.now() - start < SESSION_TIMEOUT_MS) {
    try {
      const sessions = await listSessions(connection, clientId);
      if (sessions.length > 0) {
        return sessions[sessions.length - 1];
      }
    } catch (error) {
      lastError = error;
    }
    await delay(250);
  }
  if (lastError) {
    throw new Error(
      `timed out waiting for node-lynx DebugRouter session; last error: ${formatError(
        lastError
      )}`
    );
  }
  throw new Error('timed out waiting for node-lynx DebugRouter session');
}

async function sendCDPMessage(
  connection,
  clientId,
  sessionId,
  method,
  params,
  timeoutMs = REQUEST_TIMEOUT_MS
) {
  const id = Math.floor(Math.random() * 40000) + 10000;
  const response = await sendMessage(
    connection,
    {
      event: 'Customized',
      data: {
        type: 'CDP',
        sender: clientId,
        data: {
          client_id: clientId,
          session_id: sessionId,
          message: { id, method, params },
        },
      },
    },
    (message) => {
      if (message.event !== 'Customized' || message.data.type !== 'CDP') {
        return false;
      }
      const payload = JSON.parse(message.data.data.message);
      return payload.id === id;
    },
    timeoutMs
  );
  const payload = JSON.parse(response.data.data.message);
  if (payload.error) {
    throw new Error(`CDP ${method} failed: ${payload.error.message}`);
  }
  return payload.result;
}

function wrapCDPError(method, error) {
  return new Error(
    `CDP ${method} failed while waiting for DOM readiness: ${formatError(
      error
    )}`
  );
}

function formatDomReadyTimeout({
  templateUrl,
  clientId,
  sessionId,
  lastError,
  child,
  childOutput,
}) {
  return [
    `timed out waiting for DebugRouter DOM readiness after ${DOM_READY_TIMEOUT_MS}ms`,
    `templateUrl: ${templateUrl}`,
    `clientId: ${clientId}`,
    `sessionId: ${sessionId}`,
    `child status: ${childStatus(child)}`,
    'last error:',
    lastError ? formatError(lastError) : '<none>',
    formatChildOutput(childOutput),
  ].join('\n');
}

async function waitForDomDocument({
  connection,
  clientId,
  sessionId,
  templateUrl,
  child,
  childOutput,
}) {
  const start = Date.now();
  let lastError;

  while (Date.now() - start < DOM_READY_TIMEOUT_MS) {
    try {
      await sendCDPMessage(
        connection,
        clientId,
        sessionId,
        'DOM.enable',
        {
          useCompression: false,
        },
        CDP_REQUEST_TIMEOUT_MS
      );
    } catch (error) {
      lastError = wrapCDPError('DOM.enable', error);
      if (child.exitCode !== null || child.signalCode !== null) {
        break;
      }
      await delay(DOM_READY_POLL_INTERVAL_MS);
      continue;
    }

    try {
      const document = await sendCDPMessage(
        connection,
        clientId,
        sessionId,
        'DOM.getDocument',
        {
          depth: 1,
        },
        CDP_REQUEST_TIMEOUT_MS
      );
      if (document.root) {
        return document;
      }
      lastError = new Error(
        `CDP DOM.getDocument returned without root: ${JSON.stringify(document)}`
      );
    } catch (error) {
      lastError = wrapCDPError('DOM.getDocument', error);
    }

    if (child.exitCode !== null || child.signalCode !== null) {
      break;
    }
    await delay(DOM_READY_POLL_INTERVAL_MS);
  }

  throw new Error(
    formatDomReadyTimeout({
      templateUrl,
      clientId,
      sessionId,
      lastError,
      child,
      childOutput,
    })
  );
}

async function runDebugRouterProtocolSmoke({
  packageRoot,
  cliPath,
  templateUrl,
  port = 8901,
  clientId = port,
}) {
  const child = spawn(process.execPath, [
    cliPath,
    'render',
    '--timeout',
    String(REQUEST_TIMEOUT_MS),
  ], {
    cwd: packageRoot,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
  const childOutput = captureChildOutput(child);
  let connection;

  try {
    await waitForCliReady(child, childOutput);
    connection = await DebugRouterConnection.connect(port);
    await initialize(connection);

    await sendAppMessage(connection, clientId, 'App.openPage', {
      url: templateUrl,
    });

    const session = await waitForSession(connection, clientId);
    assert.strictEqual(typeof session.session_id, 'number');

    const document = await waitForDomDocument({
      connection,
      clientId,
      sessionId: session.session_id,
      templateUrl,
      child,
      childOutput,
    });
    assert(document.root, 'expected DOM.getDocument to return a root node');

    await sendAppMessage(connection, clientId, 'App.closePage');
    const { code, signal } = await waitForChildExit(child);
    assert.strictEqual(signal, null);
    assert.strictEqual(code, 0);
  } finally {
    connection?.close();
    if (child.exitCode === null && child.signalCode === null) {
      child.kill('SIGTERM');
      await waitForChildExit(child);
    }
    childOutput.stop();
  }
}

module.exports = {
  runDebugRouterProtocolSmoke,
};
