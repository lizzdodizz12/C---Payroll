# Payroll Management System - Phase 4

A complete Payroll Management System built in C++ with SQLite database. The current release includes authentication, employee and position management, attendance, payroll calculation, deductions, history, summaries, and CSV export.

## Project Structure

```
PayrollSystem/
├── src/                      # Implementation files
│   ├── main.cpp             # Main application entry point
│   ├── Database.cpp         # Database connection and initialization
│   ├── Employee.cpp         # Employee class implementation
│   ├── Position.cpp         # Position class implementation
│   ├── User.cpp             # Authentication implementation
│   ├── Attendance.cpp       # Attendance persistence
│   ├── Payroll.cpp          # Payroll calculation and history
│   └── Reports.cpp          # Reports and CSV export
├── include/                 # Header files
│   ├── Database.h           # Database class definition
│   ├── Employee.h           # Employee class definition
│   ├── Position.h           # Position class definition
│   ├── User.h               # User and authentication definition
│   ├── Attendance.h         # Attendance definition
│   ├── Payroll.h            # Payroll definition
│   └── Reports.h            # Reports definition
├── database/                # SQLite database storage
│   └── payroll.db          # Main database file (auto-created)
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
└── .gitignore              # Git ignore rules
```

## Technology Stack

- **Language:** C++17
- **Database:** SQLite3
- **Compiler:** MinGW-w64 / GCC
- **Build System:** CMake 3.10+
- **IDE:** Visual Studio Code

## Prerequisites

### Windows (MinGW)

1. **Install MinGW-w64 with GCC:**
   - Download from: https://www.mingw-w64.org/
   - Add MinGW bin directory to PATH (e.g., `C:\mingw64\bin`)

2. **Install CMake:**
   - Download from: https://cmake.org/download/
   - Add CMake bin directory to PATH

3. **Install SQLite3 Development Libraries:**
   - Download from: https://www.sqlite.org/download.html
   - Or use vcpkg: `vcpkg install sqlite3:x64-windows`

4. **Verify Installation:**
   ```powershell
   g++ --version
   cmake --version
   sqlite3 --version
   ```

## Building the Project

### Qt Widgets requirement

The GUI target requires a Qt 6 or Qt 5 development kit built for the same MinGW toolchain as GCC. The current environment has MinGW-w64 and CMake, but does not yet have Qt installed. Install Qt for MinGW, then configure with its CMake prefix if CMake cannot find it:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\mingw_64"
cmake --build build --parallel 4
.\build\PayrollSystem.exe
```

The application opens with the existing local login and database. Use `admin` / `admin123` on a database that still has the default administrator account.

### Step 1: Create Build Directory
```powershell
cd c:\Users\Christian_Inna\Desktop\C++ Payroll
mkdir build
cd build
```

### Step 2: Generate Build Files with CMake
```powershell
cmake -G "MinGW Makefiles" ..
```

**Alternative (for Visual Studio):**
```powershell
cmake -G "Visual Studio 16 2019" ..
```

### Step 3: Compile the Project
```powershell
# Using make
make

# OR using cmake
cmake --build . --config Release
```

### Step 4: Run the Application
```powershell
# Windows
.\PayrollSystem.exe

# Or from the build directory
./PayrollSystem.exe
```

## Complete Setup Instructions for VS Code

### 1. Open Folder in VS Code
- File → Open Folder → Select `C++ Payroll`

### 2. Install VS Code Extensions
- **C/C++ Extension Pack** (by Microsoft)
- **CMake Tools** (by Microsoft)

### 3. Configure VS Code

Create `.vscode/settings.json`:
```json
{
    "cmake.generator": "Unix Makefiles",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "C_Cpp.default.compilerPath": "C:\\mingw64\\bin\\g++.exe",
    "C_Cpp.intelliSenseEngine": "default"
}
```

### 4. Build from VS Code
1. Open Command Palette (Ctrl+Shift+P)
2. Select "CMake: Build"
3. Or use the Build button in the status bar

### 5. Run the Application
1. Use Terminal → Run Task → cmake: build
2. Open integrated terminal and run: `.\build\PayrollSystem.exe`

## Database Schema

### Phase 1 Tables

#### users
```
id (INTEGER PRIMARY KEY)
username (TEXT UNIQUE)
password_hash (TEXT)
role (TEXT: 'Administrator', 'Payroll Staff')
created_at (DATETIME)
```

#### positions
```
position_id (INTEGER PRIMARY KEY)
position_name (TEXT UNIQUE)
pay_type (TEXT: 'Monthly', 'Daily', 'Hourly')
default_rate (REAL)
created_at (DATETIME)
```

#### employees
```
employee_id (INTEGER PRIMARY KEY)
first_name (TEXT)
last_name (TEXT)
position_id (INTEGER FK)
department (TEXT)
employment_type (TEXT: 'Regular', 'Contractual', 'Part-Time', 'Full-Time')
date_hired (DATE)
contact_number (TEXT)
email (TEXT)
employment_status (TEXT: 'Active', 'Inactive')
created_at (DATETIME)
```

#### attendance
```
attendance_id (INTEGER PRIMARY KEY)
employee_id (INTEGER FK)
payroll_period (TEXT)
days_worked (REAL)
hours_worked (REAL)
overtime_hours (REAL)
absences (INTEGER)
leave_days (INTEGER)
created_at (DATETIME)
UNIQUE(employee_id, payroll_period)
```

#### payroll
```
payroll_id (INTEGER PRIMARY KEY)
employee_id (INTEGER FK)
period_start (DATE)
period_end (DATE)
basic_pay (REAL)
overtime_pay (REAL)
bonuses (REAL)
other_earnings (REAL)
gross_pay (REAL)
total_deductions (REAL)
net_pay (REAL)
payment_status (TEXT: 'Paid', 'Unpaid', 'Pending')
date_processed (DATETIME)
created_at (DATETIME)
UNIQUE(employee_id, period_start, period_end)
```

#### payroll_deductions
```
deduction_id (INTEGER PRIMARY KEY)
payroll_id (INTEGER FK)
deduction_name (TEXT)
deduction_amount (REAL)
created_at (DATETIME)
```

## GUI Features

- Qt Widgets `QMainWindow` with classic Windows-style menus, navigation, stacked pages, status bar, tables, forms, and message boxes
- Dashboard, Employees, Positions, Attendance, Process Payroll, Payroll History, and Reports pages
- GUI reuses the existing SQLite database and domain write operations
- Employee and position management, attendance entry, payroll saving, payment status updates, history, and CSV export are connected

## Features Implemented in Phase 1 and Phase 2

### Authentication and Access Control
- User login with up to three attempts
- Administrator and Payroll Staff roles stored in SQLite
- Default administrator created only when the users table is empty
- Default first-run credentials: `admin` / `admin123`
- Administrator-only position and employee write operations
- Payroll Staff can view and search employees

Passwords are stored as a non-plaintext local hash for this prototype. A production deployment should replace this with a password hashing algorithm such as Argon2 or bcrypt.

### Position Management
- ✅ Add new positions with pay type and default rate
- ✅ View all positions in a formatted table
- ✅ Update position details
- ✅ Delete positions (with validation to prevent deletion if assigned to employees)

### Employee Management
- ✅ Add new employees with complete information
- ✅ View all employees
- ✅ Search for specific employees by ID
- ✅ Update employee information
- ✅ Delete employees

### Database Operations
- ✅ SQLite3 connection management
- ✅ Automatic database and table creation on first run
- ✅ Prepared statements to prevent SQL injection
- ✅ Proper foreign key relationships
- ✅ Unique constraints for duplicate prevention
- ✅ Error handling and logging

### Phase 2 Validation
- ✅ Position name and pay type validation
- ✅ Positive salary/rate validation
- ✅ Employee required-field validation
- ✅ Employee position existence validation

### Phase 3 and Phase 4 Payroll
- ✅ Attendance entry and updates by employee and payroll period
- ✅ Monthly, daily, and hourly basic-pay calculations
- ✅ Configurable overtime multiplier
- ✅ Bonuses, other earnings, and individual deductions
- ✅ Transactional payroll saving with duplicate-period protection
- ✅ Payroll history and individual employee history
- ✅ Payroll period summary and total net expenses
- ✅ CSV export, including automatic report-directory creation
- ✅ Mark saved payroll records as Paid

To process payroll, enter attendance first using a period such as `2026-08-01 to 2026-08-15`. Then choose **Process payroll** and use deductions such as `Tax=1000,Loan=500`.

## Usage Example

1. Run the application
2. Main menu will show:
   - Position Management
   - Employee Management
   - Exit

3. Add a position first (required before adding employees)
4. Then add employees and assign them to positions
5. View, update, or delete records as needed

## Error Handling

The application includes:
- Database connection error handling
- Prepared statement error reporting
- Duplicate key constraint validation
- Foreign key constraint validation
- Input validation for menu choices

## Compilation Troubleshooting

### Error: "sqlite3.h not found"
```powershell
# Install SQLite dev package
vcpkg install sqlite3:x64-windows
```

### Error: "g++ not found"
- Ensure MinGW bin directory is in PATH
- Restart VS Code after adding to PATH

### Error: "CMake not found"
- Install CMake from https://cmake.org/
- Add CMake to PATH

### Build fails with undefined references
```powershell
# Clear build directory and rebuild
rm -r build
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
```

## Next Steps - Phase 2

Coming soon:
- User login system
- Position and salary management enhancements
- Attendance tracking
- Payroll calculations
- Reports generation

## Code Quality

- **Object-Oriented Design:** Proper use of classes and encapsulation
- **Memory Management:** Safe resource handling
- **Input Validation:** User input checks
- **Error Handling:** Comprehensive error reporting
- **Code Organization:** Separated concerns (headers and implementations)
- **Comments:** Clear documentation of important logic

## License

This project is created for educational purposes.

## Support

For issues or questions about:
- Building: Check CMakeLists.txt configuration
- Database: Review Database.cpp and schema
- Employee/Position management: See Employee.cpp and Position.cpp
