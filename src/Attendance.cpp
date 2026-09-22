#include "Attendance.h"
#include <iostream>

bool Attendance::save(Database& db, int employeeID, const std::string& period,
                      double daysWorked, double hoursWorked, double overtimeHours,
                      int absences, int leaveDays) {
    if (!db.isConnected() || employeeID <= 0 || period.empty() || daysWorked < 0 ||
        hoursWorked < 0 || overtimeHours < 0 || absences < 0 || leaveDays < 0) {
        std::cerr << "Invalid attendance data." << std::endl;
        return false;
    }
    const char* sql = "INSERT INTO attendance (employee_id, payroll_period, days_worked, "
                      "hours_worked, overtime_hours, absences, leave_days) VALUES (?, ?, ?, ?, ?, ?, ?) "
                      "ON CONFLICT(employee_id, payroll_period) DO UPDATE SET days_worked=excluded.days_worked, "
                      "hours_worked=excluded.hours_worked, overtime_hours=excluded.overtime_hours, "
                      "absences=excluded.absences, leave_days=excluded.leave_days";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(statement, 1, employeeID);
    sqlite3_bind_text(statement, 2, period.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 3, daysWorked);
    sqlite3_bind_double(statement, 4, hoursWorked);
    sqlite3_bind_double(statement, 5, overtimeHours);
    sqlite3_bind_int(statement, 6, absences);
    sqlite3_bind_int(statement, 7, leaveDays);
    const bool success = sqlite3_step(statement) == SQLITE_DONE;
    if (!success) std::cerr << "Attendance error: " << sqlite3_errmsg(db.getDB()) << std::endl;
    sqlite3_finalize(statement);
    return success;
}

bool Attendance::remove(Database& db, int employeeID, const std::string& period) {
    if (!db.isConnected() || employeeID <= 0 || period.empty()) return false;

    const char* sql = "DELETE FROM attendance WHERE employee_id=? AND payroll_period=?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(statement, 1, employeeID);
    sqlite3_bind_text(statement, 2, period.c_str(), -1, SQLITE_TRANSIENT);
    const bool success = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

void Attendance::display(Database& db, const std::string& period) {
    const char* sql = "SELECT a.employee_id, e.first_name || ' ' || e.last_name, a.days_worked, "
                      "a.hours_worked, a.overtime_hours, a.absences, a.leave_days "
                      "FROM attendance a JOIN employees e ON e.employee_id=a.employee_id "
                      "WHERE a.payroll_period=? ORDER BY a.employee_id";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(statement, 1, period.c_str(), -1, SQLITE_TRANSIENT);
    std::cout << "\nID  Employee                 Days  Hours  OT Hours  Absences  Leave\n";
    while (sqlite3_step(statement) == SQLITE_ROW) {
        std::cout << sqlite3_column_int(statement, 0) << "   "
                  << reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)) << "   "
                  << sqlite3_column_double(statement, 2) << "   "
                  << sqlite3_column_double(statement, 3) << "   "
                  << sqlite3_column_double(statement, 4) << "        "
                  << sqlite3_column_int(statement, 5) << "         "
                  << sqlite3_column_int(statement, 6) << std::endl;
    }
    sqlite3_finalize(statement);
}
