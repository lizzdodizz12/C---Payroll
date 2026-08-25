#ifndef POSITION_H
#define POSITION_H

#include <string>
#include "Database.h"

class Position {
private:
    int positionID;
    std::string positionName;
    std::string payType;        // "Monthly", "Daily", "Hourly"
    double defaultRate;

public:
    // Constructor and Destructor
    Position();
    ~Position();
    
    // Setter methods
    void setPositionID(int id);
    void setPositionName(const std::string& name);
    void setPayType(const std::string& type);
    void setDefaultRate(double rate);
    
    // Getter methods
    int getPositionID() const;
    std::string getPositionName() const;
    std::string getPayType() const;
    double getDefaultRate() const;
    
    // Database operations
    bool addPosition(Database& db);
    bool updatePosition(Database& db);
    bool deletePosition(Database& db);
    static bool getPositionByID(Database& db, int positionID, Position& position);
    static void displayAllPositions(Database& db);
};

#endif // POSITION_H
