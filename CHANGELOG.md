# Changelog

## 0.2.0 (unreleased)

### Fixed

- SHA-256: four of the 64 round constants were mistyped, so every hash differed from standard SHA-256. The constants now match FIPS 180-4, and tests check the standard vectors.
- `verify_chain()` compared stored hashes without recomputing them, so an edited entry passed. It now recomputes every hash and checks every link.
- The hashed bytes are now the entry's canonical JSON without its `chain_hash` member. The old `|`-joined string let text move between `device_id` and `timestamp` without changing the hash.
- CMake: the library target used a reserved `::` name and configure failed. The target is `DeviceTrustShim` with the alias `dts::DeviceTrustShim`.
- Missing `<vector>` and `<ctime>` includes.

### Added

- `verify_chain_to_anchor()`, which also catches entries removed from the end of the log.
- `install()` of the headers and a CMake package, so `find_package(DeviceTrustShim 0.2)` works.
- Options `DTS_BUILD_EXAMPLES` and `DTS_BUILD_TESTS`. Both default on only when DTS is the top-level project.
- Tests: 53 checks covering the hash, the chain, each kind of edit, JSON escaping and every adapter.
- CI on Ubuntu (GCC, Clang), macOS (Clang) and Windows (MSVC), on every push.

### Changed

- The build is warning-free with `-Wall -Wextra -Wpedantic`.
- README, CONTRIBUTING and the integration guide rewritten. Comments no longer claim that any standard is met.

## First push, 2025-12-30

- Header-only `AuditChain` with a built-in SHA-256, JSON entries, chain verification, five adapters, examples, a CMake build and one test. No release was tagged.
