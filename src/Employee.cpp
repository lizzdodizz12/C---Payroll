#include "../include/Employee.h"
#include <iostream>
#include <iomanip>

Employee::Employee() 
    : employeeID(0), firstName(""), lastName(""), positionID(0), 
      department(""), employmentType(""), dateHired(""), 
      contactNumber(""), email(""), employmentStatus("Active") {}

Employee::~Employee() {}

// Setter methods
void Employee::setEmployeeID(int id) {
    employeeID = id;
}

void Employee::setFirstName(const std::string& name) {
    firstName = name;
}

void Employee::setLastName(const std::string& name) {
    lastName = name;
}

void Employee::setPositionID(int id) {
    positionID = id;
}

void Employee::setDepartment(const std::string& dept) {
    department = dept;
}

void Employee::setEmploymentType(const std::string& type) {
    employmentType = type;
}

void Employee::setDateHired(const std::string& date) {
    dateHired = date;
}

void Employee::setContactNumber(const std::string& number) {
    contactNumber = number;
}

void Employee::setEmail(const std::string& email_addr) {
    email = email_addr;
}

void Employee::setEmploymentStatus(const std::string& status) {
    employmentStatus = status;
}

// Getter methods
int Employee::getEmployeeID() const {
    return employeeID;
}

std::string Employee::getFirstName() const {
    return firstName;
}

std::string Employee::getLastName() const {
    return lastName;
}

std::string Employee::getFullName() const {
    return firstName + " " + lastName;
}

int Employee::getPositionID() const {
    return positionID;
}

std::string Employee::getDepartment() const {
    return department;
}

std::string Employee::getEmploymentType() const {
    return employmentType;
}

std::string Employee::getDateHired() const {
    return dateHired;
}

std::string Employee::getContactNumber() const {
    return contactNumber;
}

std::string Employee::getEmail() const {
    return email;
}

std::string Employee::getEmploymentStatus() const {
    return employmentStatus;
}

// Database operations
bool Employee::addEmployee(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    if (firstName.empty() || lastName.empty() || department.empty() ||
        employmentType.empty() || dateHired.empty() || positionID <= 0) {
        std::cerr << "ERROR: Required employee fields are missing." << std::endl;
        return false;
    }

    const char* positionSql = "SELECT 1 FROM positions WHERE position_id = ?";
    sqlite3_stmt* positionStatement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), positionSql, -1, &positionStatement, nullptr) != SQLITE_OK) {
        std::cerr << "ERROR checking position: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    sqlite3_bind_int(positionStatement, 1, positionID);
    const bool positionExists = sqlite3_step(positionStatement) == SQLITE_ROW;
    sqlite3_finalize(positionStatement);
    if (!positionExists) {
        std::cerr << "ERROR: The selected position does not exist." << std::endl;
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO employees 
        (first_name, last_name, position_id, department, employment_type, 
         date_hired, contact_number, email, employment_status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    // Bind parameters
    sqlite3_bind_text(stmt, 1, firstName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, lastName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, positionID);
    sqlite3_bind_text(stmt, 4, department.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, employmentType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, dateHired.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, contactNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, employmentStatus.c_str(), -1, SQLITE_STATIC);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing insert: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    // Get the inserted employee ID
    employeeID = sqlite3_last_insert_rowid(db.getDB());
    sqlite3_finalize(stmt);
    
    std::cout << "Employee added successfully with ID: " << employeeID << std::endl;
    return true;
}

bool Employee::updateEmployee(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    const char* sql = R"(
        UPDATE employees 
        SET first_name = ?, last_name = ?, position_id = ?, department = ?, 
            employment_type = ?, date_hired = ?, contact_number = ?, 
            email = ?, employment_status = ?
        WHERE employee_id = ?
    )";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    // Bind parameters
    sqlite3_bind_text(stmt, 1, firstName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, lastName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, positionID);
    sqlite3_bind_text(stmt, 4, department.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, employmentType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, dateHired.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, contactNumber.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, employmentStatus.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 10, employeeID);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing update: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "Employee updated successfully." << std::endl;
    return true;
}

bool Employee::deleteEmployee(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    const char* sql = "DELETE FROM employees WHERE employee_id = ?";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, employeeID);
    
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR executing delete: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "Employee deleted successfully." << std::endl;
    return true;
}

bool Employee::getEmployeeByID(Database& db, int employeeID, Employee& employee) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return false;
    }
    
    const char* sql = "SELECT * FROM employees WHERE employee_id = ?";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, employeeID);
    
    result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        employee.setEmployeeID(sqlite3_column_int(stmt, 0));
        employee.setFirstName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        employee.setLastName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        employee.setPositionID(sqlite3_column_int(stmt, 3));
        employee.setDepartment(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        employee.setEmploymentType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        employee.setDateHired(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        employee.setContactNumber(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        employee.setEmail(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        employee.setEmploymentStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

void Employee::displayAllEmployees(Database& db) {
    if (!db.isConnected()) {
        std::cerr << "Database not connected!" << std::endl;
        return;
    }
    
    const char* sql = "SELECT * FROM employees ORDER BY employee_id";
    
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db.getDB(), sql, -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "ERROR preparing statement: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return;
    }
    
    std::cout << "\n========== ALL EMPLOYEES ==========" << std::endl;
    std::cout << std::left << std::setw(5) << "ID" << std::setw(15) << "First Name" 
              << std::setw(15) << "Last Name" << std::setw(8) << "Pos ID" 
              << std::setw(15) << "Department" << std::setw(12) << "Type" 
              << std::setw(12) << "Status" << std::endl;
    std::cout << std::string(82, '-') << std::endl;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << std::left << std::setw(5) << sqlite3_column_int(stmt, 0)
                  << std::setw(15) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))
                  << std::setw(15) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
                  << std::setw(8) << sqlite3_column_int(stmt, 3)
                  << std::setw(15) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))
                  << std::setw(12) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))
                  << std::setw(12) << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)) << std::endl;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "===================================\n" << std::endl;
}

void Employee::displayEmployee() const {
    std::cout << "\n===== EMPLOYEE DETAILS =====" << std::endl;
    std::cout << "Employee ID: " << employeeID << std::endl;
    std::cout << "Name: " << getFullName() << std::endl;
    std::cout << "Position ID: " << positionID << std::endl;
    std::cout << "Department: " << department << std::endl;
    std::cout << "Employment Type: " << employmentType << std::endl;
    std::cout << "Date Hired: " << dateHired << std::endl;
    std::cout << "Contact Number: " << contactNumber << std::endl;
    std::cout << "Email: " << email << std::endl;
    std::cout << "Status: " << employmentStatus << std::endl;
    std::cout << "===========================\n" << std::endl;
}
