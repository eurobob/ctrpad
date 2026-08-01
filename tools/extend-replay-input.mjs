#!/usr/bin/env node

// This creates automation input for --record-from-replay. Appended records
// repeat pad/VSync transport; their copied state digests are not parity
// evidence. Only the fresh report produced from the seed can be evaluated.

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const FILE_MAGIC = 0x52525443;
const FRAME_MAGIC = 0x4d524652;
const REPLAY_VERSION = 4;
const HEADER_SIZE = 148;
const FRAME_SIZE = 440;
const FRAME_COUNT_OFFSET = 16;
const FRAME_INDEX_OFFSET = 4;
const PAD_SNAPSHOTS_OFFSET = 248;
const PAD_SNAPSHOTS_SIZE = 48;
const VBLANK_TOTAL_OFFSET = 296;
const VBLANK_PACKET_COUNT_OFFSET = 300;
const VBLANK_PACKETS_OFFSET = 304;
const VBLANK_PACKET_CAP = 64;
const END_ELAPSED_TIME_MS_OFFSET = 144;
const PAD_CHECKSUM_OFFSET = 432;
const RECORD_CHECKSUM_OFFSET = 436;
const FNV_OFFSET = 2166136261;
const FNV_PRIME = 16777619;

function usage() {
  console.error(
    "usage: tools/extend-replay-input.mjs --frames COUNT " +
      "[--repeat-from FRAME] [--pads-from REPLAY] " +
      "[--bootstrap-from V4_REPLAY] " +
      "source.ctrreplay output.ctrreplay",
  );
}

function fail(message) {
  throw new Error(message);
}

function parseUnsigned(text, name) {
  if (!/^(0|[1-9][0-9]*)$/.test(text ?? "")) {
    fail(`invalid ${name}: ${text ?? "(missing)"}`);
  }

  const value = Number(text);
  if (!Number.isSafeInteger(value) || value > 0xffffffff) {
    fail(`${name} is outside the replay u32 range: ${text}`);
  }
  return value;
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

function validateFrame(buffer, frame, replayVersion) {
  const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;

  if (buffer.readUInt32LE(recordOffset) !== FRAME_MAGIC) {
    fail(`invalid record magic at frame ${frame}`);
  }
  if (buffer.readUInt32LE(recordOffset + FRAME_INDEX_OFFSET) !== frame) {
    fail(`non-sequential record index at frame ${frame}`);
  }

  const padChecksum = fnv1a(
    buffer,
    recordOffset + PAD_SNAPSHOTS_OFFSET,
    PAD_SNAPSHOTS_SIZE,
  );
  if (padChecksum !== buffer.readUInt32LE(recordOffset + PAD_CHECKSUM_OFFSET)) {
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

  const encodedCount = buffer.readUInt32LE(
    recordOffset + VBLANK_PACKET_COUNT_OFFSET,
  );
  const packetCount =
    replayVersion >= 4 ? encodedCount & 0xffff : encodedCount;
  const preFramePacketCount = replayVersion >= 4 ? encodedCount >>> 16 : 0;
  if (
    packetCount > VBLANK_PACKET_CAP ||
    preFramePacketCount > packetCount
  ) {
    fail(`invalid VSync packet boundary at frame ${frame}`);
  }

  let vblankTotal = 0;
  for (let packetIndex = 0; packetIndex < packetCount; packetIndex += 1) {
    const encodedPacket = buffer.readUInt16LE(
      recordOffset + VBLANK_PACKETS_OFFSET + packetIndex * 2,
    );
    const packet = replayVersion >= 4 ? encodedPacket & 0xff : encodedPacket;
    const repeatCount = replayVersion >= 4 ? (encodedPacket >>> 8) + 1 : 1;
    if (packet === 0) {
      fail(`zero VSync packet at frame ${frame}`);
    }
    vblankTotal += packet * repeatCount;
  }
  if (
    vblankTotal !== buffer.readUInt32LE(recordOffset + VBLANK_TOTAL_OFFSET)
  ) {
    fail(`VSync total mismatch at frame ${frame}`);
  }
}

function parseArgs(argv) {
  let frameCountText;
  let bootstrapFromPath;
  let padsFromPath;
  let repeatFromText;
  const paths = [];

  for (let index = 2; index < argv.length; index += 1) {
    if (argv[index] === "--frames") {
      frameCountText = argv[++index];
      if (frameCountText === undefined) {
        fail("missing --frames value");
      }
    } else if (argv[index] === "--bootstrap-from") {
      bootstrapFromPath = argv[++index];
      if (bootstrapFromPath === undefined) {
        fail("missing --bootstrap-from value");
      }
    } else if (argv[index] === "--pads-from") {
      padsFromPath = argv[++index];
      if (padsFromPath === undefined) {
        fail("missing --pads-from value");
      }
    } else if (argv[index] === "--repeat-from") {
      repeatFromText = argv[++index];
      if (repeatFromText === undefined) {
        fail("missing --repeat-from value");
      }
    } else if (argv[index].startsWith("--")) {
      fail(`unknown option: ${argv[index]}`);
    } else {
      paths.push(argv[index]);
    }
  }

  if (frameCountText === undefined || paths.length !== 2) {
    usage();
    process.exit(1);
  }

  return {
    bootstrapFromPath,
    outputFrameCount: parseUnsigned(frameCountText, "frame count"),
    padsFromPath,
    repeatFromText,
    sourcePath: paths[0],
    outputPath: paths[1],
  };
}

try {
  const options = parseArgs(process.argv);
  const sourcePath = path.resolve(options.sourcePath);
  const outputPath = path.resolve(options.outputPath);

  if (sourcePath === outputPath) {
    fail("source and output paths must differ");
  }

  const source = fs.readFileSync(sourcePath);
  if (source.length < HEADER_SIZE) {
    fail("source replay is shorter than its header");
  }
  if (source.readUInt32LE(0) !== FILE_MAGIC) {
    fail("invalid replay file magic");
  }
  const sourceVersion = source.readUInt32LE(4);
  if (sourceVersion < 2 || sourceVersion > REPLAY_VERSION) {
    fail(`source replay version ${sourceVersion} cannot be promoted`);
  }
  if (
    source.readUInt32LE(8) !== HEADER_SIZE ||
    source.readUInt32LE(12) !== FRAME_SIZE
  ) {
    fail("unexpected replay header or frame size");
  }

  const sourceFrameCount = source.readUInt32LE(FRAME_COUNT_OFFSET);
  const expectedSourceSize = HEADER_SIZE + sourceFrameCount * FRAME_SIZE;
  if (source.length !== expectedSourceSize) {
    fail(
      `source size mismatch: header declares ${sourceFrameCount} frames, ` +
        `file has ${source.length} bytes`,
    );
  }
  if (options.outputFrameCount < sourceFrameCount) {
    fail(
      `output frame count ${options.outputFrameCount} is smaller than ` +
        `source frame count ${sourceFrameCount}`,
    );
  }
  if (sourceVersion < REPLAY_VERSION && options.bootstrapFromPath === undefined) {
    fail(
      `promoting replay version ${sourceVersion} requires ` +
        "--bootstrap-from with a complete version-4 timing boundary",
    );
  }

  for (let frame = 0; frame < sourceFrameCount; frame += 1) {
    validateFrame(source, frame, sourceVersion);
  }

  const repeatFrom =
    options.repeatFromText === undefined
      ? sourceFrameCount - 1
      : parseUnsigned(options.repeatFromText, "repeat frame");
  if (repeatFrom >= sourceFrameCount) {
    fail(
      `repeat frame ${repeatFrom} is outside source frame count ` +
        `${sourceFrameCount}`,
    );
  }

  const output = Buffer.alloc(
    HEADER_SIZE + options.outputFrameCount * FRAME_SIZE,
  );
  source.copy(output);
  output.writeUInt32LE(REPLAY_VERSION, 4);
  output.writeUInt32LE(options.outputFrameCount, FRAME_COUNT_OFFSET);

  if (options.bootstrapFromPath !== undefined) {
    const bootstrap = fs.readFileSync(options.bootstrapFromPath);
    if (
      bootstrap.length < HEADER_SIZE + FRAME_SIZE ||
      bootstrap.readUInt32LE(0) !== FILE_MAGIC ||
      bootstrap.readUInt32LE(4) !== REPLAY_VERSION ||
      bootstrap.readUInt32LE(8) !== HEADER_SIZE ||
      bootstrap.readUInt32LE(12) !== FRAME_SIZE
    ) {
      fail("bootstrap replay is not a complete version-4 replay");
    }
    validateFrame(bootstrap, 0, REPLAY_VERSION);

    const frameZeroOffset = HEADER_SIZE;
    bootstrap.copy(
      output,
      frameZeroOffset + VBLANK_TOTAL_OFFSET,
      frameZeroOffset + VBLANK_TOTAL_OFFSET,
      frameZeroOffset + PAD_CHECKSUM_OFFSET,
    );
    bootstrap.copy(
      output,
      frameZeroOffset + END_ELAPSED_TIME_MS_OFFSET,
      frameZeroOffset + END_ELAPSED_TIME_MS_OFFSET,
      frameZeroOffset + END_ELAPSED_TIME_MS_OFFSET + 4,
    );
    output.writeUInt32LE(0, frameZeroOffset + RECORD_CHECKSUM_OFFSET);
    output.writeUInt32LE(
      fnv1a(
        output,
        frameZeroOffset,
        FRAME_SIZE,
        frameZeroOffset + RECORD_CHECKSUM_OFFSET,
        4,
      ),
      frameZeroOffset + RECORD_CHECKSUM_OFFSET,
    );
  }

  if (options.padsFromPath !== undefined) {
    const padsFrom = fs.readFileSync(options.padsFromPath);
    if (padsFrom.length < HEADER_SIZE) {
      fail("pad-source replay is shorter than its header");
    }
    const padsFromVersion = padsFrom.readUInt32LE(4);
    const padsFromFrameCount = padsFrom.readUInt32LE(FRAME_COUNT_OFFSET);
    if (
      padsFrom.readUInt32LE(0) !== FILE_MAGIC ||
      padsFromVersion < 2 ||
      padsFromVersion > REPLAY_VERSION ||
      padsFrom.readUInt32LE(8) !== HEADER_SIZE ||
      padsFrom.readUInt32LE(12) !== FRAME_SIZE ||
      padsFromFrameCount !== sourceFrameCount ||
      padsFrom.length !== HEADER_SIZE + padsFromFrameCount * FRAME_SIZE
    ) {
      fail("pad-source replay layout or frame count does not match source");
    }

    for (let frame = 0; frame < padsFromFrameCount; frame += 1) {
      const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
      validateFrame(padsFrom, frame, padsFromVersion);
      padsFrom.copy(
        output,
        recordOffset + PAD_SNAPSHOTS_OFFSET,
        recordOffset + PAD_SNAPSHOTS_OFFSET,
        recordOffset + PAD_SNAPSHOTS_OFFSET + PAD_SNAPSHOTS_SIZE,
      );
      output.writeUInt32LE(
        fnv1a(
          output,
          recordOffset + PAD_SNAPSHOTS_OFFSET,
          PAD_SNAPSHOTS_SIZE,
        ),
        recordOffset + PAD_CHECKSUM_OFFSET,
      );
      output.writeUInt32LE(0, recordOffset + RECORD_CHECKSUM_OFFSET);
      output.writeUInt32LE(
        fnv1a(
          output,
          recordOffset,
          FRAME_SIZE,
          recordOffset + RECORD_CHECKSUM_OFFSET,
          4,
        ),
        recordOffset + RECORD_CHECKSUM_OFFSET,
      );
    }
  }

  const templateOffset = HEADER_SIZE + repeatFrom * FRAME_SIZE;
  for (
    let frame = sourceFrameCount;
    frame < options.outputFrameCount;
    frame += 1
  ) {
    const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
    output.copy(
      output,
      recordOffset,
      templateOffset,
      templateOffset + FRAME_SIZE,
    );
    output.writeUInt32LE(frame, recordOffset + FRAME_INDEX_OFFSET);
    output.writeUInt32LE(0, recordOffset + RECORD_CHECKSUM_OFFSET);
    output.writeUInt32LE(
      fnv1a(
        output,
        recordOffset,
        FRAME_SIZE,
        recordOffset + RECORD_CHECKSUM_OFFSET,
        4,
      ),
      recordOffset + RECORD_CHECKSUM_OFFSET,
    );
  }

  for (let frame = 0; frame < options.outputFrameCount; frame += 1) {
    validateFrame(output, frame, REPLAY_VERSION);
  }

  fs.writeFileSync(outputPath, output, { flag: "wx" });
  console.log(
    `[CTR ReplayExtend] source=${options.sourcePath} ` +
      `sourceVersion=${sourceVersion} sourceFrames=${sourceFrameCount} ` +
      `output=${options.outputPath} ` +
      `outputFrames=${options.outputFrameCount} repeatFrom=${repeatFrom} ` +
      `padsFrom=${options.padsFromPath ?? "(source)"} ` +
      `bootstrapFrom=${options.bootstrapFromPath ?? "(source)"}`,
  );
} catch (error) {
  console.error(`[CTR ReplayExtend] ${error.message}`);
  process.exit(1);
}
