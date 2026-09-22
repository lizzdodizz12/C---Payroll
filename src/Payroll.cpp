#include "Payroll.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace {
struct Deduction { std::string name; double amount; };
std::vector<Deduction> parseDeductions(const std::string& input) {
    std::vector<Deduction> result;
    std::stringstream items(input);
    std::string item;
    while (std::getline(items, item, ',')) {
        const std::size_t separator = item.find('=');
        if (separator == std::string::npos) continue;
        try {
            const double amount = std::stod(item.substr(separator + 1));
            if (!item.substr(0, separator).empty() && amount >= 0) {
                result.push_back({item.substr(0, separator), amount});
            }
        } catch (...) { }
    }
    return result;
}

bool parseAttendancePeriod(const std::string& period, std::string& startDate, std::string& endDate) {
    const std::string marker = " to ";
    const std::size_t separator = period.find(marker);
    if (separator == std::string::npos) return false;
    startDate = period.substr(0, separator);
    endDate = period.substr(separator + marker.size());
    return startDate.size() == 10 && endDate.size() == 10;
}

bool periodOverlaps(const std::string& recordPeriod, const std::string& selectedStart, const std::string& selectedEnd) {
    std::string recordStart;
    std::string recordEnd;
    if (!parseAttendancePeriod(recordPeriod, recordStart, recordEnd)) return false;
    return recordStart <= selectedEnd && recordEnd >= selectedStart;
}
}

bool Payroll::process(Database& db, int employeeID, const std::string& periodStart,
                      const std::string& periodEnd, double bonuses, double otherEarnings,
                      double overtimeMultiplier, const std::string& deductions) {
    if (!db.isConnected() || employeeID <= 0 || periodStart.empty() || periodEnd.empty() ||
        bonuses < 0 || otherEarnings < 0 || overtimeMultiplier <= 0) return false;
    if (periodStart > periodEnd) return false;

    const char* sourceSql = "SELECT p.pay_type, p.default_rate, a.payroll_period, a.days_worked, a.hours_worked, a.overtime_hours "
                            "FROM employees e JOIN positions p ON p.position_id=e.position_id "
                            "LEFT JOIN attendance a ON a.employee_id=e.employee_id "
                            "WHERE e.employee_id=?";
    sqlite3_stmt* source = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sourceSql, -1, &source, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(source, 1, employeeID);

    double totalDays = 0.0;
    double totalHours = 0.0;
    double totalOvertime = 0.0;
    std::string payType;
    double rate = 0.0;
    bool foundEmployee = false;

    while (sqlite3_step(source) == SQLITE_ROW) {
        if (!foundEmployee) {
            payType = reinterpret_cast<const char*>(sqlite3_column_text(source, 0));
            rate = sqlite3_column_double(source, 1);
            foundEmployee = true;
        }

        const unsigned char* periodValue = sqlite3_column_text(source, 2);
        if (!periodValue) continue;

        const std::string recordPeriod = reinterpret_cast<const char*>(periodValue);
        if (!periodOverlaps(recordPeriod, periodStart, periodEnd)) continue;

        totalDays += sqlite3_column_double(source, 3);
        totalHours += sqlite3_column_double(source, 4);
        totalOvertime += sqlite3_column_double(source, 5);
    }
    sqlite3_finalize(source);

    if (!foundEmployee) return false;

    const double basicPay = payType == "Monthly" ? rate : (payType == "Daily" ? rate * totalDays : rate * totalHours);
    const double hourlyRate = payType == "Hourly" ? rate : (payType == "Daily" ? rate / 8.0 : rate / 22.0 / 8.0);
    const double overtimePay = hourlyRate * totalOvertime * overtimeMultiplier;
    const double grossPay = basicPay + overtimePay + bonuses + otherEarnings;
    const std::vector<Deduction> parsed = parseDeductions(deductions);
    double totalDeductions = 0;
    for (const Deduction& deduction : parsed) totalDeductions += deduction.amount;

    if (sqlite3_exec(db.getDB(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr) != SQLITE_OK) return false;
    const char* insertSql = "INSERT INTO payroll (employee_id, period_start, period_end, basic_pay, overtime_pay, "
                            "bonuses, other_earnings, gross_pay, total_deductions, net_pay) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* insert = nullptr;
    bool success = sqlite3_prepare_v2(db.getDB(), insertSql, -1, &insert, nullptr) == SQLITE_OK;
    if (success) {
        sqlite3_bind_int(insert, 1, employeeID);
        sqlite3_bind_text(insert, 2, periodStart.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 3, periodEnd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(insert, 4, basicPay); sqlite3_bind_double(insert, 5, overtimePay);
        sqlite3_bind_double(insert, 6, bonuses); sqlite3_bind_double(insert, 7, otherEarnings);
        sqlite3_bind_double(insert, 8, grossPay); sqlite3_bind_double(insert, 9, totalDeductions);
        sqlite3_bind_double(insert, 10, grossPay - totalDeductions);
        success = sqlite3_step(insert) == SQLITE_DONE;
    }
    if (insert) sqlite3_finalize(insert);
    if (success) {
        const sqlite3_int64 payrollID = sqlite3_last_insert_rowid(db.getDB());
        const char* deductionSql = "INSERT INTO payroll_deductions (payroll_id, deduction_name, deduction_amount) VALUES (?, ?, ?)";
        for (const Deduction& deduction : parsed) {
            sqlite3_stmt* statement = nullptr;
            success = sqlite3_prepare_v2(db.getDB(), deductionSql, -1, &statement, nullptr) == SQLITE_OK;
            if (success) {
                sqlite3_bind_int64(statement, 1, payrollID);
                sqlite3_bind_text(statement, 2, deduction.name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(statement, 3, deduction.amount);
                success = sqlite3_step(statement) == SQLITE_DONE;
            }
            if (statement) sqlite3_finalize(statement);
            if (!success) break;
        }
    }
    sqlite3_exec(db.getDB(), success ? "COMMIT" : "ROLLBACK", nullptr, nullptr, nullptr);
    if (!success) std::cerr << "Payroll was not saved. It may already exist for this period." << std::endl;
    else std::cout << "Payroll saved. Gross: " << grossPay << " Net: " << grossPay - totalDeductions << std::endl;
    return success;
}

void Payroll::displayHistory(Database& db) {
    const char* sql = "SELECT p.payroll_id, p.employee_id, e.first_name || ' ' || e.last_name, "
                      "p.period_start, p.period_end, p.gross_pay, p.total_deductions, p.net_pay, p.payment_status "
                      "FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.period_start DESC";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return;
    std::cout << "\nID  Employee  Name                    Period                 Gross       Deductions  Net         Status\n";
    while (sqlite3_step(statement) == SQLITE_ROW) {
        std::cout << sqlite3_column_int(statement, 0) << "   " << sqlite3_column_int(statement, 1) << "         "
                  << reinterpret_cast<const char*>(sqlite3_column_text(statement, 2)) << "   "
                  << reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)) << " to "
                  << reinterpret_cast<const char*>(sqlite3_column_text(statement, 4)) << "   "
                  << sqlite3_column_double(statement, 5) << "   " << sqlite3_column_double(statement, 6) << "   "
                  << sqlite3_column_double(statement, 7) << "   " << reinterpret_cast<const char*>(sqlite3_column_text(statement, 8)) << std::endl;
    }
    sqlite3_finalize(statement);
}

void Payroll::displayEmployeeHistory(Database& db, int employeeID) {
    const char* sql = "SELECT period_start, period_end, basic_pay, overtime_pay, bonuses, other_earnings, "
                      "gross_pay, total_deductions, net_pay, payment_status FROM payroll WHERE employee_id=? ORDER BY period_start DESC";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(statement, 1, employeeID);
    while (sqlite3_step(statement) == SQLITE_ROW) {
        std::cout << reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)) << " to "
                  << reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)) << " | Basic: " << sqlite3_column_double(statement, 2)
                  << " | OT: " << sqlite3_column_double(statement, 3) << " | Gross: " << sqlite3_column_double(statement, 6)
                  << " | Deductions: " << sqlite3_column_double(statement, 7) << " | Net: " << sqlite3_column_double(statement, 8)
                  << " | " << reinterpret_cast<const char*>(sqlite3_column_text(statement, 9)) << std::endl;
    }
    sqlite3_finalize(statement);
}

bool Payroll::markPaid(Database& db, int payrollID) {
    const char* sql = "UPDATE payroll SET payment_status='Paid' WHERE payroll_id=?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(statement, 1, payrollID);
    const bool success = sqlite3_step(statement) == SQLITE_DONE && sqlite3_changes(db.getDB()) > 0;
    sqlite3_finalize(statement);
    return success;
}
