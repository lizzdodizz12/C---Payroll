#include "../include/Position.h"
#include <iostream>
#include <iomanip>

Position::Position() 
    : positionID(0), positionName(""), payType(""), defaultRate(0.0) {}

Position::~Position() {}

// Setter methods
void Position::setPositionID(int id) {
    positionID = id;
}

void Position::setPositionName(const std::string& name) {
    positionName = name;
}

void Position::setPayType(const std::string& type) {
    payType = type;
}

void Position::setDefaultRate(double rate) {
    defaultRate = rate;
}

// Getter methods
int Position::getPositionID() const {
    return positionID;
}

std::string Position::getPositionName() const {
    return positionName;
}

std::string Position::getPayType() const {
    return payType;
}

double Position::getDefaultRate() const {
    return defaultRate;
}

// Database operations
bool Position::addPosition(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    if (positionName.empty() || defaultRate <= 0.0 ||
        (payType != "Monthly" && payType != "Daily" && payType != "Hourly")) {
        std::cerr << "ERROR: Position name, pay type, and a positive rate are required." << std::endl;
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO positions (position_name, pay_type, default_rate)
        VALUES (?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    // Bind parameters
    sqlite3_bind_text(stmt, 1, positionName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, payType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, defaultRate);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing insert: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    // Get the inserted position ID
    positionID = sqlite3_last_insert_rowid(db.getDB());
    sqlite3_finalize(stmt);
    
    std::cout << "Position added successfully with ID: " << positionID << std::endl;
    return true;
}

bool Position::updatePosition(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    if (positionName.empty() || defaultRate <= 0.0 ||
        (payType != "Monthly" && payType != "Daily" && payType != "Hourly")) {
        std::cerr << "ERROR: Position name, pay type, and a positive rate are required." << std::endl;
        return false;
    }
    
    const char* sql = R"(
        UPDATE positions 
        SET position_name = ?, pay_type = ?, default_rate = ?
        WHERE position_id = ?
    )";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    // Bind parameters
    sqlite3_bind_text(stmt, 1, positionName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, payType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, defaultRate);
    sqlite3_bind_int(stmt, 4, positionID);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing update: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "Position updated successfully." << std::endl;
    return true;
}

bool Position::deletePosition(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    // Check if position is used by any employee
    const char* checkSql = "SELECT COUNT(*) FROM employees WHERE position_id = ?";
    sqlite3_stmt* checkStmt;
    
    int result = sqlite3_prepare_v2(db.getDB(), checkSql, -1, &checkStmt, nullptr);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(checkStmt, 1, positionID);
    
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(checkStmt, 0);
        sqlite3_finalize(checkStmt);
        
        if (count > 0) {
            std::cerr << "ERROR: Cannot delete position. It is currently assigned to " 
                      << count << " employee(s)." << std::endl;
            return false;
        }
    }
    
    const char* sql = "DELETE FROM positions WHERE position_id = ?";
    
    sqlite3_stmt* stmt;
    result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, positionID);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing delete: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "Position deleted successfully." << std::endl;
    return true;
}

bool Position::getPositionByID(Database& db, int positionID, Position& position) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    const char* sql = "SELECT * FROM positions WHERE position_id = ?";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, positionID);
    
    result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        position.setPositionID(sqlite3_column_int(stmt, 0));
        position.setPositionName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        position.setPayType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        position.setDefaultRate(sqlite3_column_double(stmt, 3));
        
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

void Position::displayAllPositions(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return;
    }
    
    const char* sql = "SELECT * FROM positions ORDER BY position_id";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return;
    }
    
    std::cout << "\n========== ALL POSITIONS ==========" << std::endl;
    std::cout << std::left << std::setw(5) << "ID" << std::setw(20) << "Position Name" 
              << std::setw(12) << "Pay Type" << std::setw(15) << "Default Rate" << std::endl;
    std::cout << std::string(52, '-') << std::endl;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << std::left << std::setw(5) << sqlite3_column_int(stmt, 0)
                  << std::setw(20) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))
                  << std::setw(12) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
                  << std::fixed << std::setprecision(2) << sqlite3_column_double(stmt, 3) << std::endl;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "===================================\n" << std::endl;
}
