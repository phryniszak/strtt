# strtt-mcp

MCP server that lets an assistant drive [`strtt`](../README.md) directly: start a capture
session against an ST-LINK probe, tail the live RTT terminal (channel 0) output, send text
back to the device, check status, and stop the session.

It works by spawning the `strtt` binary as a child process and proxying it:

- **stdout** (pure RTT channel-0 payload bytes) is captured into an in-memory ring buffer
  (capped at 1 MiB) that tools read from incrementally via a byte cursor.
- **stderr** (strtt's own diagnostics/log output) is kept separately and surfaced on failures
  and in `strtt_status`.
- **stdin** is written to directly to inject text into the device via RTT channel 0.

No changes to `strtt` itself are required — its stdio already separates RTT payload from
diagnostics, and `ConsoleInput` works fine over a plain pipe (no TTY needed).

## Build

```bash
cd mcp
npm install
npm run build
```

## Locating the strtt binary

By default the server looks for the binary in this order:

1. `STRTT_BIN` environment variable, if set.
2. `../build/src/rtt/strtt` relative to this directory (the repo's default CMake build output).
3. `strtt` on `PATH`.

## Tools

| Tool | Description |
|---|---|
| `strtt_start` | Launch strtt. Optional args: `ramstart`, `ramsize`, `serial`, `ap`, `tcp`, `port`, `verbosity`. Errors if a session is already running. |
| `strtt_stop` | Gracefully stop the running session. |
| `strtt_status` | Report running state, args, start time, last exit code/signal, last stderr. |
| `strtt_read` | Read RTT channel-0 output since a cursor (omit for from-the-start); returns `{ text, cursor, truncated }`. |
| `strtt_write` | Send text to the device via RTT channel 0. |

Only one strtt session is managed at a time; channels other than 0 (e.g. SysView) are out of
scope for this server.

## Example: talking to stm32g431_RTT_InputEchoApp

[`stm32g431_RTT_InputEchoApp`](../stm32g431_RTT_InputEchoApp) is a sample NUCLEO-G431KB
firmware in this repo built exactly to exercise strtt: on boot it writes a banner to RTT
channel 0, then loops forever reading one character at a time from the down-buffer and
echoing it straight back to the up-buffer ([main.c](../stm32g431_RTT_InputEchoApp/Core/Src/main.c)):

```c
SEGGER_RTT_WriteString(0, "SEGGER Real-Time-Terminal Sample\r\n");
while (1) {
    ch = SEGGER_RTT_WaitKey();
    SEGGER_RTT_Write(0, &ch, 1);
}
```

Its linker script gives it 32 KB of RAM starting at `0x20000000`
([STM32G431KBTX_FLASH.ld](../stm32g431_RTT_InputEchoApp/STM32G431KBTX_FLASH.ld)), so pass
`ramsize: "32"` (KB) to make sure the whole RAM is scanned for the RTT control block regardless
of where the linker placed it. Flash the app to the board first, then, with the assistant
connected to this MCP server:

1. **Start a session** — `strtt_start` with `{ "ramstart": "0x20000000", "ramsize": "32" }`
   (add `"serial": "..."` if more than one ST-LINK is attached):
   ```json
   { "running": true, "pid": 12345, "args": ["-v", "0", "-ramsize", "32", "-ramstart", "0x20000000"], "startedAt": "..." }
   ```
   The boot banner is only written once, right after reset, so unless strtt is already
   attached and reading at that moment it'll almost never show up in `strtt_read` — don't
   expect to see it.
2. **Send something** — `strtt_write` with `{ "text": "hi", "newline": false }`. The firmware's
   down-buffer is only 16 bytes (`BUFFER_SIZE_DOWN` in
   [SEGGER_RTT_Conf.h](../stm32g431_RTT_InputEchoApp/Core/RTT/SEGGER_RTT_Conf.h)), so keep
   writes short — a character or two at a time is the intended use for this sample.
3. **Read the echo** — `strtt_read` with `{}`, which returns what's buffered so far:
   ```json
   { "text": "hi", "cursor": 2, "truncated": false }
   ```
   Since the firmware echoes one character per loop iteration, whatever was written comes
   back byte-for-byte, in order.
4. **Stop the session** — `strtt_stop` with `{}` once done.

## Using with Claude Code / Claude Desktop

Add to your MCP server config:

```json
{
  "mcpServers": {
    "strtt": {
      "command": "node",
      "args": ["/absolute/path/to/strtt/mcp/dist/index.js"],
      "env": {
        "STRTT_BIN": "/absolute/path/to/strtt/build/src/rtt/strtt"
      }
    }
  }
}
```

## Standalone smoke test

```bash
npx @modelcontextprotocol/inspector node dist/index.js
```
