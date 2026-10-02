# Integration Guide

This guide walks through integrating Device Trust Shim (DTS) into your medical device firmware or embedded system.

## Step 1: Include the Header

Copy `include/dts/audit_chain.hpp` into your project's include directory, or add the DTS repository as a submodule:

```bash
git submodule add https://github.com/cyber-physical-engineering/device_trust_shim.git third_party/dts
```

Then include in your code:

```cpp
#include <dts/audit_chain.hpp>
```

## Step 2: Initialize the Logger

Create a global or singleton `AuditChain` instance with your device's unique identifier:

```cpp
// In your device initialization code
static dts::AuditChain audit_logger("DEVICE-SERIAL-12345");
```

**Important**: Use a persistent device identifier (serial number, MAC address, etc.) that remains constant across reboots.

## Step 3: Log Events

Call `log()` at key points in your device's operation:

```cpp
// Device startup
audit_logger.log("Firmware version 1.2.3 loaded", 
                 dts::UserID::System, 
                 dts::Severity::Info);

// User actions
audit_logger.log("Admin authentication successful", 
                 dts::UserID::Admin, 
                 dts::Severity::Info);

// Critical events
audit_logger.log("Safety limit exceeded - operation halted", 
                 dts::UserID::System, 
                 dts::Severity::Critical);
```

## Step 4: Store Logs

DTS generates JSON-formatted log entries. Store them according to your device's capabilities:

### Option A: File System (if available)
```cpp
std::string entry = audit_logger.log("Event message");
write_to_file("/var/log/audit.log", entry + "\n");
```

### Option B: Flash Memory
```cpp
std::string entry = audit_logger.log("Event message");
flash_write(audit_sector, entry);
```

### Option C: Network Upload
```cpp
std::string entry = audit_logger.log("Event message");
upload_to_gateway(entry);  // Send to clinical_ai_gateway
```

## Step 5: Verify Chain Integrity

When logs are retrieved (e.g., during audit or incident investigation), verify integrity:

```cpp
std::vector<std::string> log_entries = load_logs_from_storage();

if (!dts::AuditChain::verify_chain(log_entries)) {
    // Tampering detected - logs have been modified or deleted
    handle_security_incident();
}
```

`verify_chain()` does not catch entries removed from the end of the log. If the device reports `get_chain_hash()` to a server after each batch, verify against that saved value instead:

```cpp
if (!dts::AuditChain::verify_chain_to_anchor(log_entries, saved_chain_hash)) {
    // The log was changed, or its newest entries are missing
}
```

## Integration Patterns

### Pattern 1: Wrapper Class

Wrap DTS in your device's logging abstraction:

```cpp
class DeviceLogger {
    dts::AuditChain chain_;
    
public:
    DeviceLogger(const std::string& device_id) : chain_(device_id) {}
    
    void log_event(const std::string& msg, UserRole role) {
        dts::UserID uid = map_user_role(role);
        std::string entry = chain_.log(msg, uid);
        persist_log(entry);
    }
};
```

### Pattern 2: Macro Wrapper

Create convenience macros for common logging patterns:

```cpp
#define LOG_INFO(msg) audit_logger.log(msg, dts::UserID::System, dts::Severity::Info)
#define LOG_ERROR(msg) audit_logger.log(msg, dts::UserID::System, dts::Severity::Error)
#define LOG_ADMIN(msg) audit_logger.log(msg, dts::UserID::Admin, dts::Severity::Info)

// Usage
LOG_INFO("Device initialized");
LOG_ERROR("Network connection failed");
```

### Pattern 3: Integration with Existing Logging

If you already have a logging system, integrate DTS as an integrity layer:

```cpp
class HybridLogger {
    dts::AuditChain integrity_chain_;
    YourExistingLogger* existing_logger_;
    
public:
    void log(const std::string& msg) {
        // Log to existing system
        existing_logger_->info(msg);
        
        // Add integrity chain
        std::string integrity_entry = integrity_chain_.log(msg);
        store_integrity_log(integrity_entry);
    }
};
```

## Storage Considerations

### Log Rotation

DTS doesn't handle log rotation automatically. Implement rotation at the application layer:

```cpp
const size_t MAX_LOG_SIZE = 10 * 1024 * 1024;  // 10 MB

if (get_log_file_size() > MAX_LOG_SIZE) {
    archive_current_log();
    start_new_log_file();
    // Note: Chain continues - previous_hash from last entry of archived log
}
```

### Compression

Compress logs before storage to reduce flash wear:

```cpp
std::string entry = audit_logger.log("Event");
std::vector<uint8_t> compressed = compress(entry);
flash_write(compressed);
```

### Circular Buffer

For devices with limited storage, implement a circular buffer with chain preservation:

```cpp
// Keep last N entries + chain hash of oldest entry
// This allows verification of the preserved chain segment
```

## Inside a quality system

DTS gives you one thing: a log a reviewer can check for changes. What the log must hold, how long it is kept, and who reviews it are decisions your quality system makes. Nothing in DTS satisfies a regulation by itself.

Three habits make the log useful during a review:

- Log the events a reviewer will ask about: version changes, calibration, alarms, operator actions.
- Save `get_chain_hash()` somewhere the device cannot reach, so missing entries are caught.
- Keep the entries exactly as `log()` wrote them. Reformatting breaks verification.

## Performance Optimization

### Minimize String Allocations

Pre-allocate buffers if logging frequency is high:

```cpp
thread_local std::string log_buffer;
log_buffer.reserve(512);  // Pre-allocate
```

### Batch Verification

Verify chains in batches rather than entry-by-entry:

```cpp
// Verify entire log file at once
std::vector<std::string> all_entries = load_all_logs();
bool valid = dts::AuditChain::verify_chain(all_entries);
```

## Troubleshooting

### Chain Verification Fails

- **Missing entries**: Check if logs were deleted or not uploaded completely
- **Out of order**: Ensure entries are processed in chronological order
- **Corrupted data**: Check storage integrity (flash wear, bit errors)

### Memory Constraints

- Use `get_chain_hash()` to checkpoint chain state periodically
- Store only chain hashes for old entries, full entries for recent ones
- Implement log archival to external storage

## Next Steps

- Review [examples/](examples/) for complete integration patterns
- See [README.md](../README.md) for API reference

