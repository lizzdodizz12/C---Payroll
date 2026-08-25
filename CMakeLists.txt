cmake_minimum_required(VERSION 3.10)
project(PayrollManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SQLITE3_INCLUDE_DIRS})

# Source files
set(SOURCES
    include/MainWindow.h
    src/main.cpp
    src/Database.cpp
    src/Employee.cpp
    src/Position.cpp
    src/User.cpp
    src/Attendance.cpp
    src/Payroll.cpp
    src/Reports.cpp
    src/MainWindow.cpp
)

# Create executable
add_executable(PayrollSystem ${SOURCES})

# Link libraries
target_link_libraries(PayrollSystem PRIVATE SQLite3::SQLite3 Qt${QT_VERSION_MAJOR}::Widgets)

# Create database directory
add_custom_target(create_db_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/database"
)

add_dependencies(PayrollSystem create_db_dir)
