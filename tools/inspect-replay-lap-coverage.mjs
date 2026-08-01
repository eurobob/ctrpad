#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const STATE_FILE_MAGIC = 0x54535443;
const STATE_FILE_VERSION = 1;
const STATE_FILE_HEADER_SIZE = 32;
const STATE_RECORD_HEADER_SIZE = 32;
const CHECKPOINT_MAGIC = 0x43525443;
const REGION_SDATA = 0x54414453;
const FNV_OFFSET = 2166136261;
const FNV_PRIME = 16777619;

const layouts = new Map([
  [
    "2:4",
    {
      addressRangeCountOffset: 44,
      addressRangesOffset: 52,
      addressRangeSize: 12,
      addressRangeStartOffset: 4,
      addressRangeStartSize: 4,
      addressRangeRegionSizeOffset: 8,
      regionsOffset: 244,
      sdataGameTrackerOffset: 0x9bb4,
      gameTrackerDriversOffset: 0x24f4,
      driverLapIndexOffset: 0x44,
      driverCheckpointCurrentOffset: 0x495,
    },
  ],
  [
    "3:4",
    {
      addressRangeCountOffset: 52,
      addressRangesOffset: 68,
      addressRangeSize: 16,
      addressRangeStartOffset: 8,
      addressRangeStartSize: 8,
      addressRangeRegionSizeOffset: 4,
      regionsOffset: 324,
      sdataGameTrackerOffset: 0x9bb4,
      gameTrackerDriversOffset: 0x24f4,
      driverLapIndexOffset: 0x44,
      driverCheckpointCurrentOffset: 0x495,
    },
  ],
  [
    "3:8",
    {
      addressRangeCountOffset: 64,
      addressRangesOffset: 80,
      addressRangeSize: 16,
      addressRangeStartOffset: 8,
      addressRangeStartSize: 8,
      addressRangeRegionSizeOffset: 4,
      regionsOffset: 336,
      sdataGameTrackerOffset: 0xa618,
      gameTrackerDriversOffset: 0x2fb8,
      driverLapIndexOffset: 0x60,
      driverCheckpointCurrentOffset: 0x4fd,
    },
  ],
]);

function usage() {
  console.error(
    "usage: tools/inspect-replay-lap-coverage.mjs " +
      "[--driver 0-7] [--changes-only] state.ctrstates",
  );
}

function fail(message) {
  throw new Error(message);
}

function parseArgs(argv) {
  let driverIndex = 0;
  let changesOnly = false;
  let statePath = null;

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];

    if (arg === "--driver") {
      index += 1;
      if (index >= argv.length || !/^[0-7]$/.test(argv[index])) {
        fail("--driver requires an index from 0 through 7");
      }
      driverIndex = Number(argv[index]);
    } else if (arg === "--changes-only") {
      changesOnly = true;
    } else if (arg.startsWith("-")) {
      fail(`unknown option: ${arg}`);
    } else if (statePath === null) {
      statePath = arg;
    } else {
      fail("only one checkpoint file may be inspected");
    }
  }

  if (statePath === null) {
    usage();
    process.exit(1);
  }

  return { driverIndex, changesOnly, statePath };
}

function readU32(buffer, offset, context) {
  if (offset < 0 || offset + 4 > buffer.length) {
    fail(`${context}: u32 read is outside the payload at offset ${offset}`);
  }
  return buffer.readUInt32LE(offset);
}

function readPointer(buffer, offset, pointerSize, context) {
  if (pointerSize === 4) {
    return BigInt(readU32(buffer, offset, context));
  }
  if (offset < 0 || offset + 8 > buffer.length) {
    fail(`${context}: u64 read is outside the payload at offset ${offset}`);
  }
  return buffer.readBigUInt64LE(offset);
}

function checksum(buffer) {
  let hash = FNV_OFFSET;

  for (const value of buffer) {
    hash = Math.imul(hash ^ value, FNV_PRIME) >>> 0;
  }

  return hash;
}

function parseCheckpointPayload(payload, driverIndex, context) {
  if (readU32(payload, 0, context) !== CHECKPOINT_MAGIC) {
    fail(`${context}: invalid checkpoint payload magic`);
  }

  const checkpointVersion = readU32(payload, 4, context);
  const declaredSize = readU32(payload, 8, context);
  const regionCount = readU32(payload, 12, context);
  const pointerSize =
    checkpointVersion === 2 ? 4 : readU32(payload, 16, context);
  const layout = layouts.get(`${checkpointVersion}:${pointerSize}`);

  if (declaredSize !== payload.length) {
    fail(
      `${context}: checkpoint declares ${declaredSize} bytes, ` +
        `payload has ${payload.length}`,
    );
  }
  if (layout === undefined) {
    fail(
      `${context}: unsupported checkpoint/pointer version ` +
        `${checkpointVersion}/${pointerSize}`,
    );
  }
  if (regionCount !== 14) {
    fail(`${context}: expected 14 checkpoint regions, found ${regionCount}`);
  }

  const addressRangeCount = readU32(
    payload,
    layout.addressRangeCountOffset,
    context,
  );
  if (addressRangeCount > 16) {
    fail(
      `${context}: address range count ${addressRangeCount} exceeds capacity`,
    );
  }

  const addressRanges = [];
  for (let index = 0; index < addressRangeCount; index += 1) {
    const offset =
      layout.addressRangesOffset + index * layout.addressRangeSize;
    const kind = readU32(payload, offset, context);
    const size = readU32(
      payload,
      offset + layout.addressRangeRegionSizeOffset,
      context,
    );
    const start =
      layout.addressRangeStartSize === 4
        ? BigInt(
            readU32(
              payload,
              offset + layout.addressRangeStartOffset,
              context,
            ),
          )
        : readPointer(
            payload,
            offset + layout.addressRangeStartOffset,
            8,
            context,
          );

    addressRanges.push({ kind, size, start });
  }

  const regions = new Map();
  for (let index = 0; index < regionCount; index += 1) {
    const offset = layout.regionsOffset + index * 12;
    const kind = readU32(payload, offset, context);
    const regionOffset = readU32(payload, offset + 4, context);
    const size = readU32(payload, offset + 8, context);

    if (regionOffset + size > payload.length) {
      fail(`${context}: checkpoint region ${index} exceeds the payload`);
    }
    regions.set(kind, { offset: regionOffset, size });
  }

  const sdataRegion = regions.get(REGION_SDATA);
  if (sdataRegion === undefined) {
    fail(`${context}: checkpoint has no SDATA region`);
  }

  const driverSlotOffset =
    sdataRegion.offset +
    layout.sdataGameTrackerOffset +
    layout.gameTrackerDriversOffset +
    driverIndex * pointerSize;
  const driverAddress = readPointer(
    payload,
    driverSlotOffset,
    pointerSize,
    context,
  );

  if (driverAddress === 0n) {
    return {
      checkpointVersion,
      pointerSize,
      active: false,
      lapIndex: null,
      checkpointIndex: null,
    };
  }

  const owner = addressRanges.find(
    (range) =>
      driverAddress >= range.start &&
      driverAddress < range.start + BigInt(range.size),
  );
  if (owner === undefined) {
    fail(
      `${context}: driver ${driverIndex} pointer ` +
        `0x${driverAddress.toString(16)} has no checkpoint address owner`,
    );
  }

  const ownerRegion = regions.get(owner.kind);
  if (ownerRegion === undefined) {
    fail(`${context}: driver address owner has no captured region`);
  }

  const driverOffset =
    ownerRegion.offset + Number(driverAddress - owner.start);
  const lapIndexOffset = driverOffset + layout.driverLapIndexOffset;
  const checkpointIndexOffset =
    driverOffset + layout.driverCheckpointCurrentOffset;

  if (
    lapIndexOffset >= payload.length ||
    checkpointIndexOffset >= payload.length
  ) {
    fail(`${context}: resolved driver fields exceed the checkpoint payload`);
  }

  return {
    checkpointVersion,
    pointerSize,
    active: true,
    lapIndex: payload[lapIndexOffset],
    checkpointIndex: payload[checkpointIndexOffset],
  };
}

function inspectStateFile(args) {
  const buffer = fs.readFileSync(args.statePath);

  if (buffer.length < STATE_FILE_HEADER_SIZE) {
    fail("checkpoint file is shorter than its header");
  }
  if (buffer.readUInt32LE(0) !== STATE_FILE_MAGIC) {
    fail("invalid checkpoint file magic");
  }
  if (buffer.readUInt32LE(4) !== STATE_FILE_VERSION) {
    fail(`unsupported checkpoint file version ${buffer.readUInt32LE(4)}`);
  }
  if (
    buffer.readUInt32LE(8) !== STATE_FILE_HEADER_SIZE ||
    buffer.readUInt32LE(12) !== STATE_RECORD_HEADER_SIZE
  ) {
    fail("unexpected checkpoint file header sizes");
  }

  const recordCount = buffer.readUInt32LE(16);
  let offset = STATE_FILE_HEADER_SIZE;
  let activeRecords = 0;
  let maxLap = -1;
  let maxCheckpoint = -1;
  let checkpointVersion = null;
  let pointerSize = null;
  let previousPrintedState = null;

  console.log(
    `[CTR LapCoverage] file=${path.relative(process.cwd(), args.statePath)} ` +
      `records=${recordCount} driver=${args.driverIndex}`,
  );
  console.log("checkpoint frame active lap checkpoint");

  for (let recordIndex = 0; recordIndex < recordCount; recordIndex += 1) {
    if (offset + STATE_RECORD_HEADER_SIZE > buffer.length) {
      fail(`record ${recordIndex}: header exceeds the checkpoint file`);
    }

    const checkpointIndex = buffer.readUInt32LE(offset);
    const replayFrame = buffer.readUInt32LE(offset + 4);
    const payloadOffset = buffer.readUInt32LE(offset + 8);
    const payloadSize = buffer.readUInt32LE(offset + 12);
    const expectedChecksum = buffer.readUInt32LE(offset + 16);

    if (checkpointIndex !== recordIndex) {
      fail(
        `record ${recordIndex}: non-sequential checkpoint index ` +
          `${checkpointIndex}`,
      );
    }
    if (payloadOffset !== offset + STATE_RECORD_HEADER_SIZE) {
      fail(`record ${recordIndex}: unexpected payload offset ${payloadOffset}`);
    }
    if (payloadOffset + payloadSize > buffer.length) {
      fail(`record ${recordIndex}: payload exceeds the checkpoint file`);
    }

    const payload = buffer.subarray(payloadOffset, payloadOffset + payloadSize);
    const actualChecksum = checksum(payload);
    if (actualChecksum !== expectedChecksum) {
      fail(
        `record ${recordIndex}: checksum mismatch ` +
          `expected=0x${expectedChecksum.toString(16)} ` +
          `actual=0x${actualChecksum.toString(16)}`,
      );
    }

    const result = parseCheckpointPayload(
      payload,
      args.driverIndex,
      `record ${recordIndex}`,
    );
    if (checkpointVersion === null) {
      checkpointVersion = result.checkpointVersion;
      pointerSize = result.pointerSize;
    } else if (
      checkpointVersion !== result.checkpointVersion ||
      pointerSize !== result.pointerSize
    ) {
      fail(`record ${recordIndex}: checkpoint layout changed within the file`);
    }

    const currentState = result.active
      ? `${result.lapIndex}:${result.checkpointIndex}`
      : "inactive";
    if (!args.changesOnly || currentState !== previousPrintedState) {
      console.log(
        `${checkpointIndex} ${replayFrame} ${result.active ? "yes" : "no"} ` +
          `${result.active ? result.lapIndex : "-"} ` +
          `${result.active ? result.checkpointIndex : "-"}`,
      );
      previousPrintedState = currentState;
    }

    if (result.active) {
      activeRecords += 1;
      maxLap = Math.max(maxLap, result.lapIndex);
      maxCheckpoint = Math.max(maxCheckpoint, result.checkpointIndex);
    }

    offset = payloadOffset + payloadSize;
  }

  if (offset !== buffer.length) {
    fail(`${buffer.length - offset} trailing bytes follow the last checkpoint`);
  }

  console.log(
    `[CTR LapCoverage] checkpointVersion=${checkpointVersion} ` +
      `pointerSize=${pointerSize} activeRecords=${activeRecords} ` +
      `maxLap=${maxLap >= 0 ? maxLap : "none"} ` +
      `maxCheckpoint=${maxCheckpoint >= 0 ? maxCheckpoint : "none"} ` +
      `lapAdvanced=${maxLap >= 1 ? "yes" : "no"}`,
  );
}

try {
  inspectStateFile(parseArgs(process.argv.slice(2)));
} catch (error) {
  console.error(`[CTR LapCoverage] ${error.message}`);
  process.exit(1);
}
