#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include <lmdb.h>
#include <filesystem>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"
#include "database.h"

using json = nlohmann::json;
using namespace std::chrono;
using namespace std;

map<string, bool> loop_address;
bool ip_log = false;
std::string http = "http://";
std::string ip = "38.242.196.76:19092";
std::string rpc = "/json_rpc";

std::string faucet = "/faucet";
std::string port = "19095";
std::string IpAddress = "127.0.0.1";
int ThreadCount = 10;

namespace validation
{
    // user address validation
    bool addressValidation(const served::request &req, served::response &res, std::string &_address)
    {
        json request = json::parse(req.body());
        if (!(request.contains("address") && (request["address"].get_ref<json::string_t &>().size() == 95)))
        {
            res << "invalid input address";
            return false;
        }

        _address = request["address"];
        return true;
    }
}

namespace Faucet
{
    std::string transferFaucet(std::string _address, int argc, char *argv[])
    {
        // json transfer = R"({"jsonrpc":"2.0","id":"0","method":"transfer","params":{"destinations":[{"amount":100000000,"address":"BafzHZ1bF9BHjYNj4FyBsPip4atUyTX3LaPr4B7VGHRC5iuP2n4xAsDTViEL3h6CvaBy414gQe4HHcxEkJPGwNVZCaGkYFJ"}],"account_index":0,"subaddr_indices":[0],"priority":0,"ring_size":7,"get_tx_key": true}}
        // )"_json;

        json transferBody = {
            {"jsonrpc", "2.0"},
            {"id", "0"},
            {"method", "transfer"},
            {"params", {{"destinations", {{{"amount", 10000000000}, {"address", _address}}}}, {"account_index", 0}, {"subaddr_indices", {0}}, {"priority", 0}, {"ring_size", 7}, {"get_tx_key", true}}}};

        // condition to RPC command line arg
        string ip_port;
        for (int i = 0; i < argc; ++i)
        {
            string arg = argv[i];
            if (0 == arg.find("--ip-port"))
            {
                ip_log = true;
                size_t found = arg.find_first_of("--ip-port");
                ip_port = arg.substr(found + 1);
                string prefix = "-ip-port=";
                ip_port.erase(0, prefix.length());
            }
        }

        cpr::Response r = cpr::Post(cpr::Url{ip_log ? (http + ip_port + rpc) : (http + ip + rpc)}, //"http://38.242.196.76:19092/json_rpc"
                                    cpr::Body{transferBody.dump()},
                                    cpr::Header{{"Content-Type", "application/json"}});
        return r.text;
    }
}

// Function for json format
auto successbody(served::response &res, json resultTx)
{
    json result_body = {
        {"BDX", "Congrats you got today's reward"},
        {"STATUS", "OK"},
        {"HASH", resultTx["result"]["tx_hash"]}};
    res << result_body.dump();
}

auto unsuccessbody(served::response &res, int hrs, int mins, int secs)
{
    cout << "REMAINING TIME: " << hrs << ":" << mins << ":" << secs << endl;
    json result_body = {
        {"STATUS", "FAIL"},
        {"REMAINING TIME", {hrs, mins, secs}},
        {"WARNING", "ADDRESS ALREADY GOT THE REWARD"}};
    res << result_body.dump();
}

class routers
{
private:
    served::multiplexer mux;

public:
    routers(served::multiplexer mux_) : mux(mux_) {}

    auto faucetSend(int argc, char *argv[])
    {
        // command line arg for json
        bool help_log = false;
        vector<string> cmdLineArgs(argv, argv + argc);
        for (auto &arg : cmdLineArgs)
        {
            if (arg == "--json" || arg == "-json")
            {
                help_log = true;
            }
        }
        return [help_log, argc, argv](served::response &res, const served::request &req)
        {
            std::string address;
            if (!validation::addressValidation(req, res, address))
            {
                return;
            }
            if (loop_address[address] == true)
            {
                return;
            }
            loop_address[address] = true;
            bool his = false;
            bool reward_got = true;
            // Open the LMDB database
            MDB_env *env;
            mdb_env_create(&env);
            mdb_env_set_mapsize(env, 1 * 1024 * 1024 * 1024); //1 * 1024 * 1024 * 1024 --> 1GB
            mdb_env_set_maxdbs(env, 2);
            mdb_env_open(env, "../DataDB", 0, 0664); 

            MDB_txn *txn;
            mdb_txn_begin(env, NULL, 0, &txn);

            MDB_dbi data_db, history_db;
           
            mdb_dbi_open(txn, "data_db", MDB_CREATE, &data_db);
            mdb_dbi_open(txn, "history_db", MDB_CREATE, &history_db);
            
            MDB_val key, value;
            std::chrono::time_point<std::chrono::system_clock> givemetime = std::chrono::system_clock::now();

            key.mv_data = &address[0];
            key.mv_size = address.size();
            int result = mdb_get(txn, data_db, &key, &value);
            if (result == MDB_NOTFOUND)
            {
                // User has not entered before, so store the current time as the entry time
                for (;;)
                {
                    std::string result = Faucet::transferFaucet(address, argc, argv);
                    json resultTx = json::parse(result);
                    std::cout << resultTx << std::endl;
                    if (resultTx.contains("error"))
                    {
                        if (resultTx["error"]["code"] != -2)
                        {
                            if (resultTx["error"]["code"] == -37)
                            {
                                std::cout << "not enough money\n";
                                std::this_thread::sleep_for(std::chrono::seconds(70));
                                continue;
                            }
                            std::cout << "output distribution\n";
                            std::this_thread::sleep_for(std::chrono::seconds(10));
                            continue;
                        }
                        res << resultTx["error"]["message"];
                        break;
                    }
                    else
                    {
                        reward_got = false;
                        his = true;
                        value.mv_data = &givemetime;
                        value.mv_size = sizeof(givemetime);
                        mdb_put(txn, data_db, &key, &value, 0);

                        if (help_log)
                        {
                            json success_body = {
                                {{"Result", "OK"}, {"Data", "Congrats you got the today's reward"}}};
                            cout << success_body << endl;
                        }

                        successbody(res, resultTx);
                        loop_address[address] = false;
                        break;
                    }
                }
            }
            else
            {
                // User has entered before, so check if 24 hours have passed
                std::chrono::time_point<std::chrono::system_clock> last_entry = *(std::chrono::time_point<std::chrono::system_clock> *)value.mv_data;
                std::chrono::duration<double> diff = givemetime - last_entry;

                if (diff.count() >= 24 * 60 * 60)
                {
                    // update entry
                    for (;;)
                    {
                        std::string result = Faucet::transferFaucet(address, argc, argv);
                        json resultTx = json::parse(result);
                        std::cout << resultTx << std::endl;
                        if (resultTx.contains("error"))
                        {
                            if (resultTx["error"]["code"] != -2)
                            {
                                if (resultTx["error"]["code"] == -37)
                                {
                                    std::cout << "not enough money\n";
                                    std::this_thread::sleep_for(std::chrono::seconds(70));
                                    continue;
                                }
                                std::cout << "output distribution\n";
                                std::this_thread::sleep_for(std::chrono::seconds(10));
                                continue;
                            }
                            res << resultTx["error"]["message"];
                            break;
                        }
                        else
                        {
                            reward_got = false;
                            his = true;
                            // More than 24 hours have passed, so store the current time and new user name as the entry time
                            value.mv_data = &givemetime;
                            value.mv_size = sizeof(givemetime);
                            mdb_put(txn, data_db, &key, &value, 0);

                            if (help_log)
                            {
                                json success_body = {
                                    {{"Result", "OK"}, {"Data", "Congrats you got the today's reward"}}};
                                cout << success_body << endl;
                            }
                            successbody(res, resultTx);
                            loop_address[address] = false;
                            break;
                        }
                    }
                }
            }
            if (reward_got)
            {
                std::chrono::time_point<std::chrono::system_clock> last_entry = *(std::chrono::time_point<std::chrono::system_clock> *)value.mv_data;
                time_t entry_time_t = std::chrono::system_clock::to_time_t(last_entry);

                auto updatetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                int hrs, mins, secs, rem_time = (entry_time_t + 86400) - updatetime;
                hrs = rem_time / 3600;
                mins = (rem_time % 3600) / 60;
                secs = rem_time % 60;
                std::cout << "Address: " << address << " already got reward\n";
                if (help_log)
                {
                    json unsuccess_body = {
                        {"Result", "FAIL"}, {"Data", "Address already got the reward !"}};
                    cout << unsuccess_body << endl;
                }
                unsuccessbody(res, hrs, mins, secs);
                loop_address[address] = false;
            }

            if (his)
            {
                std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now();
                std::string entry_key = address + ":" + std::to_string(current_time.time_since_epoch().count());

                MDB_val key, value;
                key.mv_data = &entry_key[0];
                key.mv_size = entry_key.size();
                value.mv_data = &current_time;
                value.mv_size = sizeof(current_time);

                mdb_put(txn, history_db, &key, &value, 0);
            }
            mdb_txn_commit(txn);
            mdb_dbi_close(env, history_db);

            // Close the LMDB database
            mdb_dbi_close(env, data_db);
            mdb_env_close(env);
        };
    }

    void EndpointHandler(int argc, char *argv[])
    {
        mux.handle(faucet).post(faucetSend(argc, argv));
    }

    void StartServer()
    {
        std::cout << "server listioning at " << port << std::endl;
        served::net::server server(IpAddress, port, mux);
        server.run(ThreadCount);
    }
};

int main(int argc, char *argv[])
{
    // LMDB Folder open
    string Data_folder = "../DataDB";
    int check = filesystem::create_directories(Data_folder);
    if (check)
    {
        std::cout << "Folder created" << std::endl;
    }
    else
    {
        std::cout << "Folder already exist" << std::endl;
    }

    // server operations
    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler(argc, argv);
    router.StartServer();

    return 0;
}