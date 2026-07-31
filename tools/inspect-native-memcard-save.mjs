#!/usr/bin/env node

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const ICON_HEADER_SIZE = 0x100;
const SAVE_PAYLOAD_SIZE = 0x1680;
const MEMCARD_PROFILE_SIZE = 0x1600;
const MEMCARD_PROFILE_VERSION = -18;
const NATIVE_SAVE_SIZE = ICON_HEADER_SIZE + SAVE_PAYLOAD_SIZE;

function usage() {
  console.error(
    "usage: tools/inspect-native-memcard-save.mjs BASCUS-94426-SLOTS",
  );
}

function fail(message) {
  throw new Error(message);
}

function crc16Byte(crc, nextByte) {
  for (let bitIndex = 7; bitIndex >= 0; bitIndex -= 1) {
    const shifted = crc << 1;
    crc = shifted | ((nextByte >> bitIndex) & 1);
    if ((shifted & 0x10000) !== 0) {
      crc ^= 0x11021;
    }
  }
  return crc;
}

try {
  if (process.argv.length !== 3) {
    usage();
    process.exit(1);
  }

  const savePath = path.resolve(process.argv[2]);
  const save = fs.readFileSync(savePath);
  if (save.length !== NATIVE_SAVE_SIZE) {
    fail(
      `unexpected file size ${save.length}; expected ${NATIVE_SAVE_SIZE} ` +
        "bytes (0x100 icon plus 0x1680 payload)",
    );
  }
  if (save[0] !== 0x53 || save[1] !== 0x43) {
    fail("memory-card icon header does not begin with SC");
  }
  if (save[3] !== 1) {
    fail(`unexpected memory-card block count ${save[3]}; expected 1`);
  }

  const payload = save.subarray(ICON_HEADER_SIZE);
  const profileVersion = payload.readInt16LE(0);
  const profileSize = payload.readUInt16LE(2);
  if (profileVersion !== MEMCARD_PROFILE_VERSION) {
    fail(
      `unexpected profile version ${profileVersion}; ` +
        `expected ${MEMCARD_PROFILE_VERSION}`,
    );
  }
  if (profileSize !== MEMCARD_PROFILE_SIZE) {
    fail(
      `unexpected profile size 0x${profileSize.toString(16)}; ` +
        `expected 0x${MEMCARD_PROFILE_SIZE.toString(16)}`,
    );
  }

  let crc = 0;
  for (const byte of payload) {
    crc = crc16Byte(crc, byte);
  }
  if (crc !== 0) {
    fail(`retail CRC remainder is 0x${crc.toString(16)}; expected zero`);
  }

  const sha256 = crypto.createHash("sha256").update(save).digest("hex");
  console.log(
    `save=${path.relative(process.cwd(), savePath) || path.basename(savePath)}`,
  );
  console.log(
    `bytes=${save.length} iconBytes=${ICON_HEADER_SIZE} ` +
      `payloadBytes=${payload.length} blocks=${save[3]}`,
  );
  console.log(
    `profileVersion=${profileVersion} ` +
      `profileSize=0x${profileSize.toString(16)} ` +
      `crcRemainder=0x${crc.toString(16)}`,
  );
  console.log(`sha256=${sha256}`);
} catch (error) {
  console.error(`[CTR MemcardInspect] ${error.message}`);
  process.exit(1);
}
