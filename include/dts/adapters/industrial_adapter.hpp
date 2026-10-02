/**
 * @file industrial_adapter.hpp
 * @brief Industrial/OT audit logging adapter for PLCs, SCADA, and manufacturing systems
 * 
 * Provides structured logging for industrial control systems (ICS), including
 * PLC events, SCADA alarms, production batch tracking, and protocol events
 * (Modbus, OPC UA, EtherNet/IP, etc.).
 * 
 * The adapter formats messages. It does not talk to any controller or protocol itself.
 * 
 * @copyright Copyright (c) 2025 Big Data Plumbing
 * @license MIT License
 */

#ifndef DTS_INDUSTRIAL_ADAPTER_HPP
#define DTS_INDUSTRIAL_ADAPTER_HPP

#include "../audit_chain.hpp"
#include <string>
#include <sstream>

namespace dts {
namespace adapters {

/**
 * @brief Industrial protocol types
 */
enum class ProtocolType : uint8_t {
    Modbus = 1,
    OPCUA = 2,
    EtherNetIP = 3,
    Profinet = 4,
    DNP3 = 5,
    IEC61850 = 6,
    BACnet = 7,
    KNX = 8,
    MQTT = 9,
    Other = 255
};

/**
 * @brief Production event types
 */
enum class ProductionEventType : uint8_t {
    BatchStarted = 1,
    BatchCompleted = 2,
    BatchAborted = 3,
    RecipeLoaded = 4,
    RecipeChanged = 5,
    QualityCheckPassed = 6,
    QualityCheckFailed = 7
};

/**
 * @brief Industrial/OT audit logger
 * 
 * Extends AuditChain with industrial control system-specific logging
 * for PLCs, SCADA, MES, and manufacturing equipment.
 */
class IndustrialAdapter {
public:
    explicit IndustrialAdapter(const std::string& device_id,
                             const std::string& asset_tag = "",
                             const std::string& line_id = "")
        : chain_(device_id), asset_tag_(asset_tag), line_id_(line_id) {}
    
    /**
     * @brief Log PLC program execution event
     * @param program_name PLC program/routine name
     * @param rung_number Ladder logic rung number (if applicable)
     * @param event_description Event description
     */
    std::string log_plc_event(const std::string& program_name,
                             int rung_number = -1,
                             const std::string& event_description = "") {
        std::ostringstream msg;
        msg << "PLC Event | Program:" << program_name;
        
        if (rung_number >= 0) {
            msg << " | Rung:" << rung_number;
        }
        if (!event_description.empty()) {
            msg << " | " << event_description;
        }
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
        }
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log I/O state change (critical for forensics)
     * @param io_address I/O address/tag name
     * @param old_value Previous value
     * @param new_value New value
     * @param io_type Input or Output
     */
    std::string log_io_change(const std::string& io_address,
                              const std::string& old_value,
                              const std::string& new_value,
                              bool is_output = false) {
        std::ostringstream msg;
        msg << "I/O Change | "
            << (is_output ? "Output" : "Input") << ":" << io_address << " | "
            << "From:" << old_value << " | "
            << "To:" << new_value;
        
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
        }
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log SCADA alarm event
     * @param alarm_id Alarm identifier
     * @param alarm_message Alarm message/description
     * @param severity Alarm severity
     * @param acknowledged Whether alarm was acknowledged
     */
    std::string log_scada_alarm(const std::string& alarm_id,
                               const std::string& alarm_message,
                               Severity severity = Severity::Warning,
                               bool acknowledged = false) {
        std::ostringstream msg;
        msg << "SCADA Alarm | "
            << "ID:" << alarm_id << " | "
            << "Message:" << alarm_message << " | "
            << "Acknowledged:" << (acknowledged ? "Yes" : "No");
        
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
        }
        
        return chain_.log(msg.str(), UserID::Operator, severity);
    }
    
    /**
     * @brief Log production batch event
     * @param event_type Type of batch event
     * @param batch_id Batch identifier
     * @param product_code Product code/SKU
     * @param quantity Quantity produced (if applicable)
     */
    std::string log_production_event(ProductionEventType event_type,
                                    const std::string& batch_id,
                                    const std::string& product_code = "",
                                    int quantity = 0) {
        std::ostringstream msg;
        
        switch (event_type) {
            case ProductionEventType::BatchStarted:
                msg << "Batch Started";
                break;
            case ProductionEventType::BatchCompleted:
                msg << "Batch Completed";
                break;
            case ProductionEventType::BatchAborted:
                msg << "Batch Aborted";
                break;
            case ProductionEventType::RecipeLoaded:
                msg << "Recipe Loaded";
                break;
            case ProductionEventType::RecipeChanged:
                msg << "Recipe Changed";
                break;
            case ProductionEventType::QualityCheckPassed:
                msg << "Quality Check Passed";
                break;
            case ProductionEventType::QualityCheckFailed:
                msg << "Quality Check Failed";
                break;
        }
        
        msg << " | BatchID:" << batch_id;
        
        if (!product_code.empty()) {
            msg << " | Product:" << product_code;
        }
        if (quantity > 0) {
            msg << " | Quantity:" << quantity;
        }
        if (!line_id_.empty()) {
            msg << " | Line:" << line_id_;
        }
        
        Severity sev = (event_type == ProductionEventType::QualityCheckFailed ||
                       event_type == ProductionEventType::BatchAborted)
                      ? Severity::Error : Severity::Info;
        
        return chain_.log(msg.str(), UserID::Operator, sev);
    }
    
    /**
     * @brief Log protocol communication event (Modbus, OPC UA, etc.)
     * @param protocol Protocol type
     * @param source_address Source device address
     * @param destination_address Destination device address
     * @param function_code Function code/operation (e.g., Modbus function code)
     * @param success Whether operation succeeded
     */
    std::string log_protocol_event(ProtocolType protocol,
                                  const std::string& source_address,
                                  const std::string& destination_address,
                                  const std::string& function_code = "",
                                  bool success = true) {
        std::ostringstream msg;
        msg << "Protocol Event | ";
        
        switch (protocol) {
            case ProtocolType::Modbus:
                msg << "Modbus";
                break;
            case ProtocolType::OPCUA:
                msg << "OPC UA";
                break;
            case ProtocolType::EtherNetIP:
                msg << "EtherNet/IP";
                break;
            case ProtocolType::Profinet:
                msg << "Profinet";
                break;
            case ProtocolType::DNP3:
                msg << "DNP3";
                break;
            case ProtocolType::IEC61850:
                msg << "IEC 61850";
                break;
            case ProtocolType::BACnet:
                msg << "BACnet";
                break;
            case ProtocolType::KNX:
                msg << "KNX";
                break;
            case ProtocolType::MQTT:
                msg << "MQTT";
                break;
            default:
                msg << "Other";
                break;
        }
        
        msg << " | From:" << source_address << " | To:" << destination_address;
        
        if (!function_code.empty()) {
            msg << " | Function:" << function_code;
        }
        msg << " | Status:" << (success ? "Success" : "Failed");
        
        return chain_.log(msg.str(), UserID::System,
                         success ? Severity::Info : Severity::Warning);
    }
    
    /**
     * @brief Log safety interlock event (critical for safety systems)
     * @param interlock_id Interlock identifier
     * @param triggered Whether interlock was triggered
     * @param reason Reason for trigger/reset
     */
    std::string log_safety_interlock(const std::string& interlock_id,
                                    bool triggered,
                                    const std::string& reason = "") {
        std::ostringstream msg;
        msg << "Safety Interlock " << (triggered ? "TRIGGERED" : "RESET") << " | "
            << "ID:" << interlock_id;
        
        if (!reason.empty()) {
            msg << " | Reason:" << reason;
        }
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
        }
        
        return chain_.log(msg.str(), UserID::System,
                         triggered ? Severity::Critical : Severity::Info);
    }
    
    /**
     * @brief Log equipment status change
     * @param equipment_id Equipment identifier
     * @param old_status Previous status
     * @param new_status New status
     */
    std::string log_equipment_status(const std::string& equipment_id,
                                     const std::string& old_status,
                                     const std::string& new_status) {
        std::ostringstream msg;
        msg << "Equipment Status Change | "
            << "Equipment:" << equipment_id << " | "
            << "From:" << old_status << " | "
            << "To:" << new_status;
        
        if (!line_id_.empty()) {
            msg << " | Line:" << line_id_;
        }
        
        return chain_.log(msg.str(), UserID::System, Severity::Info);
    }
    
    /**
     * @brief Log parameter/recipe change (critical for audit)
     * @param parameter_name Parameter name/tag
     * @param old_value Previous value
     * @param new_value New value
     * @param user_id User who made the change
     */
    std::string log_parameter_change(const std::string& parameter_name,
                                    const std::string& old_value,
                                    const std::string& new_value,
                                    UserID user_id = UserID::Operator) {
        std::ostringstream msg;
        msg << "Parameter Changed | "
            << "Parameter:" << parameter_name << " | "
            << "From:" << old_value << " | "
            << "To:" << new_value;
        
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
        }
        
        return chain_.log(msg.str(), user_id, Severity::Warning);
    }
    
    /**
     * @brief Log maintenance event
     * @param maintenance_type Type of maintenance
     * @param technician_id Technician identifier
     * @param description Maintenance description
     */
    std::string log_maintenance(const std::string& maintenance_type,
                               const std::string& technician_id,
                               const std::string& description = "") {
        std::ostringstream msg;
        msg << "Maintenance | "
            << "Type:" << maintenance_type << " | "
            << "Technician:" << technician_id;
        
        if (!description.empty()) {
            msg << " | Description:" << description;
        }
        if (!asset_tag_.empty()) {
            msg << " | Asset:" << asset_tag_;
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
    std::string asset_tag_;
    std::string line_id_;
};

} // namespace adapters
} // namespace dts

#endif // DTS_INDUSTRIAL_ADAPTER_HPP

