#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include "Database.h"
#include "User.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(Database& database, const User& currentUser, QWidget* parent = nullptr);

private slots:
    void showDashboard();
    void showEmployees();
    void showPositions();
    void showAttendance();
    void showPayroll();
    void showHistory();
    void showReports();
    void addEmployee();
    void editEmployee();
    void deactivateEmployee();
    void addPosition();
    void editPosition();
    void deletePosition();
    void addAttendance();
    void updateAttendance();
    void deleteAttendance();
    void processPayroll();
    void markSelectedPaid();
    void exportCsv();
    void refreshCurrentPage();
    void toggleTheme();

private:
    void applyTheme();
    Database& db;
    User user;
    QStackedWidget* pages;
    QAction* themeAction;
    bool darkMode;
    QTableWidget* employeeTable;
    QTableWidget* positionTable;
    QTableWidget* attendanceTable;
    QTableWidget* historyTable;
    QTableWidget* reportTable;
    QLabel* dashboardStats;
    QLineEdit* employeeSearch;
    QComboBox* payrollEmployee;
    QDateEdit* payrollStart;
    QDateEdit* payrollEnd;
    QDoubleSpinBox* payrollBonus;
    QDoubleSpinBox* payrollOther;
    QDoubleSpinBox* payrollMultiplier;
    QLineEdit* deductionInput;
    QLabel* payrollSummary;
    QComboBox* reportType;
    QDateEdit* reportStart;
    QDateEdit* reportEnd;
    QComboBox* reportEmployee;

    QWidget* createDashboardPage();
    QWidget* createEmployeesPage();
    QWidget* createPositionsPage();
    QWidget* createAttendancePage();
    QWidget* createPayrollPage();
    QWidget* createHistoryPage();
    QWidget* createReportsPage();
    void configureTable(QTableWidget* table, const QStringList& headers);
    void loadEmployees();
    void loadPositions();
    void loadAttendance();
    void loadHistory();
    void loadEmployeeChoices(QComboBox* combo, bool includeAll = false);
    void updateDashboard();
    bool isAdministrator() const;
    int selectedID(QTableWidget* table) const;
};

#endif // MAINWINDOW_H
