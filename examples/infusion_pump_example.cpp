/**
 * @file infusion_pump_example.cpp
 * @brief Example: Safety-critical infusion pump audit logging
 * 
 * Demonstrates DTS usage on an infusion pump, where a service tool needs
 * to check that the device log was not altered.
 */

#include <dts/audit_chain.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Initialize with pump serial number
    dts::AuditChain logger("INFUSION-PUMP-789012");
    
    std::cout << "=== Device Trust Shim - Infusion Pump Example ===\n\n";
    
    // Power-on self-test
    logger.log("POST: All systems nominal", dts::UserID::System, dts::Severity::Info);
    
    // Medication administration
    logger.log("Medication loaded - Drug: Insulin, Concentration: 100U/mL", 
               dts::UserID::Operator, dts::Severity::Info);
    
    logger.log("Infusion started - Rate: 2.5 mL/hr, Duration: 60 min", 
               dts::UserID::Operator, dts::Severity::Info);
    
    // Safety event: occlusion detected
    logger.log("Occlusion detected - Pressure threshold exceeded", 
               dts::UserID::System, dts::Severity::Warning);
    
    logger.log("Infusion paused - Safety protocol activated", 
               dts::UserID::System, dts::Severity::Critical);
    
    // Service event
    logger.log("Service technician access - Calibration performed", 
               dts::UserID::Service, dts::Severity::Info);
    
    std::cout << "Audit chain integrity hash: " << logger.get_chain_hash() << "\n";
    std::cout << "All events logged with tamper-evident chain.\n";
    
    return 0;
}

