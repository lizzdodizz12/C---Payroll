#include "../include/Database.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Database::Database(const std::string& path) : db(nullptr), dbPath(path) {
    // Ensure database directory exists
    std::string dbDir = dbPath.substr(0, dbPath.find_last_of("/\\"));
    if (!fs::exists(dbDir)) {
        fs::create_directories(dbDir);
    }
}

Database::~Database() {
    disconnect();
}

bool Database::connect() {
    int result = sqlite3_open(dbPath.c_str(), &db);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR: Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    std::cout << "Database connected successfully: " << dbPath << std::endl;
    return true;
}

bool Database::disconnect() {
    if (db != nullptr) {
        int result = sqlite3_close(db);
        if (result != SQLITE_OK) {
            std::cerr << "ERROR: Cannot close database: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        db = nullptr;
        std::cout << "Database disconnected." << std::endl;
    }
    return true;
}

bool Database::isConnected() const {
    return db != nullptr;
}

sqlite3* Database::getDB() const {
    return db;
}

void Database::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

bool Database::initializeDatabase() {
    if (!isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    // Create all tables
    if (!createUsersTable()) return false;
    if (!createPositionsTable()) return false;
    if (!createEmployeesTable()) return false;
    if (!createAttendanceTable()) return false;
    if (!createPayrollTable()) return false;
    if (!createPayrollDeductionsTable()) return false;
    
    std::cout << "Database initialized successfully!" << std::endl;
    return true;
}

bool Database::createUsersTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('Administrator', 'Payroll Staff')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Users table created or already exists." << std::endl;
    return true;
}

bool Database::createPositionsTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS positions (
            position_id INTEGER PRIMARY KEY AUTOINCREMENT,
            position_name TEXT UNIQUE NOT NULL,
            pay_type TEXT NOT NULL CHECK(pay_type IN ('Monthly', 'Daily', 'Hourly')),
            default_rate REAL NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating positions table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Positions table created or already exists." << std::endl;
    return true;
}

bool Database::createEmployeesTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS employees (
            employee_id INTEGER PRIMARY KEY AUTOINCREMENT,
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            position_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            employment_type TEXT NOT NULL CHECK(employment_type IN ('Regular', 'Contractual', 'Part-Time', 'Full-Time')),
            date_hired DATE NOT NULL,
            contact_number TEXT,
            email TEXT,
            employment_status TEXT NOT NULL DEFAULT 'Active' CHECK(employment_status IN ('Active', 'Inactive')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (position_id) REFERENCES positions(position_id)
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating employees table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Employees table created or already exists." << std::endl;
    return true;
}

bool Database::createAttendanceTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS attendance (
            attendance_id INTEGER PRIMARY KEY AUTOINCREMENT,
            employee_id INTEGER NOT NULL,
            payroll_period TEXT NOT NULL,
            days_worked REAL DEFAULT 0,
            hours_worked REAL DEFAULT 0,
            overtime_hours REAL DEFAULT 0,
            absences INTEGER DEFAULT 0,
            leave_days INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (employee_id) REFERENCES employees(employee_id),
            UNIQUE(employee_id, payroll_period)
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating attendance table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Attendance table created or already exists." << std::endl;
    return true;
}

bool Database::createPayrollTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS payroll (
            payroll_id INTEGER PRIMARY KEY AUTOINCREMENT,
            employee_id INTEGER NOT NULL,
            period_start DATE NOT NULL,
            period_end DATE NOT NULL,
            basic_pay REAL NOT NULL DEFAULT 0,
            overtime_pay REAL NOT NULL DEFAULT 0,
            bonuses REAL NOT NULL DEFAULT 0,
            other_earnings REAL NOT NULL DEFAULT 0,
            gross_pay REAL NOT NULL DEFAULT 0,
            total_deductions REAL NOT NULL DEFAULT 0,
            net_pay REAL NOT NULL DEFAULT 0,
            payment_status TEXT NOT NULL DEFAULT 'Unpaid' CHECK(payment_status IN ('Paid', 'Unpaid', 'Pending')),
            date_processed DATETIME DEFAULT CURRENT_TIMESTAMP,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (employee_id) REFERENCES employees(employee_id),
            UNIQUE(employee_id, period_start, period_end)
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating payroll table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Payroll table created or already exists." << std::endl;
    return true;
}

bool Database::createPayrollDeductionsTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS payroll_deductions (
            deduction_id INTEGER PRIMARY KEY AUTOINCREMENT,
            payroll_id INTEGER NOT NULL,
            deduction_name TEXT NOT NULL,
            deduction_amount REAL NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (payroll_id) REFERENCES payroll(payroll_id) ON DELETE CASCADE
        );
    )";
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        std::cerr << "ERROR creating payroll_deductions table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "Payroll Deductions table created or already exists." << std::endl;
    return true;
}
