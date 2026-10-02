# Device Trust Shim

A header-only C++17 library for tamper-evident device logs. Each log entry is one line of JSON that carries the SHA-256 hash of the entry before it, so a verifier can tell when the log was changed.

**Status: prototype.** 53 checks pass with Apple clang 21 on macOS (October 2026). The CMake build, install and a `find_package` consumer were verified the same day.
CI builds and tests on Ubuntu (GCC and Clang), macOS (Clang) and Windows (MSVC) on every push.

[![CI](https://github.com/cyber-physical-engineering/device_trust_shim/actions/workflows/ci.yml/badge.svg)](https://github.com/cyber-physical-engineering/device_trust_shim/actions/workflows/ci.yml)

James Thornton set the architecture and requirements. The code was written with AI-assisted development in late 2025. The tests and checks were re-run in October 2026.

## What it does

- `AuditChain::log()` returns one JSON entry per event: device ID, timestamp, user, severity, message, the previous entry's hash and this entry's hash.
- `AuditChain::verify_chain()` recomputes every hash. It catches an edited, deleted, inserted or reordered entry.
- `AuditChain::verify_chain_to_anchor()` also catches entries dropped from the end of the log, if you saved the latest chain hash somewhere else.
- SHA-256 is built in and checked against the FIPS 180-4 test vectors. The library has no outside dependency.
- Five optional adapters format messages for DICOM and PACS systems, medical devices, clinical-trial devices, industrial control (PLC, SCADA and nine protocols) and building automation (KNX, BACnet).

## Quick start

Build the examples and run the tests:

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

Run an example:

```bash
./build/radiology_device_example
```

Log and verify in your own code:

```cpp
#include <dts/audit_chain.hpp>
#include <string>
#include <vector>

int main() {
    dts::AuditChain logger("PACS-2025-001234");

    std::vector<std::string> entries;
    entries.push_back(logger.log("Device power-on", dts::UserID::System, dts::Severity::Info));
    entries.push_back(logger.log("Scan started", dts::UserID::Operator, dts::Severity::Info));

    bool ok = dts::AuditChain::verify_chain(entries);                                         // true
    bool anchored = dts::AuditChain::verify_chain_to_anchor(entries, logger.get_chain_hash()); // true
    return (ok && anchored) ? 0 : 1;
}
```

## Adding it to a project

Copy `include/dts/` into your project, or use CMake:

```cmake
add_subdirectory(device_trust_shim)
target_link_libraries(your_target PRIVATE dts::DeviceTrustShim)
```

Or install it and use the package:

```bash
cmake --install build --prefix /some/prefix
```

```cmake
find_package(DeviceTrustShim 0.2 REQUIRED)
target_link_libraries(your_target PRIVATE dts::DeviceTrustShim)
```

The examples and tests build only when this repository is the top-level project. The options `DTS_BUILD_EXAMPLES` and `DTS_BUILD_TESTS` override that.

## How it works

Each entry has this shape, in this key order:

```json
{"device_id":"PACS-2025-001234","timestamp":"2026-10-02T14:30:00.000Z","user_id":2,"severity":1,"message":"Scan started","previous_hash":"<64 hex>","chain_hash":"<64 hex>"}
```

`chain_hash` is the SHA-256 of the entry's own JSON without the `chain_hash` member. `previous_hash` is the `chain_hash` of the entry before it. The first entry's `previous_hash` is the SHA-256 of the string `DTS_INIT`. Strings are JSON-escaped, so no text can move between fields without changing the hash.

`verify_chain()` parses each entry, recomputes its hash and checks the link to the entry before it. It accepts only the exact format `log()` writes: the same keys, the same order, no extra whitespace. Anything else counts as not verifiable.

Timestamps are UTC with millisecond precision, from the system clock.

## Adapters

Each adapter wraps an `AuditChain` and formats a domain-specific message. The adapters format text; the library does not talk to any PLC, DICOM node or KNX bus itself.

| Adapter | Header | Formats |
|---|---|---|
| DICOM | `dts/adapters/dicom_adapter.hpp` | study created, instance stored, AI inference request and completion, transfer, access events |
| Medical device | `dts/adapters/medtech_adapter.hpp` | power-on self-test, medication events, safety alarms with a priority, calibration, firmware updates, maintenance |
| Clinical trial | `dts/adapters/clinical_trial_adapter.hpp` | enrollment, visits, data collection, protocol deviations, adverse events, data export |
| Industrial | `dts/adapters/industrial_adapter.hpp` | PLC program and I/O events, SCADA alarms, batch events, protocol events (Modbus, OPC UA, EtherNet/IP, Profinet, DNP3, IEC 61850, BACnet, KNX, MQTT), interlocks, equipment status, parameter changes, maintenance |
| Building automation | `dts/adapters/building_automation_adapter.hpp` | HVAC, lighting, access control, fire safety, energy, KNX, BACnet, schedules, elevators, security |

The clinical-trial adapter never writes the raw patient ID. By default it logs an unsalted SHA-256 of the ID cut to 16 hex characters. That is a pseudonym; it does not make the data anonymous. You can pass your own function instead.

Example, industrial:

```cpp
#include <dts/adapters/industrial_adapter.hpp>

dts::adapters::IndustrialAdapter plc("PLC-AB-1756-L75-001", "ASSET-001", "LINE-A");
plc.log_io_change("FILL_VALVE_OUT", "0", "1", true);
plc.log_protocol_event(dts::adapters::ProtocolType::Modbus, "192.168.1.10:502", "192.168.1.20:502", "Function 03");
```

Each file in `examples/` shows one adapter end to end.

## Measured

October 2026, Apple M1 Max, `-O2`, 100,000 entries with an industrial-adapter-sized message: 8.5 µs per `log()` call, 6.8 µs per entry to verify, and 347 bytes per entry on average. `sizeof(AuditChain)` is 64 bytes plus the device-ID string. That is a desktop CPU; expect different numbers on a device.

## Limits

- There is no secret key. Anyone who can rewrite the whole log can rebuild a valid chain. Pair the chain with a saved chain hash, a signature or write-once storage.
- Dropping the newest entries passes `verify_chain()`. Use `verify_chain_to_anchor()` with a chain hash you saved elsewhere.
- One writer at a time. There is no locking.
- The code uses `std::string` and iostreams. Very small microcontrollers are out of scope.
- The default patient-ID pseudonym is an unsalted, truncated hash. A short ID can be recovered by guessing.
- Log content is not encrypted, and there is no log rotation.
- The adapters borrow vocabulary from DICOM, IEC 60601-1-8 alarm priorities and industrial protocols. That is naming, not a statement that any standard is met.

## License

MIT. See [LICENSE](LICENSE).
