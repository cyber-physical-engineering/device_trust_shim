/**
 * @file clinical_trial_adapter.hpp
 * @brief Clinical trial data collection audit logging adapter
 * 
 * Provides structured logging for clinical trial devices. The raw patient ID
 * never enters the log; a short SHA-256 pseudonym stands in for it.
 * 
 * @copyright Copyright (c) 2025 Big Data Plumbing
 * @license MIT License
 */

#ifndef DTS_CLINICAL_TRIAL_ADAPTER_HPP
#define DTS_CLINICAL_TRIAL_ADAPTER_HPP

#include "../audit_chain.hpp"
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>

namespace dts {
namespace adapters {

/**
 * @brief Clinical trial event types
 */
enum class TrialEventType : uint8_t {
    PatientEnrolled = 1,
    VisitStarted = 2,
    VisitCompleted = 3,
    DataCollected = 4,
    ProtocolDeviation = 5,
    AdverseEvent = 6,
    DataExported = 7,
    DataAnonymized = 8
};

/**
 * @brief Clinical trial audit logger
 * 
 * Extends AuditChain with clinical trial-specific logging, including
 * a patient ID pseudonym in place of the raw ID, and protocol deviation logging.
 */
class ClinicalTrialAdapter {
public:
    /**
     * @brief Pseudonym function: maps a raw patient ID to the string that is logged
     * Default: SHA-256 hash of patient ID (one-way, deterministic)
     */
    using AnonymizerFunc = std::function<std::string(const std::string&)>;
    
    explicit ClinicalTrialAdapter(const std::string& device_id,
                                 const std::string& protocol_id = "",
                                 AnonymizerFunc anonymizer = nullptr)
        : chain_(device_id), protocol_id_(protocol_id) {
        if (anonymizer) {
            anonymizer_ = anonymizer;
        } else {
            // Default: use SHA-256 hash (first 16 chars for readability)
            anonymizer_ = [](const std::string& id) {
                auto hash = dts::SHA256::hash(id);
                std::ostringstream ss;
                for (size_t i = 0; i < 8; ++i) {
                    ss << std::hex << std::setfill('0') << std::setw(2)
                       << static_cast<int>(hash[i]);
                }
                return "ANON-" + ss.str();
            };
        }
    }
    
    /**
     * @brief Log patient enrollment event
     * @param patient_id Patient identifier (replaced by its pseudonym in the log)
     * @param site_id Clinical site identifier
     * @param enrollment_date Enrollment date/time
     */
    std::string log_patient_enrolled(const std::string& patient_id,
                                    const std::string& site_id,
                                    const std::string& enrollment_date = "") {
        std::string anon_id = anonymizer_(patient_id);
        
        std::ostringstream msg;
        msg << "Patient Enrolled | "
            << "PatientID:" << anon_id << " | "
            << "SiteID:" << site_id;
        
        if (!protocol_id_.empty()) {
            msg << " | Protocol:" << protocol_id_;
        }
        if (!enrollment_date.empty()) {
            msg << " | Date:" << enrollment_date;
        }
        
        return chain_.log(msg.str(), UserID::Operator, Severity::Info);
    }
    
    /**
     * @brief Log visit event (start/completion)
     * @param event_type VisitStarted or VisitCompleted
     * @param patient_id Patient identifier (replaced by its pseudonym in the log)
     * @param visit_number Visit number
     * @param visit_type Visit type (e.g., "Screening", "Baseline", "Follow-up")
     */
    std::string log_visit_event(TrialEventType event_type,
                               const std::string& patient_id,
                               int visit_number,
                               const std::string& visit_type = "") {
        std::string anon_id = anonymizer_(patient_id);
        
        std::ostringstream msg;
        msg << (event_type == TrialEventType::VisitStarted 
                ? "Visit Started" : "Visit Completed") << " | "
            << "PatientID:" << anon_id << " | "
            << "VisitNumber:" << visit_number;
        
        if (!visit_type.empty()) {
            msg << " | VisitType:" << visit_type;
        }
        
        return chain_.log(msg.str(), UserID::Operator, Severity::Info);
    }
    
    /**
     * @brief Log data collection event
     * @param patient_id Patient identifier (replaced by its pseudonym in the log)
     * @param data_type Type of data collected (e.g., "Vital Signs", "Lab Results")
     * @param form_id Case Report Form (CRF) identifier
     * @param data_point_count Number of data points collected
     */
    std::string log_data_collected(const std::string& patient_id,
                                  const std::string& data_type,
                                  const std::string& form_id = "",
                                  int data_point_count = 0) {
        std::string anon_id = anonymizer_(patient_id);
        
        std::ostringstream msg;
        msg << "Data Collected | "
            << "PatientID:" << anon_id << " | "
            << "DataType:" << data_type;
        
        if (!form_id.empty()) {
            msg << " | CRF:" << form_id;
        }
        if (data_point_count > 0) {
            msg << " | Points:" << data_point_count;
        }
        
        return chain_.log(msg.str(), UserID::Operator, Severity::Info);
    }
    
    /**
     * @brief Log a protocol deviation
     * @param patient_id Patient identifier (replaced by its pseudonym in the log)
     * @param deviation_type Type of deviation
     * @param description Deviation description
     * @param severity Deviation severity
     */
    std::string log_protocol_deviation(const std::string& patient_id,
                                      const std::string& deviation_type,
                                      const std::string& description,
                                      Severity severity = Severity::Warning) {
        std::string anon_id = anonymizer_(patient_id);
        
        std::ostringstream msg;
        msg << "Protocol Deviation | "
            << "PatientID:" << anon_id << " | "
            << "Type:" << deviation_type << " | "
            << "Description:" << description;
        
        if (!protocol_id_.empty()) {
            msg << " | Protocol:" << protocol_id_;
        }
        
        return chain_.log(msg.str(), UserID::Operator, severity);
    }
    
    /**
     * @brief Log adverse event (SAE reporting requirement)
     * @param patient_id Patient identifier (replaced by its pseudonym in the log)
     * @param ae_type Adverse event type
     * @param severity AE severity (mild/moderate/severe)
     * @param relatedness Whether AE is related to investigational product
     */
    std::string log_adverse_event(const std::string& patient_id,
                                 const std::string& ae_type,
                                 const std::string& severity,
                                 bool relatedness = false) {
        std::string anon_id = anonymizer_(patient_id);
        
        std::ostringstream msg;
        msg << "Adverse Event | "
            << "PatientID:" << anon_id << " | "
            << "Type:" << ae_type << " | "
            << "Severity:" << severity << " | "
            << "Related:" << (relatedness ? "Yes" : "No");
        
        return chain_.log(msg.str(), UserID::Operator, Severity::Error);
    }
    
    /**
     * @brief Log data export event (for data transfer audit)
     * @param export_type Type of export (e.g., "CRF", "Lab Data", "Imaging")
     * @param destination Export destination
     * @param record_count Number of records exported
     * @param anonymized Whether identifiers were removed before export
     */
    std::string log_data_export(const std::string& export_type,
                               const std::string& destination,
                               int record_count,
                               bool anonymized = true) {
        std::ostringstream msg;
        msg << "Data Exported | "
            << "Type:" << export_type << " | "
            << "Destination:" << destination << " | "
            << "Records:" << record_count << " | "
            << "Anonymized:" << (anonymized ? "Yes" : "No");
        
        return chain_.log(msg.str(), UserID::Admin, Severity::Info);
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
    std::string protocol_id_;
    AnonymizerFunc anonymizer_;
};

} // namespace adapters
} // namespace dts

#endif // DTS_CLINICAL_TRIAL_ADAPTER_HPP

