#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const FILE_MAGIC = 0x52525443;
const FRAME_MAGIC = 0x4d524652;
const FILE_MIN_VERSION = 2;
const FILE_MAX_VERSION = 4;
const HEADER_SIZE = 148;
const FRAME_SIZE = 440;
const FRAME_COUNT_OFFSET = 16;
const FRAME_INDEX_OFFSET = 4;
const END_INFO_OFFSET = 128;
const DIGEST_OFFSET = END_INFO_OFFSET + 64;
const DIGEST_SCHEMA_OFFSET = DIGEST_OFFSET;
const DIGEST_MASK_OFFSET = DIGEST_OFFSET + 4;
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
const MAX_PRINTED_RANGES = 24;

const componentOffsets = new Map([
  ["timing", DIGEST_OFFSET + 8],
  ["rng", DIGEST_OFFSET + 16],
  ["drivers", DIGEST_OFFSET + 24],
  ["world", DIGEST_OFFSET + 32],
  ["allocation", DIGEST_OFFSET + 40],
  ["root", DIGEST_OFFSET + 48],
]);
const transportNames = ["pads", "vsync"];
const comparisonNames = [...componentOffsets.keys(), ...transportNames];

function usage() {
  console.error(
    "usage: tools/compare-replay-state-components.mjs [--prefix] " +
      "[--require name[,name...]] reference.ctrreplay candidate.ctrreplay",
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

function readReplay(filePath, allowInFlightRecord) {
  const buffer = fs.readFileSync(filePath);

  if (buffer.length < HEADER_SIZE) {
    fail(`${filePath}: replay is shorter than its header`);
  }
  if (buffer.readUInt32LE(0) !== FILE_MAGIC) {
    fail(`${filePath}: invalid replay file magic`);
  }
  const version = buffer.readUInt32LE(4);
  if (version < FILE_MIN_VERSION || version > FILE_MAX_VERSION) {
    fail(`${filePath}: unsupported replay version ${version}`);
  }
  if (buffer.readUInt32LE(8) !== HEADER_SIZE) {
    fail(`${filePath}: unexpected replay header size`);
  }
  if (buffer.readUInt32LE(12) !== FRAME_SIZE) {
    fail(`${filePath}: unexpected replay frame size`);
  }

  const frameCount = buffer.readUInt32LE(FRAME_COUNT_OFFSET);
  const expectedSize = HEADER_SIZE + frameCount * FRAME_SIZE;
  if (buffer.length < expectedSize) {
    fail(
      `${filePath}: header declares ${frameCount} frames but the file contains ` +
        `${Math.floor((buffer.length - HEADER_SIZE) / FRAME_SIZE)}`,
    );
  }
  if (buffer.length !== expectedSize) {
    const trailingBytes = buffer.length - expectedSize;
    if (!allowInFlightRecord || trailingBytes > FRAME_SIZE) {
      fail(`${filePath}: ${trailingBytes} trailing bytes after declared frames`);
    }
  }

  for (let frame = 0; frame < frameCount; frame += 1) {
    const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
    if (buffer.readUInt32LE(recordOffset) !== FRAME_MAGIC) {
      fail(`${filePath}: invalid record magic at frame ${frame}`);
    }
    if (buffer.readUInt32LE(recordOffset + FRAME_INDEX_OFFSET) !== frame) {
      fail(`${filePath}: non-sequential record index at frame ${frame}`);
    }

    const padChecksum = fnv1a(
      buffer,
      recordOffset + PAD_SNAPSHOTS_OFFSET,
      PAD_SNAPSHOTS_SIZE,
    );
    if (
      padChecksum !== buffer.readUInt32LE(recordOffset + PAD_CHECKSUM_OFFSET)
    ) {
      fail(`${filePath}: pad checksum mismatch at frame ${frame}`);
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
      fail(`${filePath}: record checksum mismatch at frame ${frame}`);
    }

    const encodedVblankCount = buffer.readUInt32LE(
      recordOffset + VBLANK_PACKET_COUNT_OFFSET,
    );
    const vblankPacketCount =
      version >= 4 ? encodedVblankCount & 0xffff : encodedVblankCount;
    const preFrameVblankPacketCount =
      version >= 4 ? encodedVblankCount >>> 16 : 0;
    if (
      vblankPacketCount > VBLANK_PACKET_CAP ||
      preFrameVblankPacketCount > vblankPacketCount
    ) {
      fail(`${filePath}: invalid VSync packet boundary at frame ${frame}`);
    }
    let vblankTotal = 0;
    for (let packetIndex = 0; packetIndex < vblankPacketCount; packetIndex += 1) {
      const encodedPacket = buffer.readUInt16LE(
        recordOffset + VBLANK_PACKETS_OFFSET + packetIndex * 2,
      );
      const packet = version >= 4 ? encodedPacket & 0xff : encodedPacket;
      const repeatCount = version >= 4 ? (encodedPacket >>> 8) + 1 : 1;
      if (packet === 0) {
        fail(`${filePath}: zero VSync packet at frame ${frame}`);
      }
      vblankTotal += packet * repeatCount;
    }
    if (
      vblankTotal !== buffer.readUInt32LE(recordOffset + VBLANK_TOTAL_OFFSET)
    ) {
      fail(`${filePath}: VSync total mismatch at frame ${frame}`);
    }
  }

  return {
    path: filePath,
    displayPath: path.relative(process.cwd(), filePath) || path.basename(filePath),
    buffer,
    version,
    frameCount,
  };
}

function readDigestU32(replay, frame, memberOffset) {
  return replay.buffer.readUInt32LE(
    HEADER_SIZE + frame * FRAME_SIZE + memberOffset,
  );
}

function readComponent(replay, frame, memberOffset) {
  return replay.buffer.readBigUInt64LE(
    HEADER_SIZE + frame * FRAME_SIZE + memberOffset,
  );
}

function padSnapshotsEqual(reference, candidate, frame) {
  const referenceOffset =
    HEADER_SIZE + frame * FRAME_SIZE + PAD_SNAPSHOTS_OFFSET;
  const candidateOffset =
    HEADER_SIZE + frame * FRAME_SIZE + PAD_SNAPSHOTS_OFFSET;

  return reference.buffer
    .subarray(referenceOffset, referenceOffset + PAD_SNAPSHOTS_SIZE)
    .equals(
      candidate.buffer.subarray(
        candidateOffset,
        candidateOffset + PAD_SNAPSHOTS_SIZE,
      ),
    );
}

function readVblankPacketCounts(replay, recordOffset) {
  const encoded = replay.buffer.readUInt32LE(
    recordOffset + VBLANK_PACKET_COUNT_OFFSET,
  );

  return {
    packetCount: replay.version >= 4 ? encoded & 0xffff : encoded,
    preFramePacketCount: replay.version >= 4 ? encoded >>> 16 : 0,
  };
}

function vsyncBoundaryEqual(reference, candidate, frame) {
  const referenceOffset = HEADER_SIZE + frame * FRAME_SIZE;
  const candidateOffset = HEADER_SIZE + frame * FRAME_SIZE;
  const referenceCounts = readVblankPacketCounts(reference, referenceOffset);
  const candidateCounts = readVblankPacketCounts(candidate, candidateOffset);

  if (
    reference.buffer.readUInt32LE(referenceOffset + VBLANK_TOTAL_OFFSET) !==
      candidate.buffer.readUInt32LE(candidateOffset + VBLANK_TOTAL_OFFSET) ||
    referenceCounts.packetCount !== candidateCounts.packetCount ||
    referenceCounts.preFramePacketCount !==
      candidateCounts.preFramePacketCount
  ) {
    return false;
  }

  const packetBytes = referenceCounts.packetCount * 2;
  return reference.buffer
    .subarray(
      referenceOffset + VBLANK_PACKETS_OFFSET,
      referenceOffset + VBLANK_PACKETS_OFFSET + packetBytes,
    )
    .equals(
      candidate.buffer.subarray(
        candidateOffset + VBLANK_PACKETS_OFFSET,
        candidateOffset + VBLANK_PACKETS_OFFSET + packetBytes,
      ),
    );
}

function addMismatchRange(summary, frame) {
  const current = summary.ranges.at(-1);
  if (current !== undefined && current.end + 1 === frame) {
    current.end = frame;
    return;
  }
  summary.ranges.push({ start: frame, end: frame });
}

function formatRanges(ranges) {
  const visible = ranges.slice(0, MAX_PRINTED_RANGES);
  const text = visible
    .map((range) =>
      range.start === range.end ? `${range.start}` : `${range.start}-${range.end}`,
    )
    .join(",");

  if (ranges.length <= MAX_PRINTED_RANGES) {
    return text || "none";
  }
  return `${text},...(+${ranges.length - MAX_PRINTED_RANGES} ranges)`;
}

function parseArgs(argv) {
  let allowPrefix = false;
  let required = [];
  const paths = [];

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === "--prefix") {
      allowPrefix = true;
      continue;
    }
    if (arg === "--require") {
      index += 1;
      if (index >= argv.length) {
        fail("--require needs a comma-separated component list");
      }
      required = argv[index].split(",").filter(Boolean);
      continue;
    }
    if (arg.startsWith("--")) {
      fail(`unknown option: ${arg}`);
    }
    paths.push(arg);
  }

  if (paths.length !== 2) {
    usage();
    process.exit(64);
  }

  for (const component of required) {
    if (!comparisonNames.includes(component)) {
      fail(`unknown required component: ${component}`);
    }
  }

  return { allowPrefix, required, paths };
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  const reference = readReplay(args.paths[0], false);
  const candidate = readReplay(args.paths[1], args.allowPrefix);

  if (!args.allowPrefix && reference.frameCount !== candidate.frameCount) {
    fail(
      `frame-count mismatch: reference=${reference.frameCount} ` +
        `candidate=${candidate.frameCount}`,
    );
  }
  if (args.allowPrefix && candidate.frameCount > reference.frameCount) {
    fail(
      `candidate is not a prefix: reference=${reference.frameCount} ` +
        `candidate=${candidate.frameCount}`,
    );
  }

  const comparedFrames = args.allowPrefix
    ? candidate.frameCount
    : reference.frameCount;
  const summaries = new Map();
  for (const name of comparisonNames) {
    summaries.set(name, { equal: 0, mismatched: 0, ranges: [] });
  }

  for (let frame = 0; frame < comparedFrames; frame += 1) {
    const referenceSchema = readDigestU32(
      reference,
      frame,
      DIGEST_SCHEMA_OFFSET,
    );
    const candidateSchema = readDigestU32(
      candidate,
      frame,
      DIGEST_SCHEMA_OFFSET,
    );
    const referenceMask = readDigestU32(reference, frame, DIGEST_MASK_OFFSET);
    const candidateMask = readDigestU32(candidate, frame, DIGEST_MASK_OFFSET);
    if (
      referenceSchema !== candidateSchema ||
      referenceMask !== candidateMask
    ) {
      fail(
        `digest schema/mask mismatch at frame ${frame}: ` +
          `reference=${referenceSchema}/0x${referenceMask.toString(16)} ` +
          `candidate=${candidateSchema}/0x${candidateMask.toString(16)}`,
      );
    }

    for (const [name, memberOffset] of componentOffsets) {
      const summary = summaries.get(name);
      if (
        readComponent(reference, frame, memberOffset) ===
        readComponent(candidate, frame, memberOffset)
      ) {
        summary.equal += 1;
      } else {
        summary.mismatched += 1;
        addMismatchRange(summary, frame);
      }
    }

    for (const [name, equal] of [
      ["pads", padSnapshotsEqual(reference, candidate, frame)],
      ["vsync", vsyncBoundaryEqual(reference, candidate, frame)],
    ]) {
      const summary = summaries.get(name);
      if (equal) {
        summary.equal += 1;
      } else {
        summary.mismatched += 1;
        addMismatchRange(summary, frame);
      }
    }
  }

  console.log(
    `[CTR ReplayCompare] reference=${reference.displayPath} version=${reference.version} frames=${reference.frameCount}`,
  );
  console.log(
    `[CTR ReplayCompare] candidate=${candidate.displayPath} version=${candidate.version} frames=${candidate.frameCount} ` +
      `compared=${comparedFrames}${args.allowPrefix ? " prefix=yes" : ""}`,
  );
  for (const [name, summary] of summaries) {
    console.log(
      `[CTR ReplayCompare] ${name}: equal=${summary.equal} ` +
        `mismatched=${summary.mismatched} ranges=${formatRanges(summary.ranges)}`,
    );
  }

  const failed = args.required.filter(
    (name) => summaries.get(name).mismatched !== 0,
  );
  if (failed.length !== 0) {
    console.error(
      `[CTR ReplayCompare] required components mismatched: ${failed.join(",")}`,
    );
    process.exit(2);
  }
  if (args.required.length !== 0) {
    console.log(
      `[CTR ReplayCompare] required components match: ${args.required.join(",")}`,
    );
  }
}

try {
  main();
} catch (error) {
  console.error(`[CTR ReplayCompare] ${error.message}`);
  process.exit(1);
}
