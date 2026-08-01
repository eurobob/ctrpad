#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const usage = `Usage: ${path.basename(process.argv[1])} I686_DEBUG_OBJECT [REPOSITORY_ROOT]`;

if (process.argv.length < 3 || process.argv.length > 4) {
  process.stderr.write(`${usage}\n`);
  process.exit(1);
}

const objectPath = path.resolve(process.argv[2]);
const repositoryRoot = path.resolve(process.argv[3] ?? process.cwd());
const sourceRoots = ["include", "game", "platform"];

const serializedRoots = new Set([
  "AnimTex",
  "BSP",
  "Level",
  "Model",
  "ModelAnim",
  "ModelHeader",
  "NavHeader",
  "QuadBlock",
  "SCVert",
  "Skybox",
  "SpawnType2",
]);

const fixedResidentRoots = new Set([
  "Data",
  "GameTracker",
  "OVR233_Garage",
  "OVR_230_VideoBSS",
  "Ovr233_Credits_BSS",
  "rData",
  "sData",
]);

function visitSourceFiles(directory, output) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) {
      visitSourceFiles(entryPath, output);
    } else if (entry.isFile() && /\.[ch]$/.test(entry.name)) {
      output.push(entryPath);
    }
  }
}

function sourceFiles() {
  const files = [];
  for (const sourceRoot of sourceRoots) {
    visitSourceFiles(path.join(repositoryRoot, sourceRoot), files);
  }
  return files.sort();
}

function collectSourceFacts(files) {
  const pinnedTypes = new Set();
  const scratchpadTypes = new Set();
  let ptr32References = 0;

  const pinnedPattern =
    /(?:offsetof|OFFSETOF|sizeof)\s*\(\s*(?:\(\s*)?(?:struct|union)\s+([A-Za-z_][A-Za-z0-9_]*)/g;
  const scratchpadPattern =
    /CTR_SCRATCHPAD_PTR\(\s*(?:struct|union)\s+([A-Za-z_][A-Za-z0-9_]*)/g;
  const ptr32Pattern = /\b[A-Za-z_][A-Za-z0-9_]*Ptr32\b/g;

  for (const file of files) {
    const source = fs.readFileSync(file, "utf8");
    for (const match of source.matchAll(pinnedPattern)) {
      pinnedTypes.add(match[1]);
    }
    for (const match of source.matchAll(scratchpadPattern)) {
      scratchpadTypes.add(match[1]);
    }
    ptr32References += [...source.matchAll(ptr32Pattern)].length;
  }

  return { pinnedTypes, scratchpadTypes, ptr32References };
}

function parseDwarf(object) {
  const output = execFileSync("dwarfdump", ["--debug-info", object], {
    encoding: "utf8",
    maxBuffer: 128 * 1024 * 1024,
    stdio: ["ignore", "pipe", "ignore"],
  });
  const dies = new Map();
  let current = null;
  const stack = [];

  for (const line of output.split(/\n/)) {
    const dieMatch = line.match(
      /^0x([0-9a-f]+):([ ]*)DW_TAG_([A-Za-z0-9_]+)/,
    );
    if (dieMatch) {
      const depth = Math.floor((dieMatch[2].length - 1) / 2);
      current = {
        offset: Number.parseInt(dieMatch[1], 16),
        tag: dieMatch[3],
        children: [],
      };
      while (stack.length > depth) {
        stack.pop();
      }
      if (depth > 0 && stack.length === depth) {
        stack[depth - 1].children.push(current);
      }
      stack[depth] = current;
      dies.set(current.offset, current);
      continue;
    }
    if (current === null) {
      continue;
    }

    let match = line.match(/DW_AT_name\s+\("(.*)"\)/);
    if (match) {
      current.name = match[1];
    }
    match = line.match(/DW_AT_type\s+\(0x([0-9a-f]+)/);
    if (match) {
      current.type = Number.parseInt(match[1], 16);
    }
    match = line.match(/DW_AT_decl_file\s+\("(.*)"\)/);
    if (match) {
      current.file = match[1];
    }
    match = line.match(/DW_AT_decl_line\s+\(([0-9]+)\)/);
    if (match) {
      current.line = Number.parseInt(match[1], 10);
    }
    match = line.match(/DW_AT_data_member_location\s+\(0x([0-9a-f]+)\)/);
    if (match) {
      current.location = Number.parseInt(match[1], 16);
    }
    match = line.match(/DW_AT_byte_size\s+\(0x([0-9a-f]+)\)/);
    if (match) {
      current.size = Number.parseInt(match[1], 16);
    }
  }

  return dies;
}

function unwrapType(dies, type, seen = new Set()) {
  if (type === undefined || type === null || seen.has(type.offset)) {
    return null;
  }
  seen.add(type.offset);
  if (
    ["typedef", "const_type", "volatile_type", "restrict_type", "array_type"].includes(
      type.tag,
    )
  ) {
    return unwrapType(dies, dies.get(type.type), seen);
  }
  return type;
}

function containsPointer(dies, type, seen = new Set()) {
  const unwrapped = unwrapType(dies, type);
  if (unwrapped === null || seen.has(unwrapped.offset)) {
    return false;
  }
  seen.add(unwrapped.offset);
  if (unwrapped.tag === "pointer_type") {
    return true;
  }
  if (
    unwrapped.tag === "structure_type" ||
    unwrapped.tag === "union_type"
  ) {
    return unwrapped.children.some(
      (child) =>
        child.tag === "member" &&
        containsPointer(dies, dies.get(child.type), new Set(seen)),
    );
  }
  return false;
}

function relativeSourcePath(file) {
  if (file === undefined) {
    return "?";
  }
  if (file.startsWith("/src/")) {
    return file.slice(5);
  }
  const relative = path.relative(repositoryRoot, file);
  return relative.startsWith("..") ? file : relative;
}

function rootStorage(root, scratchpadTypes) {
  if (serializedRoots.has(root.name)) {
    return {
      storage: "serialized_file",
      decision: "guest_ref32_checked_asset_region",
    };
  }
  if (scratchpadTypes.has(root.name)) {
    return {
      storage: "scratchpad_guest",
      decision: "host_sidecar_or_local_preserve_scratch_bytes",
    };
  }
  if (
    fixedResidentRoots.has(root.name) ||
    /^Overlay(?:RDATA|DATA)_/.test(root.name ?? "")
  ) {
    return {
      storage: "resident_map",
      decision: "native_host_pointer_view_preserve_guest_schema",
    };
  }
  return {
    storage: "runtime_host",
    decision: "widen_native_pointer_native_layout_assert",
  };
}

function pointerRowsForRoot(dies, root, scratchpadTypes) {
  const rows = [];
  const context = rootStorage(root, scratchpadTypes);

  function visit(aggregate, pathPrefix, seen) {
    if (aggregate === null || seen.has(aggregate.offset)) {
      return;
    }
    const nextSeen = new Set(seen);
    nextSeen.add(aggregate.offset);

    for (const member of aggregate.children.filter(
      (child) => child.tag === "member",
    )) {
      const memberName = member.name ?? `<anonymous@${member.line ?? "?"}>`;
      const memberPath =
        pathPrefix.length === 0 ? memberName : `${pathPrefix}.${memberName}`;
      const unwrapped = unwrapType(dies, dies.get(member.type));
      if (unwrapped === null) {
        continue;
      }
      if (unwrapped.tag === "pointer_type") {
        rows.push({
          root: root.name,
          rootFile: relativeSourcePath(root.file),
          rootLine: root.line ?? 0,
          path: memberPath,
          owner: aggregate.name ?? `<anonymous@${aggregate.line ?? "?"}>`,
          field: memberName,
          fieldFile: relativeSourcePath(member.file ?? aggregate.file),
          fieldLine: member.line ?? aggregate.line ?? 0,
          storage: context.storage,
          decision: context.decision,
        });
      } else if (
        (unwrapped.tag === "structure_type" ||
          unwrapped.tag === "union_type") &&
        containsPointer(dies, unwrapped)
      ) {
        visit(unwrapped, memberPath, nextSeen);
      }
    }
  }

  visit(root, "", new Set());
  return rows;
}

function tsv(value) {
  return String(value).replaceAll("\t", " ").replaceAll("\n", " ");
}

const files = sourceFiles();
const facts = collectSourceFacts(files);
const dies = parseDwarf(objectPath);
const rootsByIdentity = new Map();

for (const die of dies.values()) {
  if (
    (die.tag === "structure_type" || die.tag === "union_type") &&
    facts.pinnedTypes.has(die.name) &&
    containsPointer(dies, die)
  ) {
    const identity = `${die.name}|${die.file ?? ""}|${die.line ?? 0}`;
    if (!rootsByIdentity.has(identity)) {
      rootsByIdentity.set(identity, die);
    }
  }
}

const roots = [...rootsByIdentity.values()].sort(
  (left, right) =>
    relativeSourcePath(left.file).localeCompare(relativeSourcePath(right.file)) ||
    (left.line ?? 0) - (right.line ?? 0) ||
    left.name.localeCompare(right.name),
);
const rows = roots.flatMap((root) =>
  pointerRowsForRoot(dies, root, facts.scratchpadTypes),
);

process.stdout.write(
  [
    "root_type",
    "root_definition",
    "field_path",
    "declaring_type",
    "field_definition",
    "storage_owner",
    "migration_decision",
  ].join("\t") + "\n",
);
for (const row of rows) {
  process.stdout.write(
    [
      row.root,
      `${row.rootFile}:${row.rootLine}`,
      row.path,
      row.owner,
      `${row.fieldFile}:${row.fieldLine}`,
      row.storage,
      row.decision,
    ]
      .map(tsv)
      .join("\t") + "\n",
  );
}

const storageCounts = new Map();
for (const row of rows) {
  storageCounts.set(row.storage, (storageCounts.get(row.storage) ?? 0) + 1);
}
const counts = [...storageCounts.entries()]
  .sort(([left], [right]) => left.localeCompare(right))
  .map(([storage, count]) => `${storage}=${count}`)
  .join(" ");

process.stderr.write(
  `[CTR 64-bit census] pinned-types=${facts.pinnedTypes.size} ` +
    `pointer-bearing-root-definitions=${roots.length} field-contexts=${rows.length} ` +
    `scratchpad-types=${facts.scratchpadTypes.size} Ptr32-references=${facts.ptr32References} ${counts}\n`,
);
