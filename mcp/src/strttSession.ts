import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const moduleDir = path.dirname(fileURLToPath(import.meta.url));

// How long to wait after spawning before declaring the session "started".
// strtt fails fast (exit(-1)) if it can't open the probe or find RTT, so a
// short grace period is enough to tell "failed to start" from "running".
const START_GRACE_PERIOD_MS = 1800;
const STOP_GRACE_PERIOD_MS = 3000;
const EXIT_POLL_INTERVAL_MS = 50;

// Ring buffer cap for RTT stdout. Trimmed from the front once exceeded.
const MAX_STDOUT_BYTES = 1024 * 1024;
const MAX_STDERR_BYTES = 64 * 1024;

export interface StartOptions {
  ramstart?: string;
  ramsize?: string;
  serial?: string;
  ap?: number;
  tcp?: boolean;
  port?: number;
  verbosity?: number;
}

export interface SessionStatus {
  running: boolean;
  pid?: number;
  args?: string[];
  startedAt?: string;
  exitCode?: number | null;
  exitSignal?: NodeJS.Signals | null;
  lastStderr?: string;
}

export interface ReadResult {
  text: string;
  cursor: number;
  truncated: boolean;
}

function resolveBinaryPath(): string {
  if (process.env.STRTT_BIN) return process.env.STRTT_BIN;

  const exeName = process.platform === "win32" ? "strtt.exe" : "strtt";
  // mcp/dist/strttSession.js -> ../.. -> repo root -> build/src/rtt/<exe>
  const fallback = path.resolve(moduleDir, "..", "..", "build", "src", "rtt", exeName);
  if (existsSync(fallback)) return fallback;

  // Fall back to PATH lookup (spawn/execvp resolves bare names via PATH).
  return exeName;
}

function optionsToArgs(opts: StartOptions): string[] {
  const args: string[] = [];
  // strtt defaults to LOG_LVL_SILENT (-3), which suppresses even error-level
  // diagnostics on stderr. That leaves failures (e.g. "no probe found") with
  // no detail beyond an exit code, so default to error-level (0) here unless
  // the caller explicitly asked for something else (including full silence).
  const verbosity = opts.verbosity ?? 0;
  args.push("-v", String(verbosity));
  if (opts.ramsize) args.push("-ramsize", opts.ramsize);
  if (opts.ramstart) args.push("-ramstart", opts.ramstart);
  if (opts.port !== undefined) args.push("-port", String(opts.port));
  if (opts.tcp) args.push("-tcp");
  if (opts.ap !== undefined) args.push("-ap", String(opts.ap));
  if (opts.serial) args.push("-serial", opts.serial);
  return args;
}

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Manages a single strtt child process: spawns it, proxies its RTT
 * channel-0 stdout into a cursor-addressable ring buffer, forwards writes
 * to its stdin, and tracks stderr/exit state for status reporting.
 *
 * strtt itself keeps RTT payload bytes on stdout and all diagnostics on
 * stderr (log_output defaults to stderr), so no framing/parsing is needed
 * here to separate the two.
 */
export class StrttSession {
  private child: ChildProcessWithoutNullStreams | null = null;
  private args: string[] = [];
  private startedAt: string | undefined;

  private stdoutBuf = Buffer.alloc(0);
  private stdoutBufStartOffset = 0; // absolute offset of stdoutBuf[0]

  private stderrBuf = Buffer.alloc(0);

  private lastExitCode: number | null = null;
  private lastExitSignal: NodeJS.Signals | null = null;

  isRunning(): boolean {
    return this.child !== null;
  }

  /**
   * Last-resort synchronous cleanup for when this process itself is going
   * down (signal handler, MCP transport closed, 'exit' event) and there's
   * no time/ability to await the graceful stop() sequence. child.kill()
   * just sends the signal and returns immediately, so this is safe to call
   * from an 'exit' handler.
   */
  killSync(): void {
    this.child?.kill("SIGKILL");
  }

  async start(opts: StartOptions): Promise<SessionStatus> {
    if (this.child) {
      throw new Error("strtt is already running; call strtt_stop first");
    }

    const bin = resolveBinaryPath();
    const args = optionsToArgs(opts);

    this.stdoutBuf = Buffer.alloc(0);
    this.stdoutBufStartOffset = 0;
    this.stderrBuf = Buffer.alloc(0);
    this.lastExitCode = null;
    this.lastExitSignal = null;

    const child = spawn(bin, args, { stdio: ["pipe", "pipe", "pipe"] });
    this.child = child;
    this.args = args;
    this.startedAt = new Date().toISOString();

    child.stdout.on("data", (chunk: Buffer) => this.appendStdout(chunk));
    child.stderr.on("data", (chunk: Buffer) => this.appendStderr(chunk));

    // Persistent handlers: these fire whenever the process ends, whether
    // that's during the startup grace period, during normal operation
    // (e.g. probe unplugged), or in response to stop(). They're the single
    // source of truth for "is a child currently alive".
    child.on("exit", (code, signal) => {
      this.lastExitCode = code;
      this.lastExitSignal = signal;
      this.child = null;
    });
    child.on("error", (err) => {
      this.appendStderr(Buffer.from(`spawn error: ${err.message}\n`));
      this.child = null;
    });

    await sleep(START_GRACE_PERIOD_MS);

    if (this.child === null) {
      const stderrTail = this.stderrBuf.toString("utf8");
      throw new Error(
        `strtt exited during startup (code=${this.lastExitCode}, signal=${this.lastExitSignal}): ${
          stderrTail || "(no stderr output)"
        }`
      );
    }

    return this.status();
  }

  async stop(): Promise<SessionStatus> {
    const child = this.child;
    if (!child) return this.status();

    child.kill("SIGINT");
    if (!(await this.waitForExit(STOP_GRACE_PERIOD_MS))) {
      child.kill("SIGKILL");
      await this.waitForExit(STOP_GRACE_PERIOD_MS);
    }

    return this.status();
  }

  write(text: string, newline = true): void {
    if (!this.child) {
      throw new Error("strtt is not running; call strtt_start first");
    }
    this.child.stdin.write(text + (newline ? "\n" : ""));
  }

  read(sinceCursor = 0, maxBytes = 65536): ReadResult {
    const truncated = sinceCursor < this.stdoutBufStartOffset;
    const effectiveStart = Math.max(sinceCursor, this.stdoutBufStartOffset);
    const startIndex = effectiveStart - this.stdoutBufStartOffset;
    const endIndex = Math.min(this.stdoutBuf.length, startIndex + Math.max(0, maxBytes));
    const slice = this.stdoutBuf.subarray(Math.max(0, startIndex), endIndex);

    return {
      text: slice.toString("utf8"),
      cursor: this.stdoutBufStartOffset + endIndex,
      truncated,
    };
  }

  status(): SessionStatus {
    return {
      running: this.child !== null,
      pid: this.child?.pid,
      args: this.args,
      startedAt: this.startedAt,
      exitCode: this.lastExitCode,
      exitSignal: this.lastExitSignal,
      lastStderr: this.stderrBuf.length ? this.stderrBuf.toString("utf8") : undefined,
    };
  }

  private async waitForExit(timeoutMs: number): Promise<boolean> {
    const deadline = Date.now() + timeoutMs;
    while (this.child !== null && Date.now() < deadline) {
      await sleep(EXIT_POLL_INTERVAL_MS);
    }
    return this.child === null;
  }

  private appendStdout(chunk: Buffer): void {
    this.stdoutBuf = Buffer.concat([this.stdoutBuf, chunk]);
    if (this.stdoutBuf.length > MAX_STDOUT_BYTES) {
      const trim = this.stdoutBuf.length - MAX_STDOUT_BYTES;
      this.stdoutBuf = this.stdoutBuf.subarray(trim);
      this.stdoutBufStartOffset += trim;
    }
  }

  private appendStderr(chunk: Buffer): void {
    this.stderrBuf = Buffer.concat([this.stderrBuf, chunk]);
    if (this.stderrBuf.length > MAX_STDERR_BYTES) {
      this.stderrBuf = this.stderrBuf.subarray(this.stderrBuf.length - MAX_STDERR_BYTES);
    }
  }
}
