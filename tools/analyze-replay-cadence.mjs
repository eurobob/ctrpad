#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { spawn } from "node:child_process";
import { performance } from "node:perf_hooks";

const FILE_MAGIC = 0x52525443;
const FRAME_MAGIC = 0x4d524652;
const FILE_MIN_VERSION = 2;
const FILE_MAX_VERSION = 4;
const HEADER_SIZE = 148;
const FRAME_SIZE = 440;
const FRAME_COUNT_OFFSET = 16;
const FRAME_INDEX_OFFSET = 4;
const END_ELAPSED_TIME_MS_OFFSET = 144;
const PAD_SNAPSHOTS_OFFSET = 248;
const PAD_SNAPSHOTS_SIZE = 48;
const VBLANK_TOTAL_OFFSET = 296;
const VBLANK_PACKET_COUNT_OFFSET = 300;
const VBLANK_PACKETS_OFFSET = 304;
const VBLANK_PACKET_CAP = 64;
const PAD_CHECKSUM_OFFSET = 432;
const RECORD_CHECKSUM_OFFSET = 436;
const FNV_OFFSET = 2166136261;
const FNV_PRIME = 16777619;

// platform/native_platform.c keeps this rational exact when it advances the
// host counter. Two VBlanks are one normal NTSC-U game frame.
const NATIVE_VBLANK_GPU_CYCLES = 897619;
const NATIVE_GPU_CLOCK_HZ = 53693175;
const TARGET_VBLANK_HZ =
  NATIVE_GPU_CLOCK_HZ / NATIVE_VBLANK_GPU_CYCLES;
const TARGET_TWO_VBLANK_FRAME_HZ = TARGET_VBLANK_HZ / 2;

function usage() {
  console.error(
    "usage: tools/analyze-replay-cadence.mjs [--log ctr-native.log] " +
      "[--measure-executable EXECUTABLE --start-checkpoint INDEX] " +
      "input.ctrreplay",
  );
}

function fail(message) {
  throw new Error(message);
}

function fnv1a(buffer, start, size, zeroStart = -1, zeroSize = 0) {
  let hash = FNV_OFFSET;
  const end = start + size;
  const zeroEnd = zeroStart + zeroSize;

  for (let offset = start; offset < end; offset += 1) {
    const value =
      offset >= zeroStart && offset < zeroEnd ? 0 : buffer[offset];
    hash = Math.imul(hash ^ value, FNV_PRIME) >>> 0;
  }

  return hash;
}

function addHistogramValue(histogram, value) {
  histogram.set(value, (histogram.get(value) ?? 0) + 1);
}

function formatHistogram(histogram) {
  return [...histogram.entries()]
    .sort(([left], [right]) => left - right)
    .map(([value, count]) => `${value}:${count}`)
    .join(",");
}

function parseArgs(argv) {
  let logPath;
  let measureExecutable;
  let startCheckpointText;
  const paths = [];

  for (let index = 2; index < argv.length; index += 1) {
    if (argv[index] === "--log") {
      logPath = argv[++index];
      if (logPath === undefined) {
        fail("missing --log value");
      }
    } else if (argv[index] === "--measure-executable") {
      measureExecutable = argv[++index];
      if (measureExecutable === undefined) {
        fail("missing --measure-executable value");
      }
    } else if (argv[index] === "--start-checkpoint") {
      startCheckpointText = argv[++index];
      if (startCheckpointText === undefined) {
        fail("missing --start-checkpoint value");
      }
    } else if (argv[index].startsWith("--")) {
      fail(`unknown option: ${argv[index]}`);
    } else {
      paths.push(argv[index]);
    }
  }

  if (paths.length !== 1) {
    usage();
    process.exit(1);
  }

  if (
    (measureExecutable === undefined) !==
    (startCheckpointText === undefined)
  ) {
    fail(
      "--measure-executable and --start-checkpoint must be supplied together",
    );
  }
  if (
    startCheckpointText !== undefined &&
    !/^(0|[1-9][0-9]*)$/.test(startCheckpointText)
  ) {
    fail(`invalid checkpoint index: ${startCheckpointText}`);
  }
  const startCheckpoint =
    startCheckpointText === undefined ? undefined : Number(startCheckpointText);
  if (
    startCheckpoint !== undefined &&
    (!Number.isSafeInteger(startCheckpoint) || startCheckpoint > 0xffffffff)
  ) {
    fail(`checkpoint index is outside the replay u32 range: ${startCheckpointText}`);
  }

  return {
    logPath,
    measureExecutable,
    replayPath: paths[0],
    startCheckpoint,
  };
}

function decodeVblank(buffer, version, recordOffset, frame) {
  const encodedCount = buffer.readUInt32LE(
    recordOffset + VBLANK_PACKET_COUNT_OFFSET,
  );
  const packetCount = version >= 4 ? encodedCount & 0xffff : encodedCount;
  const preFramePacketCount = version >= 4 ? encodedCount >>> 16 : 0;
  if (
    packetCount > VBLANK_PACKET_CAP ||
    preFramePacketCount > packetCount
  ) {
    fail(`invalid VSync packet boundary at frame ${frame}`);
  }

  let preFrameTotal = 0;
  let inFrameTotal = 0;
  for (let packetIndex = 0; packetIndex < packetCount; packetIndex += 1) {
    const encodedPacket = buffer.readUInt16LE(
      recordOffset + VBLANK_PACKETS_OFFSET + packetIndex * 2,
    );
    const packet = version >= 4 ? encodedPacket & 0xff : encodedPacket;
    const repeatCount = version >= 4 ? (encodedPacket >>> 8) + 1 : 1;
    if (packet === 0) {
      fail(`zero VSync packet at frame ${frame}`);
    }

    if (packetIndex < preFramePacketCount) {
      preFrameTotal += packet * repeatCount;
    } else {
      inFrameTotal += packet * repeatCount;
    }
  }

  if (
    preFrameTotal + inFrameTotal !==
    buffer.readUInt32LE(recordOffset + VBLANK_TOTAL_OFFSET)
  ) {
    fail(`VSync total mismatch at frame ${frame}`);
  }

  return { inFrameTotal, preFrameTotal };
}

function readReplay(replayPath) {
  const buffer = fs.readFileSync(replayPath);
  if (buffer.length < HEADER_SIZE || buffer.readUInt32LE(0) !== FILE_MAGIC) {
    fail("invalid replay header");
  }

  const version = buffer.readUInt32LE(4);
  if (version < FILE_MIN_VERSION || version > FILE_MAX_VERSION) {
    fail(`unsupported replay version ${version}`);
  }
  if (
    buffer.readUInt32LE(8) !== HEADER_SIZE ||
    buffer.readUInt32LE(12) !== FRAME_SIZE
  ) {
    fail("unexpected replay header or frame size");
  }

  const frameCount = buffer.readUInt32LE(FRAME_COUNT_OFFSET);
  if (
    frameCount === 0 ||
    buffer.length !== HEADER_SIZE + frameCount * FRAME_SIZE
  ) {
    fail("replay size does not match a nonempty declared frame count");
  }

  const elapsedHistogram = new Map();
  const preFrameVblankHistogram = new Map();
  const inFrameVblankHistogram = new Map();
  let elapsedTimeTotal = 0;
  let preFrameVblankTotal = 0;
  let inFrameVblankTotal = 0;
  let postBootstrapElapsedTimeTotal = 0;
  let postBootstrapInFrameVblankTotal = 0;
  let postBootstrapStandardFrames = 0;
  const vblankByFrame = [];

  for (let frame = 0; frame < frameCount; frame += 1) {
    const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
    if (
      buffer.readUInt32LE(recordOffset) !== FRAME_MAGIC ||
      buffer.readUInt32LE(recordOffset + FRAME_INDEX_OFFSET) !== frame
    ) {
      fail(`invalid or non-sequential record at frame ${frame}`);
    }

    const padChecksum = fnv1a(
      buffer,
      recordOffset + PAD_SNAPSHOTS_OFFSET,
      PAD_SNAPSHOTS_SIZE,
    );
    if (
      padChecksum !== buffer.readUInt32LE(recordOffset + PAD_CHECKSUM_OFFSET)
    ) {
      fail(`pad checksum mismatch at frame ${frame}`);
    }

    const recordChecksum = fnv1a(
      buffer,
      recordOffset,
      FRAME_SIZE,
      recordOffset + RECORD_CHECKSUM_OFFSET,
      4,
    );
    if (
      recordChecksum !==
      buffer.readUInt32LE(recordOffset + RECORD_CHECKSUM_OFFSET)
    ) {
      fail(`record checksum mismatch at frame ${frame}`);
    }

    const elapsedTime = buffer.readInt32LE(
      recordOffset + END_ELAPSED_TIME_MS_OFFSET,
    );
    const vblank = decodeVblank(buffer, version, recordOffset, frame);
    vblankByFrame.push(vblank.preFrameTotal + vblank.inFrameTotal);

    elapsedTimeTotal += elapsedTime;
    preFrameVblankTotal += vblank.preFrameTotal;
    inFrameVblankTotal += vblank.inFrameTotal;
    addHistogramValue(elapsedHistogram, elapsedTime);
    addHistogramValue(preFrameVblankHistogram, vblank.preFrameTotal);
    addHistogramValue(inFrameVblankHistogram, vblank.inFrameTotal);

    if (frame > 0) {
      postBootstrapElapsedTimeTotal += elapsedTime;
      postBootstrapInFrameVblankTotal += vblank.inFrameTotal;
      if (elapsedTime === 32 && vblank.inFrameTotal === 2) {
        postBootstrapStandardFrames += 1;
      }
    }
  }

  return {
    displayPath:
      path.relative(process.cwd(), replayPath) || path.basename(replayPath),
    elapsedHistogram,
    elapsedTimeTotal,
    frameCount,
    inFrameVblankHistogram,
    inFrameVblankTotal,
    postBootstrapElapsedTimeTotal,
    postBootstrapInFrameVblankTotal,
    postBootstrapStandardFrames,
    preFrameVblankHistogram,
    preFrameVblankTotal,
    vblankByFrame,
    version,
  };
}

function readFpsSamples(logPath) {
  const text = fs.readFileSync(logPath, "utf8");
  return [...text.matchAll(/\[CTR Native\] FPS: ([0-9]+(?:\.[0-9]+)?)/g)].map(
    (match) => Number(match[1]),
  );
}

function mean(values) {
  return values.reduce((total, value) => total + value, 0) / values.length;
}

function measureWallCadence(executablePath, replayPath, checkpointIndex, replay) {
  return new Promise((resolve, reject) => {
    const lineBufferPath = "/usr/bin/stdbuf";
    if (!fs.existsSync(lineBufferPath)) {
      reject(
        new Error(
          `${lineBufferPath} is required so replay markers are timestamped ` +
            "when emitted rather than at process exit",
        ),
      );
      return;
    }

    const executable = path.resolve(executablePath);
    const absoluteReplayPath = path.resolve(replayPath);
    fs.accessSync(executable, fs.constants.X_OK);

    const child = spawn(
      lineBufferPath,
      [
        "-oL",
        "-eL",
        executable,
        "--replay",
        absoluteReplayPath,
        "--replay-start-checkpoint",
        String(checkpointIndex),
      ],
      {
        cwd: path.dirname(executable),
        stdio: ["ignore", "pipe", "pipe"],
      },
    );

    const processStart = performance.now();
    let restoredAt;
    let restoredFrame;
    let replayFinishedAt;
    let pending = "";

    function scan(chunk) {
      pending += chunk;
      let lineEnd;
      while ((lineEnd = pending.indexOf("\n")) >= 0) {
        const line = pending.slice(0, lineEnd);
        pending = pending.slice(lineEnd + 1);
        const now = performance.now();
        const restoreMatch = line.match(
          /\[CTR State\] restored replay checkpoint #([0-9]+) replayFrame=([0-9]+)/,
        );
        if (restoreMatch !== null) {
          if (Number(restoreMatch[1]) !== checkpointIndex) {
            child.kill();
            reject(new Error(`restored unexpected checkpoint: ${line}`));
            return;
          }
          restoredAt = now;
          restoredFrame = Number(restoreMatch[2]);
          console.log(
            `wallMarker restoreSeconds=${((now - processStart) / 1000).toFixed(6)} ` +
              `checkpoint=${checkpointIndex} frame=${restoredFrame}`,
          );
        }

        const finishMatch = line.match(
          /\[CTR Replay\] replay finished after ([0-9]+) frames/,
        );
        if (finishMatch !== null) {
          if (Number(finishMatch[1]) !== replay.frameCount) {
            child.kill();
            reject(new Error(`replay finished at unexpected frame: ${line}`));
            return;
          }
          replayFinishedAt = now;
        }
      }
    }

    child.stdout.setEncoding("utf8");
    child.stderr.setEncoding("utf8");
    child.stdout.on("data", scan);
    child.stderr.on("data", scan);
    child.on("error", reject);
    child.on("exit", (exitCode, signal) => {
      if (exitCode !== 0 || signal !== null) {
        reject(
          new Error(
            `measured playback exited ${exitCode ?? "null"} ` +
              `signal=${signal ?? "none"}`,
          ),
        );
        return;
      }
      if (
        restoredAt === undefined ||
        restoredFrame === undefined ||
        replayFinishedAt === undefined
      ) {
        reject(new Error("measured playback did not emit both cadence markers"));
        return;
      }
      if (restoredFrame >= replay.frameCount) {
        reject(new Error(`restored frame ${restoredFrame} is outside replay`));
        return;
      }

      const measuredFrameCount = replay.frameCount - restoredFrame;
      const measuredVblankCount = replay.vblankByFrame
        .slice(restoredFrame)
        .reduce((total, value) => total + value, 0);
      const expectedSeconds = measuredVblankCount / TARGET_VBLANK_HZ;
      const observedSeconds = (replayFinishedAt - restoredAt) / 1000;
      const deviationSeconds = observedSeconds - expectedSeconds;
      const deviationPercent = (deviationSeconds * 100) / expectedSeconds;
      const observedFrameHz = measuredFrameCount / observedSeconds;

      console.log(
        `wallCadence frames=${measuredFrameCount} ` +
          `vblanks=${measuredVblankCount} ` +
          `expectedSeconds=${expectedSeconds.toFixed(6)} ` +
          `observedSeconds=${observedSeconds.toFixed(6)}`,
      );
      console.log(
        `wallCadence deviationSeconds=${deviationSeconds.toFixed(6)} ` +
          `deviationPercent=${deviationPercent.toFixed(6)} ` +
          `observedFrameHz=${observedFrameHz.toFixed(9)}`,
      );
      resolve();
    });
  });
}

try {
  const options = parseArgs(process.argv);
  const absoluteReplayPath = path.resolve(options.replayPath);
  const replay = readReplay(absoluteReplayPath);
  const postBootstrapFrameCount = replay.frameCount - 1;
  const standardPercent =
    (replay.postBootstrapStandardFrames * 100) / postBootstrapFrameCount;
  const logicStepsPerElapsedSecond =
    postBootstrapFrameCount /
    (replay.postBootstrapElapsedTimeTotal / 1000);
  const cadenceFromVblank =
    (postBootstrapFrameCount * TARGET_VBLANK_HZ) /
    replay.postBootstrapInFrameVblankTotal;

  console.log(
    `replay=${replay.displayPath} version=${replay.version} ` +
      `frames=${replay.frameCount}`,
  );
  console.log(`targetVblankHz=${TARGET_VBLANK_HZ.toFixed(9)}`);
  console.log(
    `targetTwoVblankFrameHz=${TARGET_TWO_VBLANK_FRAME_HZ.toFixed(9)}`,
  );
  console.log(
    `elapsedTimeMS total=${replay.elapsedTimeTotal} ` +
      `histogram=${formatHistogram(replay.elapsedHistogram)}`,
  );
  console.log(
    `preFrameVblank total=${replay.preFrameVblankTotal} ` +
      `histogram=${formatHistogram(replay.preFrameVblankHistogram)}`,
  );
  console.log(
    `inFrameVblank total=${replay.inFrameVblankTotal} ` +
      `histogram=${formatHistogram(replay.inFrameVblankHistogram)}`,
  );
  console.log(
    `postBootstrap frames=${postBootstrapFrameCount} ` +
      `standard32msTwoVblank=${replay.postBootstrapStandardFrames} ` +
      `standardPercent=${standardPercent.toFixed(6)}`,
  );
  console.log(
    `postBootstrap logicStepsPerElapsedSecond=` +
      `${logicStepsPerElapsedSecond.toFixed(9)} ` +
      `cadenceFromVblankHz=${cadenceFromVblank.toFixed(9)}`,
  );

  if (options.logPath !== undefined) {
    const fpsSamples = readFpsSamples(path.resolve(options.logPath));
    if (fpsSamples.length === 0) {
      fail("log contains no native FPS samples");
    }

    const afterFirst = fpsSamples.slice(1);
    console.log(
      `fpsLog samples=${fpsSamples.length} values=${fpsSamples.join(",")}`,
    );
    console.log(
      `fpsLogAll mean=${mean(fpsSamples).toFixed(6)} ` +
        `min=${Math.min(...fpsSamples).toFixed(2)} ` +
        `max=${Math.max(...fpsSamples).toFixed(2)}`,
    );
    if (afterFirst.length > 0) {
      console.log(
        `fpsLogAfterFirst samples=${afterFirst.length} ` +
          `mean=${mean(afterFirst).toFixed(6)} ` +
          `min=${Math.min(...afterFirst).toFixed(2)} ` +
          `max=${Math.max(...afterFirst).toFixed(2)}`,
      );
    }
  }

  if (options.measureExecutable !== undefined) {
    await measureWallCadence(
      options.measureExecutable,
      absoluteReplayPath,
      options.startCheckpoint,
      replay,
    );
  }
} catch (error) {
  console.error(`[CTR ReplayCadence] ${error.message}`);
  process.exit(1);
}
