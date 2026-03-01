# Remote Tenancy and Lease Admission

Use this reference for remote daemon HTTP flows that require explicit
tenant/run admission control.

## Transport prerequisites

- Start daemon in HTTP mode (`AGENT_DEVICE_DAEMON_SERVER_MODE=http|dual`).
- Use a token from daemon metadata or `Authorization: Bearer <token>`.
- Prefer an auth hook (`AGENT_DEVICE_HTTP_AUTH_HOOK`) for caller validation and
  tenant injection.

## Lease lifecycle (JSON-RPC)

Use `POST /rpc` with JSON-RPC 2.0 methods:

- `agent_device.lease.allocate`
- `agent_device.lease.heartbeat`
- `agent_device.lease.release`

Example allocate:

```bash
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"alloc-1","method":"agent_device.lease.allocate","params":{"tenantId":"acme","runId":"run-123","ttlMs":60000}}'
```

Example heartbeat:

```bash
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"hb-1","method":"agent_device.lease.heartbeat","params":{"leaseId":"<lease-id>","ttlMs":60000}}'
```

Example release:

```bash
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"rel-1","method":"agent_device.lease.release","params":{"leaseId":"<lease-id>"}}'
```

## Command admission contract

For tenant-isolated command execution, pass all four flags:

```bash
agent-device --daemon-transport http \
  --tenant acme \
  --session-isolation tenant \
  --run-id run-123 \
  --lease-id <lease-id> \
  session list --json
```

Admission checks require tenant/run/lease scope alignment.

## Failure semantics

- Missing tenant/run/lease fields in tenant isolation mode: `INVALID_ARGS`
- Lease not active or wrong scope: `UNAUTHORIZED`
- Method mismatch: JSON-RPC `-32601` (HTTP 404)

## Operational guidance

- Keep TTL short and heartbeat only while a run is active.
- Release lease immediately on run completion/error paths.
- For bounded hosts, configure:
  - `AGENT_DEVICE_MAX_SIMULATOR_LEASES`
  - `AGENT_DEVICE_LEASE_TTL_MS`
  - `AGENT_DEVICE_LEASE_MIN_TTL_MS`
  - `AGENT_DEVICE_LEASE_MAX_TTL_MS`
