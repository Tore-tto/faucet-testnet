#include <iostream>
#include <chrono>
#include <lmdb.h>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
    bool csv_log = false;
    vector<string> cmdLineArgs(argv, argv + argc);
    for (auto &arg : cmdLineArgs)
    {
        if (arg == "--csv" || arg == "-csv")
        {
            csv_log = true;
            remove("../Data.csv");
        }
    }

    int rc;
    MDB_env *env;
    mdb_env_create(&env);
    mdb_env_set_mapsize(env, 1 * 1024 * 1024 * 1024); // 1GB Size mdb_env_set_mapsize(env, 10485760);
    // mdb_size_t max_resize = 1 * 1024 * 1024 * 1024;
    mdb_env_set_maxdbs(env, 2);
    mdb_env_open(env, "../DataDB", 0, 0664); // 0664 --> 10mb -_- 1 * 1024 * 1024 * 1024 --> 1GB

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    MDB_dbi data;
    rc = mdb_dbi_open(txn, "data_db", MDB_CREATE, &data);
    if (rc != 0)
    {
        std::cout << "Error opening database 2: " << mdb_strerror(rc) << std::endl;
        return 1;
    }

    MDB_val key, value;
    std::string user_name;

    std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now();
    key.mv_data = &user_name[0];
    key.mv_size = user_name.size();
    value.mv_data = &current_time;
    value.mv_size = sizeof(current_time);
    cout << "data storage : " << endl;

    MDB_cursor *cursor;
    rc = mdb_cursor_open(txn, data, &cursor);
    if (rc != 0)
    {
        std::cout << "Error creating cursor2: " << mdb_strerror(rc) << std::endl;
        return 1;
    }
    while (mdb_cursor_get(cursor, &key, &value, MDB_NEXT) == 0)
    {
        std::string user_name(static_cast<char *>(key.mv_data), key.mv_size);
        std::chrono::time_point<std::chrono::system_clock> entry_time = *(std::chrono::time_point<std::chrono::system_clock> *)value.mv_data;
        time_t entry_time_t = std::chrono::system_clock::to_time_t(entry_time);
        std::cout << "User: " << user_name << ", Entry time: " << std::ctime(&entry_time_t) << "Epoch Timer: " << entry_time_t << std::endl;
        std::cout << "__________________________________________" << endl;
        if (csv_log)
        {
            ofstream myfile;
            myfile.open("../Data.csv", ios::out | ios::app);
            myfile << user_name << "," << std::ctime(&entry_time_t) << "\n";
            myfile.close();
        }
    }
    std::cout << "--------------------------------------------------------------------------------------------------------------------------" << endl;
    if (rc != MDB_NOTFOUND)
    {
        std::cout << "Error iterating through key-value pairs: " << mdb_strerror(rc) << std::endl;
    }
    // Commit the transaction

    mdb_cursor_close(cursor);

    mdb_txn_commit(txn);
    mdb_dbi_close(env, data);

    mdb_env_close(env);

    return 0;
}