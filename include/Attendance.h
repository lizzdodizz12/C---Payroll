#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include "Database.h"

class Attendance {
public:
    static bool save(Database& db, int employeeID, const std::string& period,
                     double daysWorked, double hoursWorked, double overtimeHours,
                     int absences, int leaveDays);
    static void display(Database& db, const std::string& period);
};

#endif // ATTENDANCE_H
