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
const END_ELAPSED_TIME_MS_OFFSET = 144;
const PAD_SNAPSHOTS_OFFSET = 248;
const PAD_SNAPSHOT_SIZE = 12;
const PAD_TRANSPORT_SIZE = 9;
const PAD_COUNT = 4;
const VBLANK_TOTAL_OFFSET = 296;
const VBLANK_PACKET_COUNT_OFFSET = 300;
const VBLANK_PACKETS_OFFSET = 304;
const VBLANK_PACKET_CAP = 64;
const PAD_CHECKSUM_OFFSET = 432;
const RECORD_CHECKSUM_OFFSET = 436;
const FNV_OFFSET = 2166136261;
const FNV_PRIME = 16777619;

function usage() {
  console.error(
    "usage: tools/compare-replay-transport-semantics.mjs " +
      "reference.ctrreplay candidate.ctrreplay",
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

function readReplay(filePath) {
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
  if (
    buffer.readUInt32LE(8) !== HEADER_SIZE ||
    buffer.readUInt32LE(12) !== FRAME_SIZE
  ) {
    fail(`${filePath}: unexpected replay header or frame size`);
  }

  const frameCount = buffer.readUInt32LE(FRAME_COUNT_OFFSET);
  if (buffer.length !== HEADER_SIZE + frameCount * FRAME_SIZE) {
    fail(`${filePath}: replay size does not match its declared frame count`);
  }

  for (let frame = 0; frame < frameCount; frame += 1) {
    const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
    if (
      buffer.readUInt32LE(recordOffset) !== FRAME_MAGIC ||
      buffer.readUInt32LE(recordOffset + FRAME_INDEX_OFFSET) !== frame
    ) {
      fail(`${filePath}: invalid or non-sequential record at frame ${frame}`);
    }

    const padChecksum = fnv1a(
      buffer,
      recordOffset + PAD_SNAPSHOTS_OFFSET,
      PAD_SNAPSHOT_SIZE * PAD_COUNT,
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

    decodeVblankTransport({ buffer, version }, frame);
  }

  return {
    buffer,
    displayPath: path.relative(process.cwd(), filePath) || path.basename(filePath),
    frameCount,
    version,
  };
}

function decodeVblankTransport(replay, frame) {
  const recordOffset = HEADER_SIZE + frame * FRAME_SIZE;
  const encodedCount = replay.buffer.readUInt32LE(
    recordOffset + VBLANK_PACKET_COUNT_OFFSET,
  );
  const packetCount =
    replay.version >= 4 ? encodedCount & 0xffff : encodedCount;
  const preFramePacketCount = replay.version >= 4 ? encodedCount >>> 16 : 0;

  if (
    packetCount > VBLANK_PACKET_CAP ||
    preFramePacketCount > packetCount
  ) {
    fail(`${replay.displayPath ?? "replay"}: invalid VSync boundary at frame ${frame}`);
  }

  const preFrame = [];
  const inFrame = [];
  let expandedTotal = 0;

  for (let packetIndex = 0; packetIndex < packetCount; packetIndex += 1) {
    const encodedPacket = replay.buffer.readUInt16LE(
      recordOffset + VBLANK_PACKETS_OFFSET + packetIndex * 2,
    );
    const packet = replay.version >= 4 ? encodedPacket & 0xff : encodedPacket;
    const repeatCount = replay.version >= 4 ? (encodedPacket >>> 8) + 1 : 1;
    if (packet === 0) {
      fail(`${replay.displayPath ?? "replay"}: zero VSync packet at frame ${frame}`);
    }

    const destination =
      packetIndex < preFramePacketCount ? preFrame : inFrame;
    for (let repeat = 0; repeat < repeatCount; repeat += 1) {
      destination.push(packet);
      expandedTotal += packet;
    }
  }

  const recordedTotal = replay.buffer.readUInt32LE(
    recordOffset + VBLANK_TOTAL_OFFSET,
  );
  if (expandedTotal !== recordedTotal) {
    fail(
      `${replay.displayPath ?? "replay"}: VSync total mismatch at frame ${frame}`,
    );
  }

  return { inFrame, preFrame, recordedTotal };
}

function bytesEqual(reference, candidate, referenceOffset, candidateOffset, size) {
  return reference.buffer
    .subarray(referenceOffset, referenceOffset + size)
    .equals(candidate.buffer.subarray(candidateOffset, candidateOffset + size));
}

function arraysEqual(reference, candidate) {
  return (
    reference.length === candidate.length &&
    reference.every((value, index) => value === candidate[index])
  );
}

function compare(reference, candidate) {
  if (reference.frameCount !== candidate.frameCount) {
    fail(
      `frame-count mismatch: ${reference.frameCount} versus ${candidate.frameCount}`,
    );
  }

  const mismatches = {
    padTransport: 0,
    elapsedTime: 0,
    vblankTotal: 0,
    rawVblankBlock: 0,
    expandedPreFrameVblank: 0,
    expandedInFrameVblank: 0,
  };

  for (let frame = 0; frame < reference.frameCount; frame += 1) {
    const referenceOffset = HEADER_SIZE + frame * FRAME_SIZE;
    const candidateOffset = HEADER_SIZE + frame * FRAME_SIZE;

    let padTransportEqual = true;
    for (let pad = 0; pad < PAD_COUNT; pad += 1) {
      if (
        !bytesEqual(
          reference,
          candidate,
          referenceOffset +
            PAD_SNAPSHOTS_OFFSET +
            pad * PAD_SNAPSHOT_SIZE,
          candidateOffset +
            PAD_SNAPSHOTS_OFFSET +
            pad * PAD_SNAPSHOT_SIZE,
          PAD_TRANSPORT_SIZE,
        )
      ) {
        padTransportEqual = false;
        break;
      }
    }
    if (!padTransportEqual) {
      mismatches.padTransport += 1;
    }

    if (
      reference.buffer.readInt32LE(
        referenceOffset + END_ELAPSED_TIME_MS_OFFSET,
      ) !==
      candidate.buffer.readInt32LE(
        candidateOffset + END_ELAPSED_TIME_MS_OFFSET,
      )
    ) {
      mismatches.elapsedTime += 1;
    }

    if (
      !bytesEqual(
        reference,
        candidate,
        referenceOffset + VBLANK_TOTAL_OFFSET,
        candidateOffset + VBLANK_TOTAL_OFFSET,
        PAD_CHECKSUM_OFFSET - VBLANK_TOTAL_OFFSET,
      )
    ) {
      mismatches.rawVblankBlock += 1;
    }

    const referenceVblank = decodeVblankTransport(reference, frame);
    const candidateVblank = decodeVblankTransport(candidate, frame);
    if (referenceVblank.recordedTotal !== candidateVblank.recordedTotal) {
      mismatches.vblankTotal += 1;
    }
    if (
      !arraysEqual(referenceVblank.preFrame, candidateVblank.preFrame)
    ) {
      mismatches.expandedPreFrameVblank += 1;
    }
    if (!arraysEqual(referenceVblank.inFrame, candidateVblank.inFrame)) {
      mismatches.expandedInFrameVblank += 1;
    }
  }

  return mismatches;
}

try {
  if (process.argv.length !== 4) {
    usage();
    process.exit(1);
  }

  const reference = readReplay(path.resolve(process.argv[2]));
  const candidate = readReplay(path.resolve(process.argv[3]));
  const mismatches = compare(reference, candidate);

  console.log(
    `reference=${reference.displayPath} version=${reference.version} ` +
      `frames=${reference.frameCount}`,
  );
  console.log(
    `candidate=${candidate.displayPath} version=${candidate.version} ` +
      `frames=${candidate.frameCount}`,
  );
  for (const [name, mismatchCount] of Object.entries(mismatches)) {
    console.log(`${name}: equal=${reference.frameCount - mismatchCount} mismatched=${mismatchCount}`);
  }

  const semanticMismatchCount =
    mismatches.padTransport +
    mismatches.elapsedTime +
    mismatches.vblankTotal +
    mismatches.expandedPreFrameVblank +
    mismatches.expandedInFrameVblank;
  process.exit(semanticMismatchCount === 0 ? 0 : 2);
} catch (error) {
  console.error(`[CTR ReplayTransport] ${error.message}`);
  process.exit(1);
}
