#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>
#include "Database.h"

class Employee {
private:
    int employeeID;
    std::string firstName;
    std::string lastName;
    int positionID;
    std::string department;
    std::string employmentType;
    std::string dateHired;
    std::string contactNumber;
    std::string email;
    std::string employmentStatus;

public:
    // Constructor and Destructor
    Employee();
    ~Employee();
    
    // Setter methods
    void setEmployeeID(int id);
    void setFirstName(const std::string& name);
    void setLastName(const std::string& name);
    void setPositionID(int id);
    void setDepartment(const std::string& dept);
    void setEmploymentType(const std::string& type);
    void setDateHired(const std::string& date);
    void setContactNumber(const std::string& number);
    void setEmail(const std::string& email);
    void setEmploymentStatus(const std::string& status);
    
    // Getter methods
    int getEmployeeID() const;
    std::string getFirstName() const;
    std::string getLastName() const;
    std::string getFullName() const;
    int getPositionID() const;
    std::string getDepartment() const;
    std::string getEmploymentType() const;
    std::string getDateHired() const;
    std::string getContactNumber() const;
    std::string getEmail() const;
    std::string getEmploymentStatus() const;
    
    // Database operations
    bool addEmployee(Database& db);
    bool updateEmployee(Database& db);
    bool deleteEmployee(Database& db);
    static bool getEmployeeByID(Database& db, int employeeID, Employee& employee);
    static void displayAllEmployees(Database& db);
    void displayEmployee() const;
};

#endif // EMPLOYEE_H
