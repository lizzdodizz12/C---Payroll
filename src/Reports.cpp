#include "Reports.h"
#include <filesystem>
#include <fstream>
#include <iostream>

void Reports::payrollSummary(Database& db, const std::string& periodStart, const std::string& periodEnd) {
    const char* sql = "SELECT COUNT(*), COALESCE(SUM(gross_pay),0), COALESCE(SUM(total_deductions),0), COALESCE(SUM(net_pay),0) "
                      "FROM payroll WHERE period_start=? AND period_end=?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(statement, 1, periodStart.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, periodEnd.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) == SQLITE_ROW) {
        std::cout << "Payroll records: " << sqlite3_column_int(statement, 0)
                  << " | Gross: " << sqlite3_column_double(statement, 1)
                  << " | Deductions: " << sqlite3_column_double(statement, 2)
                  << " | Net expenses: " << sqlite3_column_double(statement, 3) << std::endl;
    }
    sqlite3_finalize(statement);
}

bool Reports::exportPayrollCSV(Database& db, const std::string& filePath) {
    const std::filesystem::path outputPath(filePath);
    if (outputPath.has_parent_path()) std::filesystem::create_directories(outputPath.parent_path());
    std::ofstream output(filePath);
    if (!output) return false;
    output << "Payroll ID,Employee ID,Employee Name,Period Start,Period End,Basic Pay,Overtime Pay,Bonuses,Other Earnings,Gross Pay,Total Deductions,Net Pay,Payment Status\n";
    const char* sql = "SELECT p.payroll_id,p.employee_id,e.first_name || ' ' || e.last_name,p.period_start,p.period_end,p.basic_pay,p.overtime_pay,p.bonuses,p.other_earnings,p.gross_pay,p.total_deductions,p.net_pay,p.payment_status FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.period_start";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        for (int column = 0; column < 13; ++column) {
            if (column) output << ',';
            if (column == 0 || column == 1) output << sqlite3_column_int(statement, column);
            else if (column >= 5 && column <= 11) output << sqlite3_column_double(statement, column);
            else output << '"' << reinterpret_cast<const char*>(sqlite3_column_text(statement, column)) << '"';
        }
        output << '\n';
    }
    sqlite3_finalize(statement);
    return true;
}
