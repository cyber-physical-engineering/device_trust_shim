/**
 * @file dicom_adapter_example.cpp
 * @brief Example: DICOM audit logging for PACS integration
 * 
 * Demonstrates DICOM-specific audit logging for radiology devices,
 * including AI inference request and completion events.
 */

#include <dts/adapters/dicom_adapter.hpp>
#include <iostream>

int main() {
    // Initialize DICOM adapter with device AE Title
    dts::adapters::DICOMAdapter logger("PACS-2025-001234", "PACS-AE-01");
    
    std::cout << "=== Device Trust Shim - DICOM Adapter Example ===\n\n";
    
    // Study creation
    std::cout << logger.log_study_created(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        "ANON-12345678",  // Anonymized PatientID
        "MR"
    ) << "\n\n";
    
    // Instance storage (STOW-RS)
    std::cout << logger.log_instance_stored(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        "1.2.840.113619.2.55.3.1234567890.1234567890123457",
        "1.2.840.10008.5.1.4.1.1.4"  // MR Image Storage
    ) << "\n\n";
    
    // AI inference request
    std::cout << logger.log_ai_inference_request(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        "radiology-detection-v2",
        "2.1.0",
        120  // 120 instances in study
    ) << "\n\n";
    
    // AI inference completion
    std::cout << logger.log_ai_inference_completed(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        "radiology-detection-v2",
        "INF-20250115-001",
        "Priority: HIGH | Findings: 2 lesions detected"
    ) << "\n\n";
    
    // Transfer initiation (DICOMweb STOW-RS)
    std::cout << logger.log_transfer_initiated(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        "https://healthcare-api.example.com/dicomweb/studies",
        "1.2.840.10008.1.2.4.70"  // JPEG 2000 Lossless
    ) << "\n\n";
    
    // Access control event
    std::cout << logger.log_access_event(
        "1.2.840.113619.2.55.3.1234567890.1234567890123456",
        dts::UserID::Admin,
        true
    ) << "\n\n";
    
    std::cout << "Chain Hash: " << logger.get_chain_hash() << "\n";
    
    return 0;
}

