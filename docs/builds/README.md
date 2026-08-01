# Build Evidence

Each attempted build gets a dated markdown record here containing:

- source commit and dirty state;
- host/container, architecture, compiler, CMake, generator, and SDK versions;
- exact configure, build, test, and run commands;
- exit codes and the first load-bearing error or warning;
- produced artifact identity;
- asset setup without retail filenames beyond the documented canonical path;
- what was not tested.

Raw logs may be kept locally under ignored `build-*` directories. Findings and
the commands needed to reproduce them belong in the dated record.
