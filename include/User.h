#ifndef USER_H
#define USER_H

#include <string>
#include "Database.h"

class User {
private:
    int id;
    std::string username;
    std::string role;

public:
    User();

    int getID() const;
    std::string getUsername() const;
    std::string getRole() const;

    static bool ensureDefaultAdministrator(Database& db);
    static bool login(Database& db, const std::string& username,
                      const std::string& password, User& user);
    static bool createUser(Database& db, const std::string& username,
                           const std::string& password, const std::string& role);
};

#endif // USER_H
