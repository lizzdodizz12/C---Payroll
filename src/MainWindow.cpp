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
#include <QAction>
#include <QProgressBar>
#include <QToolBar>
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
    box->setButtonSymbols(QAbstractSpinBox::NoButtons);
    return box;
}

QDateEdit* dateBox(QWidget* parent) {
    auto* box = new QDateEdit(QDate::currentDate(), parent);
    box->setCalendarPopup(true);
    box->setDisplayFormat("yyyy-MM-dd");
    return box;
}

QPushButton* createActionButton(QWidget* parent, const QString& text, const QString& accent = "#2563eb") {
    auto* button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; border-radius: 10px; padding: 8px 16px; font-weight: 700; } "
        "QPushButton:hover { background: #1d4ed8; } "
        "QPushButton:pressed { background: #1e40af; }"
    ).arg(accent));
    return button;
}

QFrame* createMetricCard(const QString& title, const QString& value, const QString& accent) {
    auto* card = new QFrame;
    card->setObjectName("metricCard");
    card->setStyleSheet(QString(
        "QFrame#metricCard { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border: none; border-radius: 16px; }"
    ).arg(accent).arg(accent.mid(0, 7) == "#" ? accent : "#2563eb"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-size: 12px; font-weight: 600; letter-spacing: 0.5px;");
    auto* valueLabel = new QLabel(value, card);
    valueLabel->setStyleSheet("color: white; font-size: 28px; font-weight: 700;");
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    return card;
}
}

MainWindow::MainWindow(Database& database, const User& currentUser, QWidget* parent)
    : QMainWindow(parent), db(database), user(currentUser), pages(new QStackedWidget(this)),
      employeeTable(nullptr), positionTable(nullptr), attendanceTable(nullptr), historyTable(nullptr),
      reportTable(nullptr), dashboardStats(nullptr), employeeSearch(nullptr), payrollEmployee(nullptr),
      payrollStart(nullptr), payrollEnd(nullptr), payrollBonus(nullptr), payrollOther(nullptr),
      payrollMultiplier(nullptr), deductionInput(nullptr), payrollSummary(nullptr), reportType(nullptr),
      reportStart(nullptr), reportEnd(nullptr), reportEmployee(nullptr), themeAction(nullptr), darkMode(false) {
    setWindowTitle("Payroll Management System");
    resize(1200, 760);
    setMinimumSize(980, 650);

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
    auto* viewMenu = menuBar()->addMenu("&View");
    themeAction = new QAction("Dark Mode", this);
    themeAction->setCheckable(true);
    connect(themeAction, &QAction::toggled, this, &MainWindow::toggleTheme);
    viewMenu->addAction(themeAction);
    menuBar()->addMenu("&Help")->addAction("About", this, [this] {
        QMessageBox::information(this, "About", "Payroll Management System\nQt Widgets and SQLite");
    });

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(16, 12, 16, 16);
    mainLayout->setSpacing(16);

    auto* navigation = new QListWidget(central);
    navigation->addItems({"Dashboard", "Employees", "Positions", "Attendance", "Process Payroll", "Payroll History", "Reports", "Exit"});
    navigation->setFixedWidth(220);
    navigation->setCurrentRow(0);
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

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
    applyTheme();
    showDashboard();
}

void MainWindow::applyTheme() {
    const QString lightStyle = R"(
        QMainWindow { background: #edf3fb; color: #0f172a; }
        QWidget { color: #0f172a; }
        QMenuBar { background: #0f172a; color: #f8fafc; border: none; }
        QMenuBar::item { padding: 8px 12px; border-radius: 6px; }
        QMenuBar::item:selected { background: #1d4ed8; }
        QMenu { background: #ffffff; border: 1px solid #dfe7f5; color: #0f172a; }
        QMenu::item:selected { background: #dbeafe; color: #0f172a; }
        QStatusBar { background: #e2e8f0; color: #0f172a; }
        QPushButton { background: #2563eb; color: white; border: none; border-radius: 10px; padding: 8px 16px; font-weight: 700; }
        QPushButton:hover { background: #1d4ed8; }
        QPushButton:pressed { background: #1e40af; }
        QPushButton:disabled { background: #cbd5e1; color: #64748b; }
        QGroupBox { font-weight: 700; border: 1px solid #dfe7f5; border-radius: 14px; margin-top: 14px; background: #ffffff; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #1e293b; }
        QTableWidget { background: #ffffff; border: 1px solid #dfe7f5; border-radius: 12px; gridline-color: #e2e8f0; selection-background-color: #dbeafe; selection-color: #111827; }
        QHeaderView::section { background: #eef2ff; color: #1e293b; padding: 8px; border: 1px solid #dfe7f5; font-weight: 700; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox, QSpinBox, QTextEdit { background: #f8fafc; border: 1px solid #cbd5e1; border-radius: 10px; padding: 8px 10px; color: #0f172a; }
        QComboBox::drop-down { border: none; }
        QLabel { color: #0f172a; }
        QListWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0f172a, stop:1 #1e293b); border: none; border-radius: 16px; color: white; }
        QListWidget::item { padding: 12px 14px; margin: 4px 8px; border-radius: 10px; }
        QListWidget::item:selected { background: #2563eb; color: white; }
        QFrame#metricCard { border: none; }
        QProgressBar { border: 1px solid #cbd5e1; border-radius: 8px; text-align: center; background: #e2e8f0; }
        QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #10b981); border-radius: 7px; }
    )";

    const QString darkStyle = R"(
        QMainWindow { background: #020817; color: #e2e8f0; }
        QWidget { color: #e2e8f0; }
        QMenuBar { background: #0f172a; color: #f8fafc; border: none; }
        QMenuBar::item { padding: 8px 12px; border-radius: 6px; }
        QMenuBar::item:selected { background: #2563eb; }
        QMenu { background: #0f172a; border: 1px solid #1e293b; color: #e2e8f0; }
        QMenu::item:selected { background: #1d4ed8; color: #ffffff; }
        QStatusBar { background: #0f172a; color: #e2e8f0; }
        QPushButton { background: #2563eb; color: white; border: none; border-radius: 10px; padding: 8px 16px; font-weight: 700; }
        QPushButton:hover { background: #1d4ed8; }
        QPushButton:pressed { background: #1e40af; }
        QPushButton:disabled { background: #374151; color: #9ca3af; }
        QGroupBox { font-weight: 700; border: 1px solid #334155; border-radius: 14px; margin-top: 14px; background: #111827; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #e2e8f0; }
        QTableWidget { background: #0f172a; border: 1px solid #334155; border-radius: 12px; gridline-color: #334155; selection-background-color: #1d4ed8; selection-color: #ffffff; }
        QHeaderView::section { background: #1e293b; color: #e2e8f0; padding: 8px; border: 1px solid #334155; font-weight: 700; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox, QSpinBox, QTextEdit { background: #0f172a; border: 1px solid #475569; border-radius: 10px; padding: 8px 10px; color: #e2e8f0; }
        QComboBox::drop-down { border: none; }
        QLabel { color: #e2e8f0; }
        QListWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #111827, stop:1 #0f172a); border: none; border-radius: 16px; color: white; }
        QListWidget::item { padding: 12px 14px; margin: 4px 8px; border-radius: 10px; }
        QListWidget::item:selected { background: #2563eb; color: white; }
        QFrame#metricCard { border: none; }
        QProgressBar { border: 1px solid #475569; border-radius: 8px; text-align: center; background: #1e293b; }
        QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60a5fa, stop:1 #34d399); border-radius: 7px; }
    )";

    setStyleSheet(darkMode ? darkStyle : lightStyle);
    if (themeAction) themeAction->setChecked(darkMode);
}

void MainWindow::toggleTheme() {
    darkMode = !darkMode;
    applyTheme();
}

bool MainWindow::isAdministrator() const { return user.getRole() == "Administrator"; }
void MainWindow::configureTable(QTableWidget* table, const QStringList& headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet("QTableWidget { selection-background-color: #dbeafe; } ");
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
}
int MainWindow::selectedID(QTableWidget* table) const {
    auto* item = table->currentItem(); return item ? item->data(Qt::UserRole).toInt() : 0;
}
QWidget* MainWindow::createDashboardPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(18);

    auto* header = new QWidget(page);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto* title = new QLabel("Payroll Overview");
    title->setStyleSheet("font-size: 26px; font-weight: 800; color: #0f172a;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    auto* userTag = new QLabel(QString("%1 · %2").arg(QString::fromStdString(user.getUsername()), QString::fromStdString(user.getRole())));
    userTag->setStyleSheet("background: #dbeafe; color: #1d4ed8; border-radius: 8px; padding: 6px 12px; font-weight: 700;");
    headerLayout->addWidget(userTag);
    layout->addWidget(header);

    dashboardStats = new QLabel(page);
    dashboardStats->setWordWrap(true);
    dashboardStats->setMinimumHeight(120);
    dashboardStats->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1d4ed8, stop:1 #0f172a); "
        "color: white; border-radius: 18px; padding: 18px; font-size: 16px; font-weight: 600;"
    );
    layout->addWidget(dashboardStats);

    auto* cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(createMetricCard("Employees", "0", "#2563eb"));
    cardsLayout->addWidget(createMetricCard("Active", "0", "#10b981"));
    cardsLayout->addWidget(createMetricCard("Positions", "0", "#f59e0b"));
    cardsLayout->addWidget(createMetricCard("Records", "0", "#8b5cf6"));
    layout->addLayout(cardsLayout);

    auto* insightLayout = new QHBoxLayout;
    insightLayout->setSpacing(16);

    auto* payrollMix = new QGroupBox("Payroll mix");
    auto* payrollMixLayout = new QVBoxLayout(payrollMix);
    auto* basePay = new QProgressBar(payrollMix);
    basePay->setValue(72); basePay->setFormat("Base pay 72%");
    auto* addOns = new QProgressBar(payrollMix); addOns->setValue(48); addOns->setFormat("Allowances 48%");
    auto* taxes = new QProgressBar(payrollMix); taxes->setValue(36); taxes->setFormat("Deductions 36%");
    payrollMixLayout->addWidget(basePay); payrollMixLayout->addWidget(addOns); payrollMixLayout->addWidget(taxes);
    insightLayout->addWidget(payrollMix, 1);

    auto* attendanceHealth = new QGroupBox("Attendance health");
    auto* attendanceLayout = new QVBoxLayout(attendanceHealth);
    auto* present = new QProgressBar(attendanceHealth); present->setValue(88); present->setFormat("Present 88%");
    auto* leave = new QProgressBar(attendanceHealth); leave->setValue(12); leave->setFormat("Leave 12%");
    auto* overtime = new QProgressBar(attendanceHealth); overtime->setValue(31); overtime->setFormat("OT 31%");
    attendanceLayout->addWidget(present); attendanceLayout->addWidget(leave); attendanceLayout->addWidget(overtime);
    insightLayout->addWidget(attendanceHealth, 1);
    layout->addLayout(insightLayout);

    auto* group = new QGroupBox("Recent Payroll Records");
    auto* groupLayout = new QVBoxLayout(group);
    auto* recent = new QTableWidget;
    configureTable(recent, {"Employee", "Period", "Net Pay", "Status"});
    groupLayout->addWidget(recent);
    layout->addWidget(group, 1);

    fillTable(recent, db.getDB(), "SELECT e.first_name || ' ' || e.last_name,p.period_start || ' to ' || p.period_end,p.net_pay,p.payment_status FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.date_processed DESC LIMIT 10", [recent](sqlite3_stmt* s) {
        int r = recent->rowCount();
        recent->insertRow(r);
        for (int c = 0; c < 4; ++c) {
            auto* item = new QTableWidgetItem(c == 2 ? QString::number(sqlite3_column_double(s, c), 'f', 2) : text(s, c));
            recent->setItem(r, c, item);
        }
    });
    return page;
}

void MainWindow::updateDashboard() {
    const char* sql = "SELECT (SELECT COUNT(*) FROM employees),(SELECT COUNT(*) FROM employees WHERE employment_status='Active'),(SELECT COUNT(*) FROM positions),(SELECT COUNT(*) FROM payroll)";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &s, nullptr) == SQLITE_OK && sqlite3_step(s) == SQLITE_ROW) {
        const int totalEmployees = sqlite3_column_int(s, 0);
        const int activeEmployees = sqlite3_column_int(s, 1);
        const int totalPositions = sqlite3_column_int(s, 2);
        const int payrollRecords = sqlite3_column_int(s, 3);

        dashboardStats->setText(QString(
            "<html><body><div style='font-size: 16px; font-weight: 700; margin-bottom: 10px;'>Operations overview</div>"
            "<div>Total employees: <b>%1</b> &nbsp; | &nbsp; Active employees: <b>%2</b></div>"
            "<div style='margin-top: 8px;'>Open positions: <b>%3</b> &nbsp; | &nbsp; Payroll entries: <b>%4</b></div>"
            "</body></html>"
        ).arg(totalEmployees).arg(activeEmployees).arg(totalPositions).arg(payrollRecords));

        const QList<QFrame*> cards = dashboardStats->parentWidget()->findChildren<QFrame*>("metricCard");
        if (cards.size() >= 4) {
            cards.at(0)->findChildren<QLabel*>().at(1)->setText(QString::number(totalEmployees));
            cards.at(1)->findChildren<QLabel*>().at(1)->setText(QString::number(activeEmployees));
            cards.at(2)->findChildren<QLabel*>().at(1)->setText(QString::number(totalPositions));
            cards.at(3)->findChildren<QLabel*>().at(1)->setText(QString::number(payrollRecords));
        }
    }
    if (s) sqlite3_finalize(s);
}

QWidget* MainWindow::createEmployeesPage() {
    auto* page = new QWidget;
    auto* l = new QVBoxLayout(page);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(16);

    auto* header = new QWidget(page);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto* title = new QLabel("Employee Directory");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    l->addWidget(header);

    auto* bar = new QHBoxLayout;
    employeeSearch = new QLineEdit;
    employeeSearch->setPlaceholderText("Search by name or department");
    auto* search = createActionButton(page, "Search", "#1d4ed8");
    auto* add = createActionButton(page, "Add Employee", "#16a34a");
    auto* edit = createActionButton(page, "Edit Employee", "#f59e0b");
    auto* deactivate = createActionButton(page, "Deactivate", "#ef4444");
    auto* refresh = createActionButton(page, "Refresh", "#0f172a");
    bar->addWidget(employeeSearch);
    bar->addWidget(search);
    bar->addWidget(add);
    bar->addWidget(edit);
    bar->addWidget(deactivate);
    bar->addWidget(refresh);
    l->addLayout(bar);

    employeeTable = new QTableWidget;
    configureTable(employeeTable, {"ID", "First Name", "Last Name", "Position", "Department", "Employment Type", "Contact", "Status"});
    l->addWidget(employeeTable, 1);

    connect(search, &QPushButton::clicked, this, &MainWindow::loadEmployees);
    connect(refresh, &QPushButton::clicked, this, &MainWindow::loadEmployees);
    connect(add, &QPushButton::clicked, this, &MainWindow::addEmployee);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editEmployee);
    connect(deactivate, &QPushButton::clicked, this, &MainWindow::deactivateEmployee);
    add->setEnabled(isAdministrator());
    edit->setEnabled(isAdministrator());
    deactivate->setEnabled(isAdministrator());
    return page;
}

void MainWindow::loadEmployees() {
    QString filter = employeeSearch ? employeeSearch->text() : QString();
    const char* sql = "SELECT e.employee_id,e.first_name,e.last_name,p.position_name,e.department,e.employment_type,e.contact_number,e.employment_status FROM employees e JOIN positions p ON p.position_id=e.position_id WHERE (?='' OR e.first_name||' '||e.last_name LIKE '%'||?||'%' OR e.department LIKE '%'||?||'%') ORDER BY e.employee_id";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &s, nullptr) != SQLITE_OK) return;
    for (int i = 1; i <= 3; ++i) sqlite3_bind_text(s, i, filter.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    employeeTable->setRowCount(0);
    while (sqlite3_step(s) == SQLITE_ROW) {
        const int r = employeeTable->rowCount();
        employeeTable->insertRow(r);
        for (int c = 0; c < 8; ++c) {
            auto* item = new QTableWidgetItem(text(s, c));
            if (c == 0) item->setData(Qt::UserRole, sqlite3_column_int(s, 0));
            employeeTable->setItem(r, c, item);
        }
    }
    sqlite3_finalize(s);
}

void MainWindow::loadEmployeeChoices(QComboBox* combo, bool all) {
    combo->clear();
    if (all) combo->addItem("All Employees", 0);
    const char* sql = "SELECT employee_id,first_name||' '||last_name FROM employees WHERE employment_status='Active' ORDER BY last_name";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &s, nullptr) != SQLITE_OK) return;
    while (sqlite3_step(s) == SQLITE_ROW) combo->addItem(text(s, 1), sqlite3_column_int(s, 0));
    sqlite3_finalize(s);
}

QWidget* MainWindow::createPositionsPage() {
    auto* p = new QWidget;
    auto* l = new QVBoxLayout(p);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(16);

    auto* header = new QWidget;
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel("Position Catalog");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    l->addWidget(header);

    auto* bar = new QHBoxLayout;
    auto* a = createActionButton(p, "Add Position", "#16a34a");
    auto* e = createActionButton(p, "Edit Position", "#f59e0b");
    auto* d = createActionButton(p, "Delete Position", "#ef4444");
    auto* r = createActionButton(p, "Refresh", "#0f172a");
    bar->addWidget(a); bar->addWidget(e); bar->addWidget(d); bar->addWidget(r); bar->addStretch();
    l->addLayout(bar);

    positionTable = new QTableWidget;
    configureTable(positionTable, {"ID", "Position Name", "Pay Type", "Default Rate"});
    l->addWidget(positionTable, 1);
    connect(a, &QPushButton::clicked, this, &MainWindow::addPosition);
    connect(e, &QPushButton::clicked, this, &MainWindow::editPosition);
    connect(d, &QPushButton::clicked, this, &MainWindow::deletePosition);
    connect(r, &QPushButton::clicked, this, &MainWindow::loadPositions);
    a->setEnabled(isAdministrator());
    e->setEnabled(isAdministrator());
    d->setEnabled(isAdministrator());
    return p;
}

void MainWindow::loadPositions() {
    fillTable(positionTable, db.getDB(), "SELECT position_id,position_name,pay_type,default_rate FROM positions ORDER BY position_id", [this](sqlite3_stmt* s) {
        int r = positionTable->rowCount();
        positionTable->insertRow(r);
        for (int c = 0; c < 4; ++c) {
            auto* item = new QTableWidgetItem(c == 3 ? QString::number(sqlite3_column_double(s, c), 'f', 2) : text(s, c));
            if (c == 0) item->setData(Qt::UserRole, sqlite3_column_int(s, 0));
            positionTable->setItem(r, c, item);
        }
    });
}

QWidget* MainWindow::createAttendancePage() {
    auto* p = new QWidget;
    auto* l = new QVBoxLayout(p);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(16);

    auto* header = new QWidget;
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel("Attendance Tracker");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    l->addWidget(header);

    auto* bar = new QHBoxLayout;
    auto* a = createActionButton(p, "Add / Update Record", "#16a34a");
    auto* r = createActionButton(p, "Refresh", "#0f172a");
    bar->addWidget(a); bar->addWidget(r); bar->addStretch();
    l->addLayout(bar);

    attendanceTable = new QTableWidget;
    configureTable(attendanceTable, {"Employee ID", "Employee", "Period", "Days", "Hours", "OT Hours", "Leave", "Absences"});
    l->addWidget(attendanceTable, 1);
    connect(a, &QPushButton::clicked, this, &MainWindow::addAttendance);
    connect(r, &QPushButton::clicked, this, &MainWindow::loadAttendance);
    return p;
}

void MainWindow::loadAttendance() {
    fillTable(attendanceTable, db.getDB(), "SELECT a.employee_id,e.first_name||' '||e.last_name,a.payroll_period,a.days_worked,a.hours_worked,a.overtime_hours,a.leave_days,a.absences FROM attendance a JOIN employees e ON e.employee_id=a.employee_id ORDER BY a.payroll_period DESC", [this](sqlite3_stmt* s) {
        int r = attendanceTable->rowCount();
        attendanceTable->insertRow(r);
        for (int c = 0; c < 8; ++c) {
            auto* item = new QTableWidgetItem((c >= 3 && c <= 5) ? QString::number(sqlite3_column_double(s, c), 'f', 2) : text(s, c));
            if (c == 0) item->setData(Qt::UserRole, sqlite3_column_int(s, 0));
            attendanceTable->setItem(r, c, item);
        }
    });
}

QWidget* MainWindow::createPayrollPage() {
    auto* p = new QWidget;
    auto* l = new QVBoxLayout(p);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(18);

    auto* title = new QLabel("Payroll Processing");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    l->addWidget(title);

    auto* g = new QGroupBox("Payroll Details");
    auto* f = new QFormLayout(g);
    payrollEmployee = new QComboBox;
    payrollStart = dateBox(g);
    payrollEnd = dateBox(g);
    payrollBonus = moneyBox(g);
    payrollOther = moneyBox(g);
    payrollMultiplier = new QDoubleSpinBox(g);
    payrollMultiplier->setRange(0.01, 10);
    payrollMultiplier->setValue(1.25);
    payrollMultiplier->setButtonSymbols(QAbstractSpinBox::NoButtons);
    deductionInput = new QLineEdit(g);
    deductionInput->setPlaceholderText("Tax=1000,Loan=500");
    f->addRow("Employee:", payrollEmployee);
    f->addRow("Period start:", payrollStart);
    f->addRow("Period end:", payrollEnd);
    f->addRow("Bonus:", payrollBonus);
    f->addRow("Other earnings:", payrollOther);
    f->addRow("OT multiplier:", payrollMultiplier);
    f->addRow("Deductions:", deductionInput);
    l->addWidget(g);

    payrollSummary = new QLabel("Enter values and save payroll.");
    payrollSummary->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    payrollSummary->setStyleSheet("background: #eff6ff; border: 1px solid #dbeafe; border-radius: 10px; padding: 10px; color: #1e3a8a;");
    l->addWidget(payrollSummary);

    auto* save = createActionButton(p, "Calculate and Save Payroll", "#10b981");
    l->addWidget(save);
    l->addStretch();
    connect(save, &QPushButton::clicked, this, &MainWindow::processPayroll);
    return p;
}

QWidget* MainWindow::createHistoryPage() {
    auto* p = new QWidget;
    auto* l = new QVBoxLayout(p);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(16);

    auto* header = new QWidget;
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel("Payroll History");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    l->addWidget(header);

    auto* b = new QHBoxLayout;
    auto* r = createActionButton(p, "Refresh", "#0f172a");
    auto* paid = createActionButton(p, "Mark as Paid", "#8b5cf6");
    b->addWidget(r); b->addWidget(paid); b->addStretch();
    l->addLayout(b);

    historyTable = new QTableWidget;
    configureTable(historyTable, {"ID", "Employee", "Start", "End", "Gross", "Deductions", "Net Pay", "Status"});
    l->addWidget(historyTable, 1);
    connect(r, &QPushButton::clicked, this, &MainWindow::loadHistory);
    connect(paid, &QPushButton::clicked, this, &MainWindow::markSelectedPaid);
    return p;
}

void MainWindow::loadHistory() {
    fillTable(historyTable, db.getDB(), "SELECT p.payroll_id,e.first_name||' '||e.last_name,p.period_start,p.period_end,p.gross_pay,p.total_deductions,p.net_pay,p.payment_status FROM payroll p JOIN employees e ON e.employee_id=p.employee_id ORDER BY p.period_start DESC", [this](sqlite3_stmt* s) {
        int r = historyTable->rowCount();
        historyTable->insertRow(r);
        for (int c = 0; c < 8; ++c) {
            auto* item = new QTableWidgetItem((c >= 4 && c <= 6) ? QString::number(sqlite3_column_double(s, c), 'f', 2) : text(s, c));
            if (c == 0) item->setData(Qt::UserRole, sqlite3_column_int(s, 0));
            historyTable->setItem(r, c, item);
        }
    });
}

QWidget* MainWindow::createReportsPage() {
    auto* p = new QWidget;
    auto* l = new QVBoxLayout(p);
    l->setContentsMargins(18, 18, 18, 18);
    l->setSpacing(18);

    auto* header = new QWidget;
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel("Reports & Exports");
    title->setStyleSheet("font-size: 24px; font-weight: 800;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    l->addWidget(header);

    auto* b = new QHBoxLayout;
    reportType = new QComboBox;
    reportType->addItems({"Payroll Summary", "Payroll History CSV", "Employee List CSV"});
    reportStart = dateBox(p);
    reportEnd = dateBox(p);
    auto* g = createActionButton(p, "Generate / Export", "#2563eb");
    b->addWidget(reportType); b->addWidget(reportStart); b->addWidget(reportEnd); b->addWidget(g); b->addStretch();
    l->addLayout(b);

    reportTable = new QTableWidget;
    configureTable(reportTable, {"Metric", "Value"});
    l->addWidget(reportTable, 1);
    connect(g, &QPushButton::clicked, this, &MainWindow::exportCsv);
    return p;
}

void MainWindow::showDashboard() { pages->setCurrentIndex(0); updateDashboard(); }
void MainWindow::showEmployees() { pages->setCurrentIndex(1); loadEmployees(); }
void MainWindow::showPositions() { pages->setCurrentIndex(2); loadPositions(); }
void MainWindow::showAttendance() { pages->setCurrentIndex(3); loadAttendance(); }
void MainWindow::showPayroll() { pages->setCurrentIndex(4); loadEmployeeChoices(payrollEmployee); }
void MainWindow::showHistory() { pages->setCurrentIndex(5); loadHistory(); }
void MainWindow::showReports() { pages->setCurrentIndex(6); }
void MainWindow::refreshCurrentPage() {
    switch (pages->currentIndex()) {
        case 0: updateDashboard(); break;
        case 1: loadEmployees(); break;
        case 2: loadPositions(); break;
        case 3: loadAttendance(); break;
        case 4: loadEmployeeChoices(payrollEmployee); break;
        case 5: loadHistory(); break;
        default: break;
    }
}

void MainWindow::addPosition() {
    QDialog d(this);
    d.setWindowTitle("Add Position");
    auto* f = new QFormLayout(&d);
    auto* n = new QLineEdit(&d);
    auto* t = new QComboBox(&d);
    t->addItems({"Monthly", "Daily", "Hourly"});
    auto* rate = moneyBox(&d);
    f->addRow("Position Name:", n);
    f->addRow("Pay Type:", t);
    f->addRow("Default Rate:", rate);
    auto* ok = new QPushButton("Save", &d);
    f->addRow(ok);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    if (d.exec() == QDialog::Accepted) {
        Position p;
        p.setPositionName(n->text().toStdString());
        p.setPayType(t->currentText().toStdString());
        p.setDefaultRate(rate->value());
        if (!p.addPosition(db)) QMessageBox::warning(this, "Position", "Unable to save position.");
        loadPositions();
    }
}

void MainWindow::editPosition() {
    int id = selectedID(positionTable);
    if (!id) { QMessageBox::warning(this, "Position", "Please select a position."); return; }
    Position p;
    if (!Position::getPositionByID(db, id, p)) return;
    bool ok = false;
    QString name = QInputDialog::getText(this, "Edit Position", "Position Name:", QLineEdit::Normal, QString::fromStdString(p.getPositionName()), &ok);
    if (!ok) return;
    QStringList types = {"Monthly", "Daily", "Hourly"};
    QString type = QInputDialog::getItem(this, "Edit Position", "Pay Type:", types, types.indexOf(QString::fromStdString(p.getPayType())), false, &ok);
    if (!ok) return;
    double rate = QInputDialog::getDouble(this, "Edit Position", "Default Rate:", p.getDefaultRate(), 0, 1000000000, 2, &ok);
    if (ok) {
        p.setPositionName(name.toStdString());
        p.setPayType(type.toStdString());
        p.setDefaultRate(rate);
        if (!p.updatePosition(db)) QMessageBox::warning(this, "Position", "Unable to update position.");
        loadPositions();
    }
}

void MainWindow::deletePosition() {
    int id = selectedID(positionTable);
    if (!id) { QMessageBox::warning(this, "Position", "Please select a position."); return; }
    Position p; p.setPositionID(id);
    if (!p.deletePosition(db)) QMessageBox::warning(this, "Position", "Position may be assigned to employees.");
    loadPositions();
}

void MainWindow::addEmployee() {
    QDialog d(this);
    d.setWindowTitle("Add Employee");
    auto* f = new QFormLayout(&d);
    auto* first = new QLineEdit(&d), *last = new QLineEdit(&d), *dept = new QLineEdit(&d), *contact = new QLineEdit(&d), *email = new QLineEdit(&d);
    auto* pos = new QComboBox(&d);
    auto* type = new QComboBox(&d);
    type->addItems({"Regular", "Contractual", "Part-Time", "Full-Time"});
    auto* hire = dateBox(&d);
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), "SELECT position_id,position_name FROM positions ORDER BY position_name", -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW) pos->addItem(text(s, 1), sqlite3_column_int(s, 0));
        sqlite3_finalize(s);
    }
    f->addRow("First Name:", first);
    f->addRow("Last Name:", last);
    f->addRow("Position:", pos);
    f->addRow("Department:", dept);
    f->addRow("Employment Type:", type);
    f->addRow("Date Hired:", hire);
    f->addRow("Contact Number:", contact);
    f->addRow("Email:", email);
    auto* ok = new QPushButton("Save", &d);
    f->addRow(ok);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    if (d.exec() == QDialog::Accepted) {
        Employee e;
        e.setFirstName(first->text().toStdString());
        e.setLastName(last->text().toStdString());
        e.setPositionID(pos->currentData().toInt());
        e.setDepartment(dept->text().toStdString());
        e.setEmploymentType(type->currentText().toStdString());
        e.setDateHired(hire->date().toString("yyyy-MM-dd").toStdString());
        e.setContactNumber(contact->text().toStdString());
        e.setEmail(email->text().toStdString());
        if (!e.addEmployee(db)) QMessageBox::warning(this, "Employee", "Unable to save employee. Check required fields.");
        loadEmployees();
    }
}

void MainWindow::editEmployee() {
    int id = selectedID(employeeTable);
    if (!id) { QMessageBox::warning(this, "Employee", "Please select an employee."); return; }
    Employee e;
    if (!Employee::getEmployeeByID(db, id, e)) return;
    bool ok = false;
    QString first = QInputDialog::getText(this, "Edit Employee", "First Name:", QLineEdit::Normal, QString::fromStdString(e.getFirstName()), &ok);
    if (!ok) return;
    QString last = QInputDialog::getText(this, "Edit Employee", "Last Name:", QLineEdit::Normal, QString::fromStdString(e.getLastName()), &ok);
    if (!ok) return;
    QString dept = QInputDialog::getText(this, "Edit Employee", "Department:", QLineEdit::Normal, QString::fromStdString(e.getDepartment()), &ok);
    if (!ok) return;
    QString contact = QInputDialog::getText(this, "Edit Employee", "Contact Number:", QLineEdit::Normal, QString::fromStdString(e.getContactNumber()), &ok);
    if (!ok) return;
    QString email = QInputDialog::getText(this, "Edit Employee", "Email:", QLineEdit::Normal, QString::fromStdString(e.getEmail()), &ok);
    if (!ok) return;
    e.setFirstName(first.toStdString());
    e.setLastName(last.toStdString());
    e.setDepartment(dept.toStdString());
    e.setContactNumber(contact.toStdString());
    e.setEmail(email.toStdString());
    if (!e.updateEmployee(db)) QMessageBox::warning(this, "Employee", "Unable to update employee.");
    loadEmployees();
}

void MainWindow::deactivateEmployee() {
    int id = selectedID(employeeTable);
    if (!id) { QMessageBox::warning(this, "Employee", "Please select an employee."); return; }
    const char* sql = "UPDATE employees SET employment_status='Inactive' WHERE employee_id=?";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, id);
        sqlite3_step(s);
        sqlite3_finalize(s);
    }
    loadEmployees();
}

void MainWindow::addAttendance() {
    QDialog d(this);
    d.setWindowTitle("Attendance / Work Record");
    auto* f = new QFormLayout(&d);
    auto* emp = new QComboBox(&d);
    loadEmployeeChoices(emp);
    auto* start = dateBox(&d), *end = dateBox(&d);
    auto* days = new QDoubleSpinBox(&d), *hours = new QDoubleSpinBox(&d), *ot = new QDoubleSpinBox(&d);
    auto* leave = new QSpinBox(&d), *absence = new QSpinBox(&d);
    for (auto* x : {days, hours, ot}) x->setRange(0, 1000);
    leave->setRange(0, 1000); absence->setRange(0, 1000);
    f->addRow("Employee:", emp);
    f->addRow("Period Start:", start);
    f->addRow("Period End:", end);
    f->addRow("Days Worked:", days);
    f->addRow("Hours Worked:", hours);
    f->addRow("Overtime Hours:", ot);
    f->addRow("Leave Days:", leave);
    f->addRow("Absences:", absence);
    auto* ok = new QPushButton("Save", &d);
    f->addRow(ok);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    if (d.exec() == QDialog::Accepted) {
        if (!Attendance::save(db, emp->currentData().toInt(), start->date().toString("yyyy-MM-dd").toStdString() + " to " + end->date().toString("yyyy-MM-dd").toStdString(), days->value(), hours->value(), ot->value(), absence->value(), leave->value())) QMessageBox::warning(this, "Attendance", "Unable to save attendance.");
        loadAttendance();
    }
}

void MainWindow::processPayroll() {
    if (payrollEmployee->currentData().toInt() == 0) {
        QMessageBox::warning(this, "Payroll", "Please select an employee.");
        return;
    }
    const bool saved = Payroll::process(db, payrollEmployee->currentData().toInt(), payrollStart->date().toString("yyyy-MM-dd").toStdString(), payrollEnd->date().toString("yyyy-MM-dd").toStdString(), payrollBonus->value(), payrollOther->value(), payrollMultiplier->value(), deductionInput->text().toStdString());
    if (saved) {
        QMessageBox::information(this, "Payroll", "Payroll saved successfully.");
        payrollSummary->setText("Payroll saved successfully.");
    } else QMessageBox::warning(this, "Payroll", "Payroll already exists or the input is invalid.");
}

void MainWindow::markSelectedPaid() {
    int id = selectedID(historyTable);
    if (!id) { QMessageBox::warning(this, "Payroll", "Please select a payroll record."); return; }
    if (Payroll::markPaid(db, id)) loadHistory();
    else QMessageBox::warning(this, "Payroll", "Unable to update payment status.");
}

void MainWindow::exportCsv() {
    QString path = QFileDialog::getSaveFileName(this, "Export CSV", "reports/payroll_history.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;
    if (Reports::exportPayrollCSV(db, path.toStdString())) QMessageBox::information(this, "Reports", "CSV exported successfully.");
    else QMessageBox::critical(this, "Reports", "Unable to export CSV.");
}
