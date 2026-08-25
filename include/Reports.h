#ifndef REPORTS_H
#define REPORTS_H

#include "Database.h"

class Reports {
public:
    static void payrollSummary(Database& db, const std::string& periodStart,
                               const std::string& periodEnd);
    static bool exportPayrollCSV(Database& db, const std::string& filePath);
};

#endif // REPORTS_H
