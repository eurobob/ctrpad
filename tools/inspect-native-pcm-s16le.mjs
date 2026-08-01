#!/usr/bin/env node

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const SAMPLE_RATE = 44100;
const CHANNELS = 2;
const BYTES_PER_SAMPLE = 2;
const FRAME_BYTES = CHANNELS * BYTES_PER_SAMPLE;
const DBFS_REFERENCE = 32768;

function usage() {
  console.error(
    "usage: tools/inspect-native-pcm-s16le.mjs AUDIO.raw",
  );
}

function fail(message) {
  throw new Error(message);
}

function decibelsFullScale(value) {
  if (value === 0) {
    return "-Infinity";
  }
  return (20 * Math.log10(value / DBFS_REFERENCE)).toFixed(6);
}

try {
  if (process.argv.length !== 3) {
    usage();
    process.exit(1);
  }

  const pcmPath = path.resolve(process.argv[2]);
  const pcm = fs.readFileSync(pcmPath);
  if (pcm.length === 0) {
    fail("PCM capture is empty");
  }
  if (pcm.length % FRAME_BYTES !== 0) {
    fail(
      `PCM capture has ${pcm.length} bytes; expected a multiple of ` +
        `${FRAME_BYTES} for stereo S16LE`,
    );
  }

  const frameCount = pcm.length / FRAME_BYTES;
  const stats = Array.from({ length: CHANNELS }, () => ({
    min: 32767,
    max: -32768,
    nonzeroSamples: 0,
    fullScaleSamples: 0,
    sumSquares: 0,
  }));
  let stereoDifferentFrames = 0;

  for (let frame = 0; frame < frameCount; frame += 1) {
    const frameOffset = frame * FRAME_BYTES;
    const left = pcm.readInt16LE(frameOffset);
    const right = pcm.readInt16LE(frameOffset + BYTES_PER_SAMPLE);
    const samples = [left, right];

    if (left !== right) {
      stereoDifferentFrames += 1;
    }

    for (let channel = 0; channel < CHANNELS; channel += 1) {
      const sample = samples[channel];
      const channelStats = stats[channel];

      channelStats.min = Math.min(channelStats.min, sample);
      channelStats.max = Math.max(channelStats.max, sample);
      channelStats.sumSquares += sample * sample;
      if (sample !== 0) {
        channelStats.nonzeroSamples += 1;
      }
      if (sample === -32768 || sample === 32767) {
        channelStats.fullScaleSamples += 1;
      }
    }
  }

  if (stats.some((channelStats) => channelStats.nonzeroSamples === 0)) {
    fail("at least one PCM channel is completely silent");
  }

  const sha256 = crypto.createHash("sha256").update(pcm).digest("hex");
  console.log(
    `pcm=${path.relative(process.cwd(), pcmPath) || path.basename(pcmPath)}`,
  );
  console.log(
    `bytes=${pcm.length} frames=${frameCount} ` +
      `durationSeconds=${(frameCount / SAMPLE_RATE).toFixed(6)} ` +
      `sampleRate=${SAMPLE_RATE} channels=${CHANNELS} format=S16LE`,
  );

  for (let channel = 0; channel < CHANNELS; channel += 1) {
    const channelStats = stats[channel];
    const rms = Math.sqrt(channelStats.sumSquares / frameCount);
    const peak = Math.max(
      Math.abs(channelStats.min),
      Math.abs(channelStats.max),
    );
    const label = channel === 0 ? "left" : "right";

    console.log(
      `${label} min=${channelStats.min} max=${channelStats.max} ` +
        `nonzeroSamples=${channelStats.nonzeroSamples} ` +
        `fullScaleSamples=${channelStats.fullScaleSamples} ` +
        `rmsDbfs=${decibelsFullScale(rms)} ` +
        `peakDbfs=${decibelsFullScale(peak)}`,
    );
  }

  console.log(`stereoDifferentFrames=${stereoDifferentFrames}`);
  console.log(`sha256=${sha256}`);
} catch (error) {
  console.error(`[CTR PCMInspect] ${error.message}`);
  process.exit(1);
}
