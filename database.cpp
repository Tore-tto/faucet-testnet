#include "database.h"
#include <cstring>
#include <chrono>
#include <string>

using namespace std;

uint32_t database::rewardtime = 0;

void database::opendatabase()
{
    int rc = sqlite3_open("../database.db", &db);
    if (rc)
    {
        std::cout << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return;
    }
    else
    {
        std::cout << "Opened database successfully\n";
    }
}

void database::tableCreation()
{
    int rc;
    const char *sql = "CREATE TABLE IF NOT EXISTS data(id INTEGER PRIMARY KEY AUTOINCREMENT,address TEXT UNIQUE,time NUMERIC);";

    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        std::cout << "failed to create table" << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
        return;
    }
    std::cout << "Data table created successfully\n";
    int his;
    const char *sql_his = "CREATE TABLE IF NOT EXISTS history(id INTEGER PRIMARY KEY AUTOINCREMENT,address TEXT,time NUMERIC);";
    his = sqlite3_exec(db, sql_his, 0, 0, &zErrMsg);
    if (his != SQLITE_OK)
    {
        std::cout << "failed to create table" << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
        return;
    }
    std::cout << "History table created successfully\n";
}

void database::insertData(std::string _address, int _time)
{
    std::cout << "insert : "
              << "_address : " << _address << "_time : " << _time << std::endl;

    char const *insertSQL = "INSERT INTO data(address,time) VALUES(?,?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare(db, insertSQL, -1, &stmt, NULL);

    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, _address.c_str(), strlen(_address.c_str()), 0);
        sqlite3_bind_int(stmt, 2, _time);

        int retVal = sqlite3_step(stmt);
        if (retVal != SQLITE_DONE)
        {
            std::cout << "command failed : " << retVal << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}

void database::updateData(std::string _address, int _time)
{
    std::cout << "update : "
              << "_address : " << _address << "_time : " << _time << std::endl;

    char const *updateSQL = "UPDATE data SET time = strftime('%s','now') WHERE address = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare(db, updateSQL, -1, &stmt, NULL);

    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, _address.c_str(), strlen(_address.c_str()), 0);
        int retVal = sqlite3_step(stmt);
        if (retVal != SQLITE_DONE)
        {
            std::cout << "command failed : " << retVal << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}

void database::historyinsert(std::string _address, std::string _time)
{

    char const *insertSQL = "INSERT INTO history(address,time) VALUES(?,?);";
    sqlite3_stmt *stmt;
    int his = sqlite3_prepare(db, insertSQL, -1, &stmt, NULL);

    if (his == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, _address.c_str(), strlen(_address.c_str()), 0);
        sqlite3_bind_text(stmt, 2, _time.c_str(), strlen(_time.c_str()), 0);

        int retVal = sqlite3_step(stmt);
        if (retVal != SQLITE_DONE)
        {
            std::cout << "command failed : " << retVal << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}

void database::getvalue(std::string _address)
{
    std::cout << "inside the select queary\n";
    std::string selectSQL = "SELECT time FROM data WHERE address = ?";
    sqlite3_stmt *resp;
    int check_data = sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &resp, 0);
    if (check_data == SQLITE_OK)
    {
        sqlite3_bind_text(resp, 1, _address.c_str(), -1, 0);
    }
    else
    {
        std::cout << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }

    int step = sqlite3_step(resp);
    last_entry_time = sqlite3_column_int64(resp, 0);
    sqlite3_finalize(resp);
}