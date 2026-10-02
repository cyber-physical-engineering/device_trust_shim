/**
 * @file audit_chain.hpp
 * @brief Tamper-evident audit logging for devices
 *
 * Device Trust Shim (DTS) writes each log entry as one line of JSON that
 * carries the SHA-256 hash of the entry before it. verify_chain() recomputes
 * every hash, so an edited, deleted, inserted or reordered entry is caught.
 *
 * Two limits. Removing the newest entries is caught only against a chain
 * hash saved elsewhere; see verify_chain_to_anchor(). And no secret key is
 * involved, so anyone who can rewrite the whole log can rebuild a valid chain.
 *
 * @copyright Copyright (c) 2025 Big Data Plumbing
 * @license MIT License
 */
#ifndef DTS_AUDIT_CHAIN_HPP
#define DTS_AUDIT_CHAIN_HPP

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace dts {

/**
 * @brief SHA-256 (FIPS 180-4), header-only. tests/test_audit_chain.cpp checks it against the standard test vectors.
 */
class SHA256 {
public:
    using Hash = std::array<uint8_t, 32>;
    
    static Hash hash(const uint8_t* data, size_t len) {
        SHA256 ctx;
        ctx.update(data, len);
        return ctx.finalize();
    }
    
    static Hash hash(const std::string& str) {
        return hash(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }
    
    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[buffer_len++] = data[i];
            if (buffer_len == 64) {
                transform();
                buffer_len = 0;
            }
        }
        bit_len += len * 8;
    }
    
    Hash finalize() {
        uint64_t bit_len_be = swap_endian(bit_len);
        uint8_t pad_len = (buffer_len < 56) ? (56 - buffer_len) : (120 - buffer_len);
        update(padding, pad_len);
        update(reinterpret_cast<const uint8_t*>(&bit_len_be), 8);
        
        Hash result;
        for (int i = 0; i < 8; ++i) {
            result[i * 4 + 0] = (h[i] >> 24) & 0xFF;
            result[i * 4 + 1] = (h[i] >> 16) & 0xFF;
            result[i * 4 + 2] = (h[i] >> 8) & 0xFF;
            result[i * 4 + 3] = (h[i] >> 0) & 0xFF;
        }
        return result;
    }
    
private:
    static constexpr uint8_t padding[64] = {0x80};
    
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    uint8_t buffer[64] = {0};
    uint8_t buffer_len = 0;
    uint64_t bit_len = 0;
    
    static constexpr uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    
    void transform() {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) |
                   (buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3];
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = right_rotate(w[i-15], 7) ^ right_rotate(w[i-15], 18) ^ (w[i-15] >> 3);
            uint32_t s1 = right_rotate(w[i-2], 17) ^ right_rotate(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3],
                 e = h[4], f = h[5], g = h[6], h_val = h[7];
        
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = right_rotate(e, 6) ^ right_rotate(e, 11) ^ right_rotate(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h_val + S1 + ch + k[i] + w[i];
            uint32_t S0 = right_rotate(a, 2) ^ right_rotate(a, 13) ^ right_rotate(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            
            h_val = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += h_val;
    }
    
    static uint32_t right_rotate(uint32_t value, uint32_t amount) {
        return (value >> amount) | (value << (32 - amount));
    }
    
    static uint64_t swap_endian(uint64_t value) {
        return ((value & 0xFF00000000000000ULL) >> 56) |
               ((value & 0x00FF000000000000ULL) >> 40) |
               ((value & 0x0000FF0000000000ULL) >> 24) |
               ((value & 0x000000FF00000000ULL) >> 8) |
               ((value & 0x00000000FF000000ULL) << 8) |
               ((value & 0x0000000000FF0000ULL) << 24) |
               ((value & 0x000000000000FF00ULL) << 40) |
               ((value & 0x00000000000000FFULL) << 56);
    }
};

/**
 * @brief User identifier enum for audit logging
 */
enum class UserID : uint8_t {
    System = 0,
    Admin = 1,
    Operator = 2,
    Service = 3,
    Unauthorized = 255
};

/**
 * @brief Event severity levels
 */
enum class Severity : uint8_t {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * @brief Hash-chained audit log for one device
 *
 * Each entry is the JSON object
 *   {"device_id":..,"timestamp":..,"user_id":..,"severity":..,"message":..,
 *    "previous_hash":..,"chain_hash":..}
 * chain_hash is the SHA-256 of that object without its chain_hash member.
 * previous_hash is the chain_hash of the entry before it, and the first
 * entry's previous_hash is SHA-256("DTS_INIT").
 *
 * One writer at a time. There is no locking.
 */
class AuditChain {
public:
    /**
     * @brief Start a chain for one device
     * @param device_id Unique device identifier (e.g., serial number)
     */
    explicit AuditChain(const std::string& device_id)
        : device_id_(device_id), previous_hash_(SHA256::hash("DTS_INIT")) {}

    /**
     * @brief Log an audit event
     * @param message Event description
     * @param user_id User who triggered the event
     * @param severity Event severity level
     * @return The entry as one line of JSON, ending in its chain hash
     */
    std::string log(const std::string& message,
                    UserID user_id = UserID::System,
                    Severity severity = Severity::Info) {
        const std::string timestamp = current_timestamp();
        const std::string previous_hex = hash_to_hex(previous_hash_);
        const std::string body = canonical_body(device_id_, timestamp,
                                                static_cast<int>(user_id),
                                                static_cast<int>(severity),
                                                message, previous_hex);
        const SHA256::Hash current_hash = SHA256::hash(body);

        // The entry is the hashed body with chain_hash added as the last member.
        std::string entry = body.substr(0, body.size() - 1);
        entry += ",\"chain_hash\":\"" + hash_to_hex(current_hash) + "\"}";

        previous_hash_ = current_hash;
        sequence_number_++;
        return entry;
    }

    /**
     * @brief The newest entry's chain hash. Save it somewhere else to anchor the log.
     */
    std::string get_chain_hash() const {
        return hash_to_hex(previous_hash_);
    }

    /**
     * @brief Get sequence number (total entries logged)
     */
    uint64_t get_sequence_number() const {
        return sequence_number_;
    }

    /**
     * @brief Verify a sequence of log entries, in order
     *
     * Recomputes every entry's hash and checks every link. Returns false if
     * any entry was edited, deleted, inserted or reordered, or cannot be
     * parsed. An empty sequence verifies. Entries removed from the end are
     * not caught here; use verify_chain_to_anchor() for that.
     */
    static bool verify_chain(const std::vector<std::string>& entries) {
        std::string last;
        return verify_entries(entries, last);
    }

    /**
     * @brief Verify entries and check the newest one against a saved chain hash
     * @param anchor_hex A chain hash saved outside the device (get_chain_hash())
     *
     * This also catches entries removed from the end of the log.
     */
    static bool verify_chain_to_anchor(const std::vector<std::string>& entries,
                                       const std::string& anchor_hex) {
        std::string last;
        return verify_entries(entries, last) && last == anchor_hex;
    }

private:
    std::string device_id_;
    SHA256::Hash previous_hash_;
    uint64_t sequence_number_ = 0;

    static std::string current_timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t time_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream out;
        out << std::put_time(std::gmtime(&time_now), "%Y-%m-%dT%H:%M:%S");
        out << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
        return out.str();
    }

    // The exact bytes that are hashed: the entry without its chain_hash member.
    static std::string canonical_body(const std::string& device_id,
                                      const std::string& timestamp,
                                      int user_id, int severity,
                                      const std::string& message,
                                      const std::string& previous_hex) {
        std::ostringstream body;
        body << "{"
             << "\"device_id\":\"" << escape_json(device_id) << "\","
             << "\"timestamp\":\"" << escape_json(timestamp) << "\","
             << "\"user_id\":" << user_id << ","
             << "\"severity\":" << severity << ","
             << "\"message\":\"" << escape_json(message) << "\","
             << "\"previous_hash\":\"" << previous_hex << "\""
             << "}";
        return body.str();
    }

    static bool verify_entries(const std::vector<std::string>& entries,
                               std::string& last_hash_hex) {
        std::string expected_previous = hash_to_hex(SHA256::hash("DTS_INIT"));
        last_hash_hex = expected_previous;

        for (const auto& entry : entries) {
            ParsedEntry parsed;
            if (!parse_entry(entry, parsed)) {
                return false;
            }
            if (parsed.previous_hash != expected_previous) {
                return false;
            }
            const std::string body = canonical_body(parsed.device_id, parsed.timestamp,
                                                    parsed.user_id, parsed.severity,
                                                    parsed.message, parsed.previous_hash);
            if (hash_to_hex(SHA256::hash(body)) != parsed.chain_hash) {
                return false;
            }
            expected_previous = parsed.chain_hash;
        }
        last_hash_hex = expected_previous;
        return true;
    }

    struct ParsedEntry {
        std::string device_id;
        std::string timestamp;
        int user_id = 0;
        int severity = 0;
        std::string message;
        std::string previous_hash;
        std::string chain_hash;
    };

    // Strict reader for the exact format log() writes: same keys, same order,
    // no extra whitespace. Anything else is treated as not verifiable.
    static bool parse_entry(const std::string& json, ParsedEntry& out) {
        size_t pos = 0;
        return expect(json, pos, "{\"device_id\":") && read_string(json, pos, out.device_id) &&
               expect(json, pos, ",\"timestamp\":") && read_string(json, pos, out.timestamp) &&
               expect(json, pos, ",\"user_id\":") && read_int(json, pos, out.user_id) &&
               expect(json, pos, ",\"severity\":") && read_int(json, pos, out.severity) &&
               expect(json, pos, ",\"message\":") && read_string(json, pos, out.message) &&
               expect(json, pos, ",\"previous_hash\":") && read_string(json, pos, out.previous_hash) &&
               expect(json, pos, ",\"chain_hash\":") && read_string(json, pos, out.chain_hash) &&
               expect(json, pos, "}") && pos == json.size() &&
               is_hex_hash(out.previous_hash) && is_hex_hash(out.chain_hash);
    }

    static bool expect(const std::string& json, size_t& pos, const char* literal) {
        const size_t len = std::strlen(literal);
        if (json.compare(pos, len, literal) != 0) {
            return false;
        }
        pos += len;
        return true;
    }

    static bool read_int(const std::string& json, size_t& pos, int& value) {
        size_t start = pos;
        if (pos < json.size() && json[pos] == '-') {
            ++pos;
        }
        size_t digits = 0;
        long result = 0;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
            result = result * 10 + (json[pos] - '0');
            if (result > 1000000) {
                return false;
            }
            ++pos;
            ++digits;
        }
        if (digits == 0) {
            return false;
        }
        value = static_cast<int>(json[start] == '-' ? -result : result);
        return true;
    }

    static int hex_value(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    // Reads a JSON string written by escape_json(). Escapes that escape_json()
    // never writes are rejected, which keeps the canonical form unique.
    static bool read_string(const std::string& json, size_t& pos, std::string& out) {
        if (pos >= json.size() || json[pos] != '"') {
            return false;
        }
        ++pos;
        out.clear();
        while (pos < json.size()) {
            char c = json[pos++];
            if (c == '"') {
                return true;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                return false;
            }
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos >= json.size()) {
                return false;
            }
            char e = json[pos++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos + 4 > json.size()) {
                        return false;
                    }
                    int code = 0;
                    for (int i = 0; i < 4; ++i) {
                        int v = hex_value(json[pos + i]);
                        if (v < 0) {
                            return false;
                        }
                        code = code * 16 + v;
                    }
                    if (code >= 0x20) {
                        return false;
                    }
                    out += static_cast<char>(code);
                    pos += 4;
                    break;
                }
                default:
                    return false;
            }
        }
        return false;
    }

    static bool is_hex_hash(const std::string& s) {
        if (s.size() != 64) {
            return false;
        }
        for (char c : s) {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                return false;
            }
        }
        return true;
    }

    static std::string hash_to_hex(const SHA256::Hash& hash) {
        std::ostringstream ss;
        for (const auto& byte : hash) {
            ss << std::hex << std::setfill('0') << std::setw(2)
               << static_cast<int>(byte);
        }
        return ss.str();
    }

    static std::string escape_json(const std::string& str) {
        std::ostringstream escaped;
        for (char c : str) {
            switch (c) {
                case '"': escaped << "\\\""; break;
                case '\\': escaped << "\\\\"; break;
                case '\b': escaped << "\\b"; break;
                case '\f': escaped << "\\f"; break;
                case '\n': escaped << "\\n"; break;
                case '\r': escaped << "\\r"; break;
                case '\t': escaped << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        escaped << "\\u" << std::hex << std::setfill('0')
                                << std::setw(4) << static_cast<int>(c) << std::dec;
                    } else {
                        escaped << c;
                    }
                    break;
            }
        }
        return escaped.str();
    }
};

} // namespace dts

#endif // DTS_AUDIT_CHAIN_HPP
