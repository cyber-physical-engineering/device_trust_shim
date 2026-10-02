/**
 * @file radiology_device_example.cpp
 * @brief Example: Tamper-evident audit logging for radiology device
 * 
 * Demonstrates DTS usage on a radiology device, where a reviewer needs
 * to check that the device log was not altered.
 */

#include <dts/audit_chain.hpp>
#include <iostream>
#include <vector>

int main() {
    // Initialize audit chain with device serial number
    dts::AuditChain logger("PACS-2025-001234");
    
    std::cout << "=== Device Trust Shim - Radiology Device Example ===\n\n";
    
    // Simulate device startup sequence
    std::cout << logger.log("Device power-on initiated", 
                            dts::UserID::System, 
                            dts::Severity::Info) << "\n\n";
    
    std::cout << logger.log("Firmware version 2.1.3 loaded", 
                            dts::UserID::System, 
                            dts::Severity::Info) << "\n\n";
    
    // Admin authentication
    std::cout << logger.log("Admin authentication successful", 
                            dts::UserID::Admin, 
                            dts::Severity::Info) << "\n\n";
    
    // Patient scan initiated
    std::cout << logger.log("Patient scan initiated - MRN: 12345678", 
                            dts::UserID::Operator, 
                            dts::Severity::Info) << "\n\n";
    
    // Critical event: AI inference request
    std::cout << logger.log("AI inference request - Model: radiology-v2, Input: DICOM-001.dcm", 
                            dts::UserID::System, 
                            dts::Severity::Warning) << "\n\n";
    
    // Error condition
    std::cout << logger.log("Network connectivity lost during DICOM transfer", 
                            dts::UserID::System, 
                            dts::Severity::Error) << "\n\n";
    
    // Display chain state
    std::cout << "Chain Hash: " << logger.get_chain_hash() << "\n";
    std::cout << "Total Entries: " << logger.get_sequence_number() << "\n";
    
    return 0;
}

