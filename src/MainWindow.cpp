#include "MainWindow.h"
#include "Employee.h"
#include "Position.h"
#include "Attendance.h"
#include "Payroll.h"
#include "Reports.h"
#include <QApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStatusBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QDate>
#include <cmath>
#include <functional>

namespace {
QString text(sqlite3_stmt* statement, int column) {
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? QString::fromUtf8(reinterpret_cast<const char*>(value)) : QString();
}
void fillTable(QTableWidget* table, sqlite3* db, const char* sql,
               const std::function<void(sqlite3_stmt*)>& row) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) return;
    table->setRowCount(0);
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const int currentRow = table->rowCount();
        table->insertRow(currentRow);
        row(statement);
    }
    sqlite3_finalize(statement);
}
QDoubleSpinBox* moneyBox(QWidget* parent) {
    auto* box = new QDoubleSpinBox(parent);
    box->setRange(0, 1000000000);
    box->setDecimals(2);
    box->setPrefix(QString::fromUtf8("P "));
    return box;
}
QDateEdit* dateBox(QWidget* parent) {
    auto* box = new QDateEdit(QDate::currentDate(), parent);
    box->setCalendarPopup(true);
    box->setDisplayFormat("yyyy-MM-dd");
    return box;
}
}

MainWindow::MainWindow(Database& database, const User& currentUser, QWidget* parent)
    : QMainWindow(parent), db(database), user(currentUser), pages(new QStackedWidget(this)),
      employeeTable(nullptr), positionTable(nullptr), attendanceTable(nullptr), historyTable(nullptr),
      reportTable(nullptr), dashboardStats(nullptr), employeeSearch(nullptr), payrollEmployee(nullptr),
      payrollStart(nullptr), payrollEnd(nullptr), payrollBonus(nullptr), payrollOther(nullptr),
      payrollMultiplier(nullptr), deductionInput(nullptr), payrollSummary(nullptr), reportType(nullptr),
      reportStart(nullptr), reportEnd(nullptr), reportEmployee(nullptr) {
    setWindowTitle("Payroll Management System");
    resize(1100, 700);
    setMinimumSize(900, 600);
    setStyleSheet("QMainWindow { background: #d4d0c8; } QGroupBox { font-weight: bold; } "
                  "QPushButton { padding: 4px 12px; } QTableWidget { background: white; }");

    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("Refresh", this, &MainWindow::refreshCurrentPage);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", qApp, &QApplication::quit);
    auto* employeesMenu = menuBar()->addMenu("&Employees");
    employeesMenu->addAction("Employee List", this, &MainWindow::showEmployees);
    employeesMenu->addAction("Positions", this, &MainWindow::showPositions);
    auto* payrollMenu = menuBar()->addMenu("&Payroll");
    payrollMenu->addAction("Attendance", this, &MainWindow::showAttendance);
    payrollMenu->addAction("Process Payroll", this, &MainWindow::showPayroll);
    payrollMenu->addAction("Payroll History", this, &MainWindow::showHistory);
    auto* reportsMenu = menuBar()->addMenu("&Reports");
    reportsMenu->addAction("Reports", this, &MainWindow::showReports);
    menuBar()->addMenu("&Help")->addAction("About", this, [this] {
        QMessageBox::information(this, "About", "Payroll Management System\nQt Widgets and SQLite");
    });

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);
    auto* navigation = new QListWidget(central);
    navigation->addItems({"Dashboard", "Employees", "Positions", "Attendance", "Process Payroll", "Payroll History", "Reports", "Exit"});
    navigation->setFixedWidth(170);
    navigation->setCurrentRow(0);
    mainLayout->addWidget(navigation);
    mainLayout->addWidget(pages, 1);
    setCentralWidget(central);
    statusBar()->showMessage("Ready | " + QString::fromStdString(user.getUsername()) + " (" + QString::fromStdString(user.getRole()) + ")");

    pages->addWidget(createDashboardPage());
    pages->addWidget(createEmployeesPage());
    pages->addWidget(createPositionsPage());
    pages->addWidget(createAttendancePage());
    pages->addWidget(createPayrollPage());
    pages->addWidget(createHistoryPage());
    pages->addWidget(createReportsPage());
    connect(navigation, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row == 7) qApp->quit(); else pages->setCurrentIndex(row);
        refreshCurrentPage();
    });
    showDashboard();
}

bool MainWindow::isAdministrator() const { return user.getRole() == "Administrator"; }
void MainWindow::configureTable(QTableWidget* table, const QStringList& headers) {
    table->setColumnCount(headers.size()); table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->horizontalHeader()->setStretchLastSection(true);
}
int MainWindow::selectedID(QTableWidget* table) const {
    auto* item = table->currentItem(); return item ? item->data(Qt::UserRole).toInt() : 0;
}
QWidget* MainWindow::createDashboardPage() {
    auto* page = new QWidget; auto* layout = new QVBoxLayout(page);
    auto* title = new QLabel("PAYROLL MANAGEMENT SYSTEM"); title->setStyleSheet("font-size: 20px; font-weight: bold;"); layout->addWidget(title);
    dashboardStats = new QLabel; dashboardStats->setFrameStyle(QFrame::Panel | QFrame::Sunken); dashboardStats->setMinimumHeight(100); layout->addWidget(dashboardStats);
    auto* group = new QGroupBox("Recent Payroll Records"); auto* groupLayout = new QVBoxLayout(group);
    auto* recent = new QTableWidget; configureTable(recent, {"Employee", "Period", "Net Pay", "Status"}); groupLayout->addWidget(recent); layout->addWidget(group, 1);
    fillTable(recent, db.getDB(), "SELECT e.first_name || ' ' || e.last_name,p.period_start || ' to ' || p.period_end,p.net_pay,p.payment_status FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.date_processed DESC LIMIT 10", [recent](sqlite3_stmt* s) { int r=recent->rowCount(); recent->insertRow(r); for(int c=0;c<4;++c) recent->setItem(r,c,new QTableWidgetItem(c==2?QString::number(sqlite3_column_double(s,c),'f',2):text(s,c))); });
    return page;
}
void MainWindow::updateDashboard() {
    const char* sql = "SELECT (SELECT COUNT(*) FROM employees),(SELECT COUNT(*) FROM employees WHERE employment_status='Active'),(SELECT COUNT(*) FROM positions),(SELECT COUNT(*) FROM payroll)";
    sqlite3_stmt* s=nullptr; if(sqlite3_prepare_v2(db.getDB(),sql,-1,&s,nullptr)==SQLITE_OK && sqlite3_step(s)==SQLITE_ROW) dashboardStats->setText(QString("Total Employees: %1    Active Employees: %2    Total Positions: %3    Payroll Records: %4").arg(sqlite3_column_int(s,0)).arg(sqlite3_column_int(s,1)).arg(sqlite3_column_int(s,2)).arg(sqlite3_column_int(s,3))); if(s)sqlite3_finalize(s);
}
QWidget* MainWindow::createEmployeesPage() {
    auto* page=new QWidget; auto* l=new QVBoxLayout(page); auto* bar=new QHBoxLayout; employeeSearch=new QLineEdit; employeeSearch->setPlaceholderText("Search employee name or department"); auto* search=new QPushButton("Search"); bar->addWidget(employeeSearch); bar->addWidget(search); auto* add=new QPushButton("Add Employee"); auto* edit=new QPushButton("Edit Employee"); auto* deactivate=new QPushButton("Deactivate"); auto* refresh=new QPushButton("Refresh"); bar->addWidget(add);bar->addWidget(edit);bar->addWidget(deactivate);bar->addWidget(refresh);l->addLayout(bar); employeeTable=new QTableWidget; configureTable(employeeTable,{"ID","First Name","Last Name","Position","Department","Employment Type","Contact","Status"});l->addWidget(employeeTable);
    connect(search,&QPushButton::clicked,this,&MainWindow::loadEmployees); connect(refresh,&QPushButton::clicked,this,&MainWindow::loadEmployees); connect(add,&QPushButton::clicked,this,&MainWindow::addEmployee); connect(edit,&QPushButton::clicked,this,&MainWindow::editEmployee); connect(deactivate,&QPushButton::clicked,this,&MainWindow::deactivateEmployee); add->setEnabled(isAdministrator()); edit->setEnabled(isAdministrator()); deactivate->setEnabled(isAdministrator()); return page;
}
void MainWindow::loadEmployees() { QString filter=employeeSearch?employeeSearch->text():QString(); const char* sql="SELECT e.employee_id,e.first_name,e.last_name,p.position_name,e.department,e.employment_type,e.contact_number,e.employment_status FROM employees e JOIN positions p ON p.position_id=e.position_id WHERE (?='' OR e.first_name||' '||e.last_name LIKE '%'||?||'%' OR e.department LIKE '%'||?||'%') ORDER BY e.employee_id"; sqlite3_stmt*s=nullptr;if(sqlite3_prepare_v2(db.getDB(),sql,-1,&s,nullptr)!=SQLITE_OK)return;for(int i=1;i<=3;++i)sqlite3_bind_text(s,i,filter.toUtf8().constData(),-1,SQLITE_TRANSIENT);employeeTable->setRowCount(0);while(sqlite3_step(s)==SQLITE_ROW){int r=employeeTable->rowCount();employeeTable->insertRow(r);for(int c=0;c<8;++c){auto* item=new QTableWidgetItem(text(s,c));if(c==0)item->setData(Qt::UserRole,sqlite3_column_int(s,0));employeeTable->setItem(r,c,item);}}sqlite3_finalize(s);}
void MainWindow::loadEmployeeChoices(QComboBox* combo,bool all) { combo->clear();if(all)combo->addItem("All Employees",0);const char*sql="SELECT employee_id,first_name||' '||last_name FROM employees WHERE employment_status='Active' ORDER BY last_name";sqlite3_stmt*s=nullptr;if(sqlite3_prepare_v2(db.getDB(),sql,-1,&s,nullptr)!=SQLITE_OK)return;while(sqlite3_step(s)==SQLITE_ROW)combo->addItem(text(s,1),sqlite3_column_int(s,0));sqlite3_finalize(s);}
QWidget* MainWindow::createPositionsPage(){auto*p=new QWidget;auto*l=new QVBoxLayout(p);auto*b=new QHBoxLayout;auto*a=new QPushButton("Add Position"),*e=new QPushButton("Edit Position"),*d=new QPushButton("Delete Position"),*r=new QPushButton("Refresh");b->addWidget(a);b->addWidget(e);b->addWidget(d);b->addWidget(r);b->addStretch();l->addLayout(b);positionTable=new QTableWidget;configureTable(positionTable,{"ID","Position Name","Pay Type","Default Rate"});l->addWidget(positionTable);connect(a,&QPushButton::clicked,this,&MainWindow::addPosition);connect(e,&QPushButton::clicked,this,&MainWindow::editPosition);connect(d,&QPushButton::clicked,this,&MainWindow::deletePosition);connect(r,&QPushButton::clicked,this,&MainWindow::loadPositions);a->setEnabled(isAdministrator());e->setEnabled(isAdministrator());d->setEnabled(isAdministrator());return p;}
void MainWindow::loadPositions(){fillTable(positionTable,db.getDB(),"SELECT position_id,position_name,pay_type,default_rate FROM positions ORDER BY position_id",[this](sqlite3_stmt*s){int r=positionTable->rowCount();positionTable->insertRow(r);for(int c=0;c<4;++c){auto*i=new QTableWidgetItem(c==3?QString::number(sqlite3_column_double(s,c),'f',2):text(s,c));if(c==0)i->setData(Qt::UserRole,sqlite3_column_int(s,0));positionTable->setItem(r,c,i);}});}
QWidget* MainWindow::createAttendancePage(){auto*p=new QWidget;auto*l=new QVBoxLayout(p);auto*b=new QHBoxLayout;auto*a=new QPushButton("Add / Update Record"),*r=new QPushButton("Refresh");b->addWidget(a);b->addWidget(r);b->addStretch();l->addLayout(b);attendanceTable=new QTableWidget;configureTable(attendanceTable,{"Employee ID","Employee","Period","Days","Hours","OT Hours","Leave","Absences"});l->addWidget(attendanceTable);connect(a,&QPushButton::clicked,this,&MainWindow::addAttendance);connect(r,&QPushButton::clicked,this,&MainWindow::loadAttendance);return p;}
void MainWindow::loadAttendance(){fillTable(attendanceTable,db.getDB(),"SELECT a.employee_id,e.first_name||' '||e.last_name,a.payroll_period,a.days_worked,a.hours_worked,a.overtime_hours,a.leave_days,a.absences FROM attendance a JOIN employees e ON e.employee_id=a.employee_id ORDER BY a.payroll_period DESC",[this](sqlite3_stmt*s){int r=attendanceTable->rowCount();attendanceTable->insertRow(r);for(int c=0;c<8;++c){auto*i=new QTableWidgetItem((c>=3&&c<=5)?QString::number(sqlite3_column_double(s,c),'f',2):text(s,c));if(c==0)i->setData(Qt::UserRole,sqlite3_column_int(s,0));attendanceTable->setItem(r,c,i);}});}
QWidget* MainWindow::createPayrollPage(){auto*p=new QWidget;auto*l=new QVBoxLayout(p);auto*g=new QGroupBox("Process Payroll");auto*f=new QFormLayout(g);payrollEmployee=new QComboBox;payrollStart=dateBox(g);payrollEnd=dateBox(g);payrollBonus=moneyBox(g);payrollOther=moneyBox(g);payrollMultiplier=new QDoubleSpinBox(g);payrollMultiplier->setRange(0.01,10);payrollMultiplier->setValue(1.25);deductionInput=new QLineEdit(g);deductionInput->setPlaceholderText("Tax=1000,Loan=500");f->addRow("Employee:",payrollEmployee);f->addRow("Period start:",payrollStart);f->addRow("Period end:",payrollEnd);f->addRow("Bonus:",payrollBonus);f->addRow("Other earnings:",payrollOther);f->addRow("OT multiplier:",payrollMultiplier);f->addRow("Deductions:",deductionInput);l->addWidget(g);payrollSummary=new QLabel("Enter values and save payroll.");payrollSummary->setFrameStyle(QFrame::Panel|QFrame::Sunken);l->addWidget(payrollSummary);auto*save=new QPushButton("Calculate and Save Payroll");l->addWidget(save);l->addStretch();connect(save,&QPushButton::clicked,this,&MainWindow::processPayroll);return p;}
QWidget* MainWindow::createHistoryPage(){auto*p=new QWidget;auto*l=new QVBoxLayout(p);auto*b=new QHBoxLayout;auto*r=new QPushButton("Refresh"),*paid=new QPushButton("Mark as Paid");b->addWidget(r);b->addWidget(paid);b->addStretch();l->addLayout(b);historyTable=new QTableWidget;configureTable(historyTable,{"ID","Employee","Start","End","Gross","Deductions","Net Pay","Status"});l->addWidget(historyTable);connect(r,&QPushButton::clicked,this,&MainWindow::loadHistory);connect(paid,&QPushButton::clicked,this,&MainWindow::markSelectedPaid);return p;}
void MainWindow::loadHistory(){fillTable(historyTable,db.getDB(),"SELECT p.payroll_id,e.first_name||' '||e.last_name,p.period_start,p.period_end,p.gross_pay,p.total_deductions,p.net_pay,p.payment_status FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.period_start DESC",[this](sqlite3_stmt*s){int r=historyTable->rowCount();historyTable->insertRow(r);for(int c=0;c<8;++c){auto*i=new QTableWidgetItem((c>=4&&c<=6)?QString::number(sqlite3_column_double(s,c),'f',2):text(s,c));if(c==0)i->setData(Qt::UserRole,sqlite3_column_int(s,0));historyTable->setItem(r,c,i);}});}
QWidget* MainWindow::createReportsPage(){auto*p=new QWidget;auto*l=new QVBoxLayout(p);auto*b=new QHBoxLayout;reportType=new QComboBox;reportType->addItems({"Payroll Summary","Payroll History CSV","Employee List CSV"});reportStart=dateBox(p);reportEnd=dateBox(p);auto*g=new QPushButton("Generate / Export");b->addWidget(reportType);b->addWidget(reportStart);b->addWidget(reportEnd);b->addWidget(g);l->addLayout(b);reportTable=new QTableWidget;configureTable(reportTable,{"Metric","Value"});l->addWidget(reportTable);connect(g,&QPushButton::clicked,this,&MainWindow::exportCsv);return p;}
void MainWindow::showDashboard(){pages->setCurrentIndex(0);updateDashboard();}
void MainWindow::showEmployees(){pages->setCurrentIndex(1);loadEmployees();}
void MainWindow::showPositions(){pages->setCurrentIndex(2);loadPositions();}
void MainWindow::showAttendance(){pages->setCurrentIndex(3);loadAttendance();}
void MainWindow::showPayroll(){pages->setCurrentIndex(4);loadEmployeeChoices(payrollEmployee);}
void MainWindow::showHistory(){pages->setCurrentIndex(5);loadHistory();}
void MainWindow::showReports(){pages->setCurrentIndex(6);}
void MainWindow::refreshCurrentPage(){switch(pages->currentIndex()){case 0:updateDashboard();break;case 1:loadEmployees();break;case 2:loadPositions();break;case 3:loadAttendance();break;case 5:loadHistory();break;default:break;}}
void MainWindow::addPosition(){QDialog d(this);d.setWindowTitle("Add Position");auto*f=new QFormLayout(&d);auto*n=new QLineEdit(&d);auto*t=new QComboBox(&d);t->addItems({"Monthly","Daily","Hourly"});auto*rate=moneyBox(&d);f->addRow("Position Name:",n);f->addRow("Pay Type:",t);f->addRow("Default Rate:",rate);auto*ok=new QPushButton("Save",&d);f->addRow(ok);connect(ok,&QPushButton::clicked,&d,&QDialog::accept);if(d.exec()==QDialog::Accepted){Position p;p.setPositionName(n->text().toStdString());p.setPayType(t->currentText().toStdString());p.setDefaultRate(rate->value());if(!p.addPosition(db))QMessageBox::warning(this,"Position","Unable to save position.");loadPositions();}}
void MainWindow::editPosition(){int id=selectedID(positionTable);if(!id){QMessageBox::warning(this,"Position","Please select a position.");return;}Position p;if(!Position::getPositionByID(db,id,p))return;bool ok=false;QString name=QInputDialog::getText(this,"Edit Position","Position Name:",QLineEdit::Normal,QString::fromStdString(p.getPositionName()),&ok);if(!ok)return;QStringList types={"Monthly","Daily","Hourly"};QString type=QInputDialog::getItem(this,"Edit Position","Pay Type:",types,types.indexOf(QString::fromStdString(p.getPayType())),false,&ok);if(!ok)return;double rate=QInputDialog::getDouble(this,"Edit Position","Default Rate:",p.getDefaultRate(),0,1000000000,2,&ok);if(ok){p.setPositionName(name.toStdString());p.setPayType(type.toStdString());p.setDefaultRate(rate);if(!p.updatePosition(db))QMessageBox::warning(this,"Position","Unable to update position.");loadPositions();}}
void MainWindow::deletePosition(){int id=selectedID(positionTable);if(!id){QMessageBox::warning(this,"Position","Please select a position.");return;}Position p;p.setPositionID(id);if(!p.deletePosition(db))QMessageBox::warning(this,"Position","Position may be assigned to employees.");loadPositions();}
void MainWindow::addEmployee(){QDialog d(this);d.setWindowTitle("Add Employee");auto*f=new QFormLayout(&d);auto*first=new QLineEdit(&d),*last=new QLineEdit(&d),*dept=new QLineEdit(&d),*contact=new QLineEdit(&d),*email=new QLineEdit(&d);auto*pos=new QComboBox(&d);auto*type=new QComboBox(&d);type->addItems({"Regular","Contractual","Part-Time","Full-Time"});auto*hire=dateBox(&d);sqlite3_stmt*s=nullptr;if(sqlite3_prepare_v2(db.getDB(),"SELECT position_id,position_name FROM positions ORDER BY position_name",-1,&s,nullptr)==SQLITE_OK){while(sqlite3_step(s)==SQLITE_ROW)pos->addItem(text(s,1),sqlite3_column_int(s,0));sqlite3_finalize(s);}f->addRow("First Name:",first);f->addRow("Last Name:",last);f->addRow("Position:",pos);f->addRow("Department:",dept);f->addRow("Employment Type:",type);f->addRow("Date Hired:",hire);f->addRow("Contact Number:",contact);f->addRow("Email:",email);auto*ok=new QPushButton("Save",&d);f->addRow(ok);connect(ok,&QPushButton::clicked,&d,&QDialog::accept);if(d.exec()==QDialog::Accepted){Employee e;e.setFirstName(first->text().toStdString());e.setLastName(last->text().toStdString());e.setPositionID(pos->currentData().toInt());e.setDepartment(dept->text().toStdString());e.setEmploymentType(type->currentText().toStdString());e.setDateHired(hire->date().toString("yyyy-MM-dd").toStdString());e.setContactNumber(contact->text().toStdString());e.setEmail(email->text().toStdString());if(!e.addEmployee(db))QMessageBox::warning(this,"Employee","Unable to save employee. Check required fields.");loadEmployees();}}
void MainWindow::editEmployee(){int id=selectedID(employeeTable);if(!id){QMessageBox::warning(this,"Employee","Please select an employee.");return;}Employee e;if(!Employee::getEmployeeByID(db,id,e))return;bool ok=false;QString first=QInputDialog::getText(this,"Edit Employee","First Name:",QLineEdit::Normal,QString::fromStdString(e.getFirstName()),&ok);if(!ok)return;QString last=QInputDialog::getText(this,"Edit Employee","Last Name:",QLineEdit::Normal,QString::fromStdString(e.getLastName()),&ok);if(!ok)return;QString dept=QInputDialog::getText(this,"Edit Employee","Department:",QLineEdit::Normal,QString::fromStdString(e.getDepartment()),&ok);if(!ok)return;QString contact=QInputDialog::getText(this,"Edit Employee","Contact Number:",QLineEdit::Normal,QString::fromStdString(e.getContactNumber()),&ok);if(!ok)return;QString email=QInputDialog::getText(this,"Edit Employee","Email:",QLineEdit::Normal,QString::fromStdString(e.getEmail()),&ok);if(!ok)return;e.setFirstName(first.toStdString());e.setLastName(last.toStdString());e.setDepartment(dept.toStdString());e.setContactNumber(contact.toStdString());e.setEmail(email.toStdString());if(!e.updateEmployee(db))QMessageBox::warning(this,"Employee","Unable to update employee.");loadEmployees();}
void MainWindow::deactivateEmployee(){int id=selectedID(employeeTable);if(!id){QMessageBox::warning(this,"Employee","Please select an employee.");return;}const char*sql="UPDATE employees SET employment_status='Inactive' WHERE employee_id=?";sqlite3_stmt*s=nullptr;if(sqlite3_prepare_v2(db.getDB(),sql,-1,&s,nullptr)==SQLITE_OK){sqlite3_bind_int(s,1,id);sqlite3_step(s);sqlite3_finalize(s);}loadEmployees();}
void MainWindow::addAttendance(){QDialog d(this);d.setWindowTitle("Attendance / Work Record");auto*f=new QFormLayout(&d);auto*emp=new QComboBox(&d);loadEmployeeChoices(emp);auto*start=dateBox(&d),*end=dateBox(&d);auto*days=new QDoubleSpinBox(&d),*hours=new QDoubleSpinBox(&d),*ot=new QDoubleSpinBox(&d);auto*leave=new QSpinBox(&d),*absence=new QSpinBox(&d);for(auto*x:{days,hours,ot})x->setRange(0,1000);leave->setRange(0,1000);absence->setRange(0,1000);f->addRow("Employee:",emp);f->addRow("Period Start:",start);f->addRow("Period End:",end);f->addRow("Days Worked:",days);f->addRow("Hours Worked:",hours);f->addRow("Overtime Hours:",ot);f->addRow("Leave Days:",leave);f->addRow("Absences:",absence);auto*ok=new QPushButton("Save",&d);f->addRow(ok);connect(ok,&QPushButton::clicked,&d,&QDialog::accept);if(d.exec()==QDialog::Accepted){if(!Attendance::save(db,emp->currentData().toInt(),start->date().toString("yyyy-MM-dd").toStdString()+" to "+end->date().toString("yyyy-MM-dd").toStdString(),days->value(),hours->value(),ot->value(),absence->value(),leave->value()))QMessageBox::warning(this,"Attendance","Unable to save attendance.");loadAttendance();}}
void MainWindow::processPayroll(){if(payrollEmployee->currentData().toInt()==0){QMessageBox::warning(this,"Payroll","Please select an employee.");return;}const bool saved=Payroll::process(db,payrollEmployee->currentData().toInt(),payrollStart->date().toString("yyyy-MM-dd").toStdString(),payrollEnd->date().toString("yyyy-MM-dd").toStdString(),payrollBonus->value(),payrollOther->value(),payrollMultiplier->value(),deductionInput->text().toStdString());if(saved){QMessageBox::information(this,"Payroll","Payroll saved successfully.");payrollSummary->setText("Payroll saved successfully.");}else QMessageBox::warning(this,"Payroll","Payroll already exists or the input is invalid.");}
void MainWindow::markSelectedPaid(){int id=selectedID(historyTable);if(!id){QMessageBox::warning(this,"Payroll","Please select a payroll record.");return;}if(Payroll::markPaid(db,id))loadHistory();else QMessageBox::warning(this,"Payroll","Unable to update payment status.");}
void MainWindow::exportCsv(){QString path=QFileDialog::getSaveFileName(this,"Export CSV","reports/payroll_history.csv","CSV files (*.csv)");if(path.isEmpty())return;if(Reports::exportPayrollCSV(db,path.toStdString()))QMessageBox::information(this,"Reports","CSV exported successfully.");else QMessageBox::critical(this,"Reports","Unable to export CSV.");}
