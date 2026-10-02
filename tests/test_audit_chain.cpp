/**
 * @file test_audit_chain.cpp
 * @brief Unit tests for the DTS hash, the audit chain and the adapters
 *
 * Plain C++17 with no test framework. Each CHECK counts a failure instead of
 * aborting, and the checks stay active in every build type (they do not use
 * assert, which NDEBUG would remove).
 */

#include <dts/audit_chain.hpp>
#include <dts/adapters/building_automation_adapter.hpp>
#include <dts/adapters/clinical_trial_adapter.hpp>
#include <dts/adapters/dicom_adapter.hpp>
#include <dts/adapters/industrial_adapter.hpp>
#include <dts/adapters/medtech_adapter.hpp>

#include <iostream>
#include <string>
#include <vector>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  "      \
                      << #cond << "\n";                                      \
        }                                                                    \
    } while (0)

static std::string to_hex(const dts::SHA256::Hash& hash) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    for (auto byte : hash) {
        out += digits[byte >> 4];
        out += digits[byte & 0x0F];
    }
    return out;
}

// Replace the first occurrence of `from` in `s` with `to`.
static std::string replace_once(std::string s, const std::string& from, const std::string& to) {
    size_t pos = s.find(from);
    if (pos != std::string::npos) {
        s.replace(pos, from.size(), to);
    }
    return s;
}

// Known-answer tests: FIPS 180-4 examples plus padding-boundary lengths.
// Expected values come from a standard SHA-256 implementation.
static void test_sha256_known_answers() {
    CHECK(to_hex(dts::SHA256::hash(std::string(""))) ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(to_hex(dts::SHA256::hash(std::string("abc"))) ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(to_hex(dts::SHA256::hash(std::string(
              "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"))) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(to_hex(dts::SHA256::hash(std::string(
              "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
              "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"))) ==
          "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
    CHECK(to_hex(dts::SHA256::hash(std::string(1000000, 'a'))) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    struct Case { size_t length; const char* digest; };
    const Case cases[] = {
        {55, "d5e285683cd4efc02d021a5c62014694958901005d6f71e89e0989fac77e4072"},
        {56, "04c26261370ee7541549d16dee320c723e3fd14671e66a099afe0a377c16888e"},
        {57, "ae14a2563ccf969d99aca69ce6bb74981f734bbf9f655f73b8f06db68cab5217"},
        {63, "75220b47218278e656f2013bb8f0c455a25eaf01e86c64924e9d48d89776d6f2"},
        {64, "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c"},
        {65, "9537c5fdf120482f7d58d25e9ed583f52c02b4e304ea814db1633ad565aed7e9"},
        {119, "000b48d4edf0fa7bee3c6236ecd2785baa5db4eeb8bb54341b029e0d9fa5fb0c"},
        {120, "13f05a0b594787f5ecd315edc96141bd3243203d1b7d4f0836f37308b276ba98"},
        {127, "70156a14adbabf98cff3a71c7084b417abf057a8efd27329ca36b7202c87d81f"},
        {128, "24da1b81d0b16df6428eee73c69fcb2a93c76bc6df706f0c6670fe6bfe800464"},
    };
    for (const auto& c : cases) {
        CHECK(to_hex(dts::SHA256::hash(std::string(c.length, 'x'))) == c.digest);
    }
    std::cout << "ok  SHA-256 known answers\n";
}

static std::vector<std::string> sample_chain() {
    dts::AuditChain logger("TEST-DEVICE-001");
    std::vector<std::string> entries;
    entries.push_back(logger.log("Event 1"));
    entries.push_back(logger.log("Event 2", dts::UserID::Operator, dts::Severity::Warning));
    entries.push_back(logger.log("Event 3", dts::UserID::Admin, dts::Severity::Critical));
    return entries;
}

static void test_basic_logging() {
    dts::AuditChain logger("TEST-DEVICE-001");
    CHECK(logger.get_chain_hash() ==
          "87096d89791020ab24d9493f0956392d64c6dc57bb495f2a6b1b4cdc378b6347");  // SHA-256("DTS_INIT")

    std::string entry1 = logger.log("Test event 1");
    std::string entry2 = logger.log("Test event 2");

    CHECK(logger.get_sequence_number() == 2);
    CHECK(entry1.find("\"previous_hash\":\"87096d89") != std::string::npos);
    CHECK(entry2.find("\"chain_hash\":\"" + logger.get_chain_hash() + "\"") != std::string::npos);
    std::cout << "ok  basic logging\n";
}

static void test_valid_chain_verifies() {
    CHECK(dts::AuditChain::verify_chain(sample_chain()));
    CHECK(dts::AuditChain::verify_chain({}));  // an empty log verifies
    std::cout << "ok  valid chain verifies\n";
}

static void test_edits_are_detected() {
    const std::vector<std::string> good = sample_chain();

    auto edited = [&](size_t index, const std::string& from, const std::string& to) {
        std::vector<std::string> copy = good;
        copy[index] = replace_once(copy[index], from, to);
        CHECK(copy[index] != good[index]);  // the edit really happened
        return dts::AuditChain::verify_chain(copy);
    };

    CHECK(!edited(1, "Event 2", "HACKED!"));                     // message
    CHECK(!edited(1, "TEST-DEVICE-001", "TEST-DEVICE-999"));     // device id
    CHECK(!edited(1, "\"user_id\":2", "\"user_id\":1"));         // who
    CHECK(!edited(1, "\"severity\":2", "\"severity\":1"));       // severity
    CHECK(!edited(1, "\"timestamp\":\"20", "\"timestamp\":\"19")); // when
    CHECK(!edited(2, "\"chain_hash\":\"", "\"chain_hash\":\"0")); // hash field
    std::cout << "ok  edits are detected\n";
}

static void test_deletion_insertion_and_reordering_are_detected() {
    const std::vector<std::string> good = sample_chain();

    std::vector<std::string> deleted = {good[0], good[2]};
    CHECK(!dts::AuditChain::verify_chain(deleted));

    std::vector<std::string> duplicated = {good[0], good[1], good[1], good[2]};
    CHECK(!dts::AuditChain::verify_chain(duplicated));

    std::vector<std::string> reordered = {good[1], good[0], good[2]};
    CHECK(!dts::AuditChain::verify_chain(reordered));
    std::cout << "ok  deletion, insertion and reordering are detected\n";
}

static void test_truncation_needs_an_anchor() {
    dts::AuditChain logger("TEST-DEVICE-ANCHOR");
    std::vector<std::string> entries;
    entries.push_back(logger.log("one"));
    entries.push_back(logger.log("two"));
    entries.push_back(logger.log("three"));
    const std::string anchor = logger.get_chain_hash();  // saved off the device

    std::vector<std::string> truncated(entries.begin(), entries.end() - 1);

    // Without an anchor, dropping the newest entry still verifies. This is the
    // documented limit of a keyless hash chain.
    CHECK(dts::AuditChain::verify_chain(truncated));
    // With the saved anchor, the same truncation is caught.
    CHECK(dts::AuditChain::verify_chain_to_anchor(entries, anchor));
    CHECK(!dts::AuditChain::verify_chain_to_anchor(truncated, anchor));
    std::cout << "ok  truncation is caught with an anchor\n";
}

static void test_special_characters_round_trip() {
    dts::AuditChain logger("DEV \"quoted\" \\ id");
    std::vector<std::string> entries;
    entries.push_back(logger.log("quote \" backslash \\ newline \n tab \t bell \a"));
    entries.push_back(logger.log("UTF-8: 22.5 \xC2\xB0" "C, pipes | in | text"));
    CHECK(dts::AuditChain::verify_chain(entries));
    CHECK(entries[0].find('\n') == std::string::npos);  // one line per entry
    CHECK(entries[0].find("\\u0007") != std::string::npos);
    std::cout << "ok  special characters round trip\n";
}

// The hash covers the escaped JSON, so text cannot be moved from one field to
// another while the hash still matches.
static void test_field_boundaries_are_fixed() {
    dts::AuditChain logger("A|B");
    std::vector<std::string> entries = {logger.log("message")};
    CHECK(dts::AuditChain::verify_chain(entries));

    std::string forged = replace_once(entries[0], "\"device_id\":\"A|B\"", "\"device_id\":\"A\"");
    forged = replace_once(forged, "\"timestamp\":\"", "\"timestamp\":\"B|");
    CHECK(!dts::AuditChain::verify_chain({forged}));
    std::cout << "ok  field boundaries are fixed\n";
}

static void test_malformed_input_is_rejected() {
    CHECK(!dts::AuditChain::verify_chain({"not json"}));
    CHECK(!dts::AuditChain::verify_chain({""}));
    std::vector<std::string> good = sample_chain();
    CHECK(!dts::AuditChain::verify_chain({good[0] + " "}));  // trailing bytes
    std::cout << "ok  malformed input is rejected\n";
}

static void test_adapters_produce_verifiable_chains() {
    {
        dts::adapters::IndustrialAdapter logger("PLC-001", "ASSET-001", "LINE-A");
        std::vector<std::string> e;
        e.push_back(logger.log_io_change("FILL_VALVE_OUT", "0", "1", true));
        e.push_back(logger.log_protocol_event(dts::adapters::ProtocolType::Modbus,
                                              "192.168.1.10:502", "192.168.1.20:502", "Function 03"));
        CHECK(dts::AuditChain::verify_chain(e));
    }
    {
        dts::adapters::BuildingAutomationAdapter logger("BAS-001", "BUILDING-A");
        std::vector<std::string> e;
        e.push_back(logger.log_access_control("DOOR-MAIN", "BADGE-12345", true));
        e.push_back(logger.log_knx_event("1/2/3", "ON", "DPT1.001"));
        CHECK(dts::AuditChain::verify_chain(e));
    }
    {
        dts::adapters::DICOMAdapter logger("PACS-001", "PACS-AE-01");
        std::vector<std::string> e;
        e.push_back(logger.log_study_created("1.2.840.113619.2.55.3", "ANON-1", "MR"));
        CHECK(dts::AuditChain::verify_chain(e));
    }
    {
        dts::adapters::MedTechAdapter logger("PUMP-001");
        std::vector<std::string> e;
        e.push_back(logger.log_safety_alarm("Occlusion", dts::adapters::AlarmPriority::High,
                                            "Pressure threshold exceeded"));
        CHECK(dts::AuditChain::verify_chain(e));
    }
    {
        dts::adapters::ClinicalTrialAdapter logger("TRIAL-001", "PROTOCOL-1");
        std::vector<std::string> e;
        e.push_back(logger.log_patient_enrolled("PATIENT-12345", "SITE-001"));
        CHECK(dts::AuditChain::verify_chain(e));
        CHECK(e[0].find("PATIENT-12345") == std::string::npos);  // raw ID not written
    }
    std::cout << "ok  adapters produce verifiable chains\n";
}

int main() {
    std::cout << "Running DTS unit tests\n\n";

    test_sha256_known_answers();
    test_basic_logging();
    test_valid_chain_verifies();
    test_edits_are_detected();
    test_deletion_insertion_and_reordering_are_detected();
    test_truncation_needs_an_anchor();
    test_special_characters_round_trip();
    test_field_boundaries_are_fixed();
    test_malformed_input_is_rejected();
    test_adapters_produce_verifiable_chains();

    std::cout << "\n" << (g_checks - g_failures) << " of " << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
