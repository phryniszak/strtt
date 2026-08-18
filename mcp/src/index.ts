#!/usr/bin/env node
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";

import { StrttSession } from "./strttSession.js";

const session = new StrttSession();

const server = new McpServer({
  name: "strtt-mcp",
  version: "0.1.0",
});

function textResult(payload: unknown) {
  return {
    content: [
      {
        type: "text" as const,
        text: typeof payload === "string" ? payload : JSON.stringify(payload, null, 2),
      },
    ],
  };
}

function errorResult(err: unknown) {
  return {
    isError: true,
    content: [
      {
        type: "text" as const,
        text: err instanceof Error ? err.message : String(err),
      },
    ],
  };
}

server.registerTool(
  "strtt_start",
  {
    title: "Start strtt",
    description:
      "Launch strtt against an ST-LINK probe/target to begin capturing RTT channel 0. " +
      "Fails if a session is already running (call strtt_stop first).",
    inputSchema: {
      ramstart: z.string().optional().describe("RAM start address strtt scans from, e.g. '0x20000000'"),
      ramsize: z.string().optional().describe("Size of the RAM window to scan, e.g. '0x2000' or in KB"),
      serial: z.string().optional().describe("ST-LINK serial number, when multiple probes are attached"),
      ap: z.number().int().optional().describe("Access port number (e.g. 1 for STM32H5/H7)"),
      tcp: z.boolean().optional().describe("Connect via ST-LINK GDB server over TCP instead of USB directly"),
      port: z.number().int().optional().describe("TCP port for the ST-LINK GDB server connection"),
      verbosity: z
        .number()
        .int()
        .min(-3)
        .max(4)
        .optional()
        .describe("strtt debug level, -3 (silent) to 4; defaults to 0 (errors only) so failures are diagnosable"),
    },
  },
  async (args) => {
    try {
      const status = await session.start(args);
      return textResult(status);
    } catch (err) {
      return errorResult(err);
    }
  }
);

server.registerTool(
  "strtt_stop",
  {
    title: "Stop strtt",
    description: "Gracefully stop the running strtt session (SIGINT, then SIGKILL if it doesn't exit in time).",
    inputSchema: {},
  },
  async () => {
    try {
      const status = await session.stop();
      return textResult(status);
    } catch (err) {
      return errorResult(err);
    }
  }
);

server.registerTool(
  "strtt_status",
  {
    title: "strtt session status",
    description: "Report whether strtt is running, its args, start time, and last exit/error info.",
    inputSchema: {},
  },
  async () => textResult(session.status())
);

server.registerTool(
  "strtt_read",
  {
    title: "Read RTT output",
    description:
      "Tail RTT channel 0 output captured from the running strtt session. Pass the cursor " +
      "returned by the previous call to fetch only new data since then; omit it to read from " +
      "the start of what's currently buffered.",
    inputSchema: {
      cursor: z.number().int().min(0).optional().describe("Byte cursor from a previous strtt_read call"),
      maxBytes: z.number().int().positive().max(1024 * 1024).optional().describe("Max bytes to return (default 65536)"),
    },
  },
  async ({ cursor, maxBytes }) => {
    try {
      const result = session.read(cursor, maxBytes);
      return textResult(result);
    } catch (err) {
      return errorResult(err);
    }
  }
);

server.registerTool(
  "strtt_write",
  {
    title: "Write to RTT channel 0",
    description: "Send text to the device over RTT channel 0 (the down-buffer), as if typed at the strtt console.",
    inputSchema: {
      text: z.string().describe("Text to send"),
      newline: z.boolean().optional().describe("Append a trailing newline (default true)"),
    },
  },
  async ({ text, newline }) => {
    try {
      session.write(text, newline);
      return textResult({ written: text.length });
    } catch (err) {
      return errorResult(err);
    }
  }
);

// A running strtt child keeps its own event loop handles open (stdout/stderr
// listeners), so this process won't necessarily exit on its own just because
// the MCP client disconnected. Make sure a live strtt child never outlives
// this server, however it goes down.
let shuttingDown = false;
function shutdown(): void {
  if (shuttingDown) return;
  shuttingDown = true;
  session.killSync();
}

server.server.onclose = () => {
  shutdown();
  process.exit(0);
};
process.on("SIGINT", () => {
  shutdown();
  process.exit(0);
});
process.on("SIGTERM", () => {
  shutdown();
  process.exit(0);
});
process.on("exit", shutdown);

async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((err) => {
  console.error("strtt-mcp fatal error:", err);
  process.exit(1);
});
