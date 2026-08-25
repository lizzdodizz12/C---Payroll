#ifndef PAYROLL_H
#define PAYROLL_H

#include "Database.h"

class Payroll {
public:
    static bool process(Database& db, int employeeID, const std::string& periodStart,
                        const std::string& periodEnd, double bonuses,
                        double otherEarnings, double overtimeMultiplier,
                        const std::string& deductions);
    static void displayHistory(Database& db);
    static void displayEmployeeHistory(Database& db, int employeeID);
    static bool markPaid(Database& db, int payrollID);
};

#endif // PAYROLL_H
