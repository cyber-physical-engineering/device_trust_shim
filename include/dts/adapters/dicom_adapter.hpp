/**
 * @file dicom_adapter.hpp
 * @brief DICOM-specific audit logging adapter for PACS and radiology devices
 * 
 * Provides structured logging for DICOM events (Study creation, AI inference,
 * image transfer) with automatic extraction of relevant DICOM tags for audit trails.
 * 
 * @copyright Copyright (c) 2025 Big Data Plumbing
 * @license MIT License
 */

#ifndef DTS_DICOM_ADAPTER_HPP
#define DTS_DICOM_ADAPTER_HPP

#include "../audit_chain.hpp"
#include <string>
#include <sstream>

namespace dts {
namespace adapters {

/**
 * @brief DICOM event types for audit logging
 */
enum class DICOMEventType : uint8_t {
    StudyCreated = 1,
    StudyModified = 2,
    InstanceStored = 3,
    InstanceDeleted = 4,
    AIInferenceRequested = 5,
    AIInferenceCompleted = 6,
    TransferInitiated = 7,
    TransferCompleted = 8,
    AccessGranted = 9,
    AccessDenied = 10
};

/**
 * @brief DICOM-specific audit logger
 * 
 * Extends AuditChain with DICOM-aware event logging, automatically
 * extracting and logging relevant DICOM tags for audit.
 */
class DICOMAdapter {
public:
    explicit DICOMAdapter(const std::string& device_id, 
                         const std::string& ae_title = "")
        : chain_(device_id), ae_title_(ae_title) {}
    
    /**
     * @brief Log DICOM study creation event
     * @param study_instance_uid DICOM StudyInstanceUID (0020,000D)
     * @param patient_id DICOM PatientID (0010,0020) - may be anonymized
     * @param modality Modality (0008,0060)
     * @param user_id User who created the study
     */
    std::string log_study_created(const std::string& study_instance_uid,
                                   const std::string& patient_id,
                                   const std::string& modality,
                                   UserID user_id = UserID::Operator) {
        std::ostringstream msg;
        msg << "DICOM Study Created | "
            << "StudyInstanceUID:" << study_instance_uid << " | "
            << "PatientID:" << patient_id << " | "
            << "Modality:" << modality;
        
        if (!ae_title_.empty()) {
            msg << " | AETitle:" << ae_title_;
        }
        
        return chain_.log(msg.str(), user_id, Severity::Info);
    }
    
    /**
     * @brief Log an AI inference request
     * @param study_instance_uid Study being analyzed
     * @param model_name AI model identifier
     * @param model_version Model version
     * @param input_instances Number of DICOM instances in input
     */
    std::string log_ai_inference_request(const std::string& study_instance_uid,
                                         const std::string& model_name,
                                         const std::string& model_version,
                                         int input_instances = 1) {
        std::ostringstream msg;
        msg << "AI Inference Request | "
            << "StudyInstanceUID:" << study_instance_uid << " | "
            << "Model:" << model_name << " | "
            << "Version:" << model_version << " | "
            << "InputInstances:" << input_instances;
        
        return chain_.log(msg.str(), UserID::System, Severity::Warning);
    }
    
    /**
     * @brief Log AI inference completion
     * @param study_instance_uid Study analyzed
     * @param model_name Model used
     * @param inference_id Unique inference identifier
     * @param result_summary Summary of AI output (e.g., "Priority: HIGH")
     */
    std::string log_ai_inference_completed(const std::string& study_instance_uid,
                                           const std::string& model_name,
                                           const std::string& inference_id,
                                           const std::string& result_summary) {
        std::ostringstream msg;
        msg << "AI Inference Completed | "
            << "StudyInstanceUID:" << study_instance_uid << " | "
            << "Model:" << model_name << " | "
            << "InferenceID:" << inference_id << " | "
            << "Result:" << result_summary;
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log DICOM instance storage (STOW-RS)
     * @param study_instance_uid Study UID
     * @param sop_instance_uid SOP Instance UID (0008,0018)
     * @param sop_class_uid SOP Class UID (0008,0016)
     */
    std::string log_instance_stored(const std::string& study_instance_uid,
                                    const std::string& sop_instance_uid,
                                    const std::string& sop_class_uid) {
        std::ostringstream msg;
        msg << "DICOM Instance Stored | "
            << "StudyInstanceUID:" << study_instance_uid << " | "
            << "SOPInstanceUID:" << sop_instance_uid << " | "
            << "SOPClassUID:" << sop_class_uid;
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log DICOM transfer initiation (DICOMweb STOW-RS)
     * @param study_instance_uid Study being transferred
     * @param destination Destination AE Title or endpoint
     * @param transfer_syntax Transfer syntax UID
     */
    std::string log_transfer_initiated(const std::string& study_instance_uid,
                                      const std::string& destination,
                                      const std::string& transfer_syntax = "") {
        std::ostringstream msg;
        msg << "DICOM Transfer Initiated | "
            << "StudyInstanceUID:" << study_instance_uid << " | "
            << "Destination:" << destination;
        
        if (!transfer_syntax.empty()) {
            msg << " | TransferSyntax:" << transfer_syntax;
        }
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log an access control event
     * @param study_instance_uid Study accessed
     * @param user_id User attempting access
     * @param granted Whether access was granted
     * @param reason Reason for denial (if denied)
     */
    std::string log_access_event(const std::string& study_instance_uid,
                                 UserID user_id,
                                 bool granted,
                                 const std::string& reason = "") {
        std::ostringstream msg;
        msg << "DICOM Access " << (granted ? "Granted" : "Denied") << " | "
            << "StudyInstanceUID:" << study_instance_uid;
        
        if (!granted && !reason.empty()) {
            msg << " | Reason:" << reason;
        }
        
        return chain_.log(msg.str(), user_id, 
                         granted ? Severity::Info : Severity::Warning);
    }
    
    /**
     * @brief Get underlying audit chain (for direct access if needed)
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
    std::string ae_title_;
};

} // namespace adapters
} // namespace dts

#endif // DTS_DICOM_ADAPTER_HPP

