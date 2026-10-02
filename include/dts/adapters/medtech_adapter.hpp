/**
 * @file medtech_adapter.hpp
 * @brief Audit logging adapter for infusion pumps, ventilators and similar devices
 * 
 * Provides structured logging for infusion pumps, ventilators, and other
 * safety-critical medical devices with alarm and safety event tracking.
 * 
 * @copyright Copyright (c) 2025 Big Data Plumbing
 * @license MIT License
 */

#ifndef DTS_MEDTECH_ADAPTER_HPP
#define DTS_MEDTECH_ADAPTER_HPP

#include "../audit_chain.hpp"
#include <string>
#include <sstream>

namespace dts {
namespace adapters {

/**
 * @brief Alarm priority levels (IEC 60601-1-8)
 */
enum class AlarmPriority : uint8_t {
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

/**
 * @brief Medication administration event types
 */
enum class MedicationEventType : uint8_t {
    Loaded = 1,
    Started = 2,
    Paused = 3,
    Stopped = 4,
    Completed = 5,
    Error = 6
};

/**
 * @brief MedTech device audit logger
 * 
 * Extends AuditChain with medical device-specific event logging
 * for infusion pumps, ventilators, and other safety-critical devices.
 */
class MedTechAdapter {
public:
    explicit MedTechAdapter(const std::string& device_id,
                           const std::string& device_type = "")
        : chain_(device_id), device_type_(device_type) {}
    
    /**
     * @brief Log power-on self-test (POST) result
     * @param passed Whether POST passed
     * @param details POST details or failure reason
     */
    std::string log_post_result(bool passed, const std::string& details = "") {
        std::ostringstream msg;
        msg << "POST " << (passed ? "PASSED" : "FAILED");
        
        if (!details.empty()) {
            msg << " | " << details;
        }
        
        return chain_.log(msg.str(), UserID::System,
                         passed ? Severity::Info : Severity::Critical);
    }
    
    /**
     * @brief Log medication administration event
     * @param event_type Type of medication event
     * @param drug_name Drug name or identifier
     * @param concentration Drug concentration (e.g., "100U/mL")
     * @param rate Infusion rate (e.g., "2.5 mL/hr")
     * @param duration Duration in minutes (if applicable)
     */
    std::string log_medication_event(MedicationEventType event_type,
                                    const std::string& drug_name,
                                    const std::string& concentration = "",
                                    const std::string& rate = "",
                                    int duration = 0) {
        std::ostringstream msg;
        
        switch (event_type) {
            case MedicationEventType::Loaded:
                msg << "Medication Loaded";
                break;
            case MedicationEventType::Started:
                msg << "Infusion Started";
                break;
            case MedicationEventType::Paused:
                msg << "Infusion Paused";
                break;
            case MedicationEventType::Stopped:
                msg << "Infusion Stopped";
                break;
            case MedicationEventType::Completed:
                msg << "Infusion Completed";
                break;
            case MedicationEventType::Error:
                msg << "Medication Error";
                break;
        }
        
        msg << " | Drug:" << drug_name;
        
        if (!concentration.empty()) {
            msg << " | Concentration:" << concentration;
        }
        if (!rate.empty()) {
            msg << " | Rate:" << rate;
        }
        if (duration > 0) {
            msg << " | Duration:" << duration << "min";
        }
        
        Severity sev = (event_type == MedicationEventType::Error) 
                      ? Severity::Error 
                      : Severity::Info;
        
        return chain_.log(msg.str(), UserID::Operator, sev);
    }
    
    /**
     * @brief Log a safety alarm with its priority (the IEC 60601-1-8 priority terms)
     * @param alarm_type Alarm type identifier
     * @param priority Alarm priority level
     * @param description Alarm description
     * @param action_taken Action taken in response
     */
    std::string log_safety_alarm(const std::string& alarm_type,
                                AlarmPriority priority,
                                const std::string& description,
                                const std::string& action_taken = "") {
        std::ostringstream msg;
        msg << "Safety Alarm | "
            << "Type:" << alarm_type << " | "
            << "Priority:" << static_cast<int>(priority) << " | "
            << "Description:" << description;
        
        if (!action_taken.empty()) {
            msg << " | Action:" << action_taken;
        }
        
        Severity sev = (priority == AlarmPriority::Critical) 
                      ? Severity::Critical
                      : (priority == AlarmPriority::High)
                      ? Severity::Error
                      : Severity::Warning;
        
        return chain_.log(msg.str(), UserID::System, sev);
    }
    
    /**
     * @brief Log device calibration event
     * @param calibration_type Type of calibration performed
     * @param technician_id Service technician identifier
     * @param result Calibration result (pass/fail)
     */
    std::string log_calibration(const std::string& calibration_type,
                               const std::string& technician_id,
                               bool result) {
        std::ostringstream msg;
        msg << "Calibration " << (result ? "PASSED" : "FAILED") << " | "
            << "Type:" << calibration_type << " | "
            << "Technician:" << technician_id;
        
        return chain_.log(msg.str(), UserID::Service,
                         result ? Severity::Info : Severity::Warning);
    }
    
    /**
     * @brief Log firmware update event
     * @param old_version Previous firmware version
     * @param new_version New firmware version
     * @param success Whether update succeeded
     */
    std::string log_firmware_update(const std::string& old_version,
                                    const std::string& new_version,
                                    bool success) {
        std::ostringstream msg;
        msg << "Firmware Update " << (success ? "SUCCESS" : "FAILED") << " | "
            << "From:" << old_version << " | "
            << "To:" << new_version;
        
        return chain_.log(msg.str(), UserID::Admin,
                         success ? Severity::Info : Severity::Error);
    }
    
    /**
     * @brief Log device maintenance event
     * @param maintenance_type Type of maintenance
     * @param technician_id Service technician
     * @param notes Maintenance notes
     */
    std::string log_maintenance(const std::string& maintenance_type,
                               const std::string& technician_id,
                               const std::string& notes = "") {
        std::ostringstream msg;
        msg << "Maintenance | "
            << "Type:" << maintenance_type << " | "
            << "Technician:" << technician_id;
        
        if (!notes.empty()) {
            msg << " | Notes:" << notes;
        }
        
        return chain_.log(msg.str(), UserID::Service, Severity::Info);
    }
    
    /**
     * @brief Get underlying audit chain
     */
    AuditChain& get_chain() { return chain_; }
    const AuditChain& get_chain() const { return chain_; }
    
    /**
     * @brief Get current chain hash
     */
    std::string get_chain_hash() const {
        return chain_.get_chain_hash();
    }

private:
    AuditChain chain_;
    std::string device_type_;
};

} // namespace adapters
} // namespace dts

#endif // DTS_MEDTECH_ADAPTER_HPP

