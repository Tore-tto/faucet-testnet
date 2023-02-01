#pragma once

#include <iostream>
#include <sqlite3.h>
#include <thread>
#include <chrono>
#include <vector>

class database
{
public:
    sqlite3 *db;
    char *zErrMsg = 0;
    static uint32_t rewardtime;
    int last_entry_time = 0;

    void opendatabase();
    void tableCreation();
    void insertData(std::string _address, int _time);
    void updateData(std::string _address, int _time);
    void getvalue(std::string _address);
    void historyinsert(std::string _address,std::string _time);

    void closeDatabase()
    {
        std::cout << "database closed\n";
        sqlite3_close(db);
    };
};