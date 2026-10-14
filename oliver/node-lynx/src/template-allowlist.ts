import * as fs from 'fs';
import * as path from 'path';
import * as http from 'http';
import * as https from 'https';
import { fileURLToPath } from 'url';

function denied(target: string): Error {
  return Object.assign(
    new Error(
      `ERR_NODE_LYNX_TEMPLATE_DENIED: template is outside the allowlist: ${target}`
    ),
    { code: 'ERR_NODE_LYNX_TEMPLATE_DENIED' }
  );
}

function httpUrl(value: string): URL | undefined {
  if (!/^https?:/i.test(value)) return undefined;
  const url = new URL(value);
  if (url.username || url.password) throw denied(value);
  return url;
}

// Match path segments, never a raw URL string prefix. Reject encoded traversal
// and separators rather than relying on a server's decoding conventions.
function urlPath(url: URL): string {
  let pathname = url.pathname;
  for (let i = 0; i < 8; i++) {
    if (/%2f|%5c|%00/i.test(pathname)) throw denied(url.href);
    const decoded = decodeURIComponent(pathname);
    if (decoded.split('/').some((part) => part === '.' || part === '..'))
      throw denied(url.href);
    if (decoded === pathname) return pathname;
    pathname = decoded;
  }
  throw denied(url.href);
}

function contains(root: string, file: string): boolean {
  const relative = path.relative(root, file);
  return (
    relative === '' ||
    (!path.isAbsolute(relative) &&
      relative !== '..' &&
      !relative.startsWith(`..${path.sep}`))
  );
}

/** Host-owned template directories and URL prefixes, fixed at construction. */
export class TemplateAllowlist {
  private readonly roots: string[] = [];
  private readonly prefixes: Array<{ origin: string; pathname: string }> = [];

  constructor(entries: readonly string[], private readonly timeoutMs = 10000) {
    for (const entry of entries) {
      const url = httpUrl(entry);
      if (url) {
        if (url.search || url.hash)
          throw new Error(
            'template URL prefixes must not contain a query or fragment'
          );
        this.prefixes.push({
          origin: url.origin,
          pathname: urlPath(url).replace(/\/+$/, ''),
        });
      } else {
        const root = fs.realpathSync(
          entry.startsWith('file://')
            ? fileURLToPath(entry)
            : path.resolve(entry)
        );
        if (!fs.statSync(root).isDirectory())
          throw new Error(
            `template allowlist entry must be a directory: ${entry}`
          );
        this.roots.push(root);
      }
    }
  }

  private resolve(target: string): URL | string {
    try {
      const url = httpUrl(target);
      if (url) {
        const pathname = urlPath(url);
        if (
          this.prefixes.some(
            (prefix) =>
              prefix.origin === url.origin &&
              (pathname === prefix.pathname ||
                pathname.startsWith(`${prefix.pathname}/`))
          )
        )
          return url;
      } else {
        const file = target.startsWith('file://')
          ? fileURLToPath(target)
          : target;
        if (path.isAbsolute(file)) {
          const real = fs.realpathSync(file);
          if (
            this.roots.some((root) => contains(root, real)) &&
            fs.statSync(real).isFile()
          )
            return real;
        }
      }
    } catch {
      // Malformed URLs, missing files and failed path resolution fail closed.
    }
    throw denied(target);
  }

  assertAllowed(target: string): void {
    this.resolve(target);
  }

  async read(target: string): Promise<Buffer> {
    const resolved = this.resolve(target);
    if (resolved instanceof URL) return this.readHttp(resolved, 5);
    const file = await fs.promises.open(
      resolved,
      fs.constants.O_RDONLY | fs.constants.O_NOFOLLOW | fs.constants.O_NONBLOCK
    );
    try {
      const stat = await file.stat();
      if (!stat.isFile()) throw denied(target);
      // Recheck common path-replacement races. The host must still control the
      // allowed directories; this is not a filesystem sandbox.
      const current = this.resolve(target);
      if (current !== resolved) throw denied(target);
      const currentStat = await fs.promises.stat(current as string);
      if (stat.dev !== currentStat.dev || stat.ino !== currentStat.ino)
        throw denied(target);
      return await file.readFile();
    } finally {
      await file.close();
    }
  }

  private readHttp(url: URL, redirects: number): Promise<Buffer> {
    this.assertAllowed(url.href);
    return new Promise((resolve, reject) => {
      const request = (url.protocol === 'https:' ? https : http).get(
        url,
        (response) => {
          response.on('error', reject);
          const status = response.statusCode ?? 0;
          if (status >= 300 && status < 400 && response.headers.location) {
            response.resume();
            try {
              const next = new URL(response.headers.location, url);
              if (redirects === 0 || !httpUrl(next.href))
                throw denied(next.href);
              this.readHttp(next, redirects - 1).then(resolve, reject);
            } catch (error) {
              reject(error);
            }
            return;
          }
          if (status < 200 || status >= 300) {
            response.resume();
            reject(
              new Error(`request failed with status ${status}: ${url.href}`)
            );
            return;
          }
          const chunks: Buffer[] = [];
          response.on('data', (chunk) => chunks.push(Buffer.from(chunk)));
          response.on('end', () => resolve(Buffer.concat(chunks)));
        }
      );
      const timeout = setTimeout(
        () => request.destroy(new Error('template download timed out')),
        this.timeoutMs
      );
      request.on('close', () => clearTimeout(timeout));
      request.on('error', reject);
    });
  }
}
