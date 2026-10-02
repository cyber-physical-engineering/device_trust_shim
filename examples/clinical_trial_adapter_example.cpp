/**
 * @file clinical_trial_adapter_example.cpp
 * @brief Example: Clinical trial data collection audit logging
 * 
 * Demonstrates audit logging for clinical trial devices. The raw patient ID
 * is replaced by a pseudonym before it is logged.
 */

#include <dts/adapters/clinical_trial_adapter.hpp>
#include <iostream>

int main() {
    // Initialize with protocol ID
    dts::adapters::ClinicalTrialAdapter logger(
        "TRIAL-DEVICE-001",
        "PROTOCOL-2025-001"
    );
    
    std::cout << "=== Device Trust Shim - Clinical Trial Adapter Example ===\n\n";
    
    // Patient enrollment
    std::cout << logger.log_patient_enrolled(
        "PATIENT-12345",  // logged as ANON- plus a 16-hex-character pseudonym
        "SITE-001",
        "2025-01-15T10:00:00Z"
    ) << "\n\n";
    
    // Visit started
    std::cout << logger.log_visit_event(
        dts::adapters::TrialEventType::VisitStarted,
        "PATIENT-12345",
        1,
        "Screening"
    ) << "\n\n";
    
    // Data collection
    std::cout << logger.log_data_collected(
        "PATIENT-12345",
        "Vital Signs",
        "CRF-001",
        5  // 5 data points
    ) << "\n\n";
    
    // Protocol deviation
    std::cout << logger.log_protocol_deviation(
        "PATIENT-12345",
        "Missed Visit",
        "Patient missed scheduled visit window by 2 days",
        dts::Severity::Warning
    ) << "\n\n";
    
    // Adverse event
    std::cout << logger.log_adverse_event(
        "PATIENT-12345",
        "Headache",
        "Mild",
        false  // Not related to investigational product
    ) << "\n\n";
    
    // Visit completed
    std::cout << logger.log_visit_event(
        dts::adapters::TrialEventType::VisitCompleted,
        "PATIENT-12345",
        1,
        "Screening"
    ) << "\n\n";
    
    // Data export (for EDC system)
    std::cout << logger.log_data_export(
        "CRF",
        "EDC-System-001",
        150,  // 150 records
        true  // identifiers removed before export
    ) << "\n\n";
    
    std::cout << "Chain Hash: " << logger.get_chain_hash() << "\n";
    std::cout << "Note: the log never carries the raw patient ID, only a pseudonym (ANON- plus 16 hex characters of its SHA-256).\n";
    
    return 0;
}

