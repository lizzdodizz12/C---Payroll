#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <sqlite3.h>
#include <iostream>

class Database {
private:
    sqlite3* db;
    std::string dbPath;
    
    // Private helper method
    void executeSQL(const std::string& sql);

public:
    // Constructor and Destructor
    Database(const std::string& path = "database/payroll.db");
    ~Database();
    
    // Connection management
    bool connect();
    bool disconnect();
    bool isConnected() const;
    
    // Database initialization
    bool initializeDatabase();
    
    // Table creation
    bool createUsersTable();
    bool createEmployeesTable();
    bool createPositionsTable();
    bool createAttendanceTable();
    bool createPayrollTable();
    bool createPayrollDeductionsTable();
    
    // Getter for database pointer
    sqlite3* getDB() const;
};

#endif // DATABASE_H
