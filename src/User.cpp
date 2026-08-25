#include "User.h"
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
std::string hashPassword(const std::string& password) {
    // Keeps credentials out of the database as plain text for this local prototype.
    const std::size_t value = std::hash<std::string>{}(password);
    std::ostringstream output;
    output << std::hex << value;
    return output.str();
}

bool validRole(const std::string& role) {
    return role == "Administrator" || role == "Payroll Staff";
}
}

User::User() : id(0), username(""), role("") {}

int User::getID() const { return id; }
std::string User::getUsername() const { return username; }
std::string User::getRole() const { return role; }

bool User::ensureDefaultAdministrator(Database& db) {
    if (!db.isConnected()) return false;

    const char* countSql = "SELECT COUNT(*) FROM users";
    sqlite3_stmt* countStatement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), countSql, -1, &countStatement, nullptr) != SQLITE_OK) {
        std::cerr << "ERROR checking users: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }

    int userCount = 0;
    if (sqlite3_step(countStatement) == SQLITE_ROW) {
        userCount = sqlite3_column_int(countStatement, 0);
    }
    sqlite3_finalize(countStatement);

    if (userCount > 0) return true;

    std::cout << "No users found. Creating default administrator account.\n"
              << "Username: admin\nPassword: admin123\n" << std::endl;
    return createUser(db, "admin", "admin123", "Administrator");
}

bool User::login(Database& db, const std::string& loginUsername,
                 const std::string& password, User& user) {
    if (!db.isConnected()) return false;

    const char* sql = "SELECT id, username, role FROM users "
                      "WHERE username = ? AND password_hash = ?";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "ERROR preparing login: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }

    sqlite3_bind_text(statement, 1, loginUsername.c_str(), -1, SQLITE_TRANSIENT);
    const std::string passwordHash = hashPassword(password);
    sqlite3_bind_text(statement, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    const bool authenticated = sqlite3_step(statement) == SQLITE_ROW;
    if (authenticated) {
        user.id = sqlite3_column_int(statement, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
        user.role = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    }
    sqlite3_finalize(statement);
    return authenticated;
}

bool User::createUser(Database& db, const std::string& newUsername,
                      const std::string& password, const std::string& newRole) {
    if (!db.isConnected() || newUsername.empty() || password.empty() || !validRole(newRole)) {
        return false;
    }

    const char* sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db.getDB(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "ERROR preparing user creation: " << sqlite3_errmsg(db.getDB()) << std::endl;
        return false;
    }

    const std::string passwordHash = hashPassword(password);
    sqlite3_bind_text(statement, 1, newUsername.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, newRole.c_str(), -1, SQLITE_TRANSIENT);

    const int result = sqlite3_step(statement);
    if (result != SQLITE_DONE) {
        std::cerr << "ERROR creating user: " << sqlite3_errmsg(db.getDB()) << std::endl;
        sqlite3_finalize(statement);
        return false;
    }
    sqlite3_finalize(statement);
    return true;
}
