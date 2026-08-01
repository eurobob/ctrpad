#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const ICON_HEADER_SIZE = 0x100;
const SAVE_PAYLOAD_SIZE = 0x1680;
const MEMCARD_PROFILE_SIZE = 0x1600;
const MEMCARD_PROFILE_VERSION = -18;
const NATIVE_SAVE_SIZE = ICON_HEADER_SIZE + SAVE_PAYLOAD_SIZE;

const GAME_PROGRESS_OFFSET = 0x144;
const GAME_PROGRESS_UNLOCKS_OFFSET = 0x4;
const SCRAPBOOK_UNLOCK_BIT = 36;
const SCRAPBOOK_UNLOCK_WORD = SCRAPBOOK_UNLOCK_BIT >> 5;
const SCRAPBOOK_UNLOCK_MASK = 1 << (SCRAPBOOK_UNLOCK_BIT & 0x1f);
const SCRAPBOOK_UNLOCK_OFFSET =
  ICON_HEADER_SIZE +
  GAME_PROGRESS_OFFSET +
  GAME_PROGRESS_UNLOCKS_OFFSET +
  SCRAPBOOK_UNLOCK_WORD * 4;

function usage() {
  console.error(
    "usage: tools/prepare-scrapbook-test-save.mjs SOURCE_SAVE OUTPUT_SAVE",
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

function crc16(buffer) {
  let crc = 0;
  for (const byte of buffer) {
    crc = crc16Byte(crc, byte);
  }
  return crc;
}

function validateSave(save, label) {
  if (save.length !== NATIVE_SAVE_SIZE) {
    fail(
      `${label}: unexpected file size ${save.length}; ` +
        `expected ${NATIVE_SAVE_SIZE}`,
    );
  }
  if (save[0] !== 0x53 || save[1] !== 0x43 || save[3] !== 1) {
    fail(`${label}: invalid one-block PlayStation memory-card icon header`);
  }

  const payload = save.subarray(ICON_HEADER_SIZE);
  if (payload.readInt16LE(0) !== MEMCARD_PROFILE_VERSION) {
    fail(`${label}: unexpected memory-card profile version`);
  }
  if (payload.readUInt16LE(2) !== MEMCARD_PROFILE_SIZE) {
    fail(`${label}: unexpected memory-card profile size`);
  }
  if (crc16(payload) !== 0) {
    fail(`${label}: retail CRC remainder is not zero`);
  }
}

try {
  if (process.argv.length !== 4) {
    usage();
    process.exit(1);
  }

  const sourcePath = path.resolve(process.argv[2]);
  const outputPath = path.resolve(process.argv[3]);
  if (sourcePath === outputPath) {
    fail("source and output paths must differ; in-place edits are refused");
  }

  const save = fs.readFileSync(sourcePath);
  validateSave(save, "source");

  const oldUnlockWord = save.readUInt32LE(SCRAPBOOK_UNLOCK_OFFSET);
  const newUnlockWord = oldUnlockWord | SCRAPBOOK_UNLOCK_MASK;
  save.writeUInt32LE(newUnlockWord, SCRAPBOOK_UNLOCK_OFFSET);

  const payload = save.subarray(ICON_HEADER_SIZE);
  let crc = crc16(payload.subarray(0, payload.length - 2));
  crc = crc16Byte(crc, 0);
  crc = crc16Byte(crc, 0);
  payload[payload.length - 2] = crc >> 8;
  payload[payload.length - 1] = crc;

  validateSave(save, "output");
  if (
    (save.readUInt32LE(SCRAPBOOK_UNLOCK_OFFSET) & SCRAPBOOK_UNLOCK_MASK) ===
    0
  ) {
    fail("output validation did not observe the Scrapbook unlock bit");
  }

  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, save, { flag: "wx" });

  console.log(`source=${sourcePath}`);
  console.log(`output=${outputPath}`);
  console.log(
    `scrapbookUnlockOffset=0x${SCRAPBOOK_UNLOCK_OFFSET.toString(16)} ` +
      `oldWord=0x${oldUnlockWord.toString(16)} ` +
      `newWord=0x${newUnlockWord.toString(16)}`,
  );
  console.log("crcRemainder=0x0");
} catch (error) {
  console.error(`[CTR ScrapbookSave] ${error.message}`);
  process.exit(1);
}
