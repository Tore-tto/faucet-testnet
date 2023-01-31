#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"
#include "database.h"
#include <thread>
#include <chrono>
#include <vector>
#include <map>

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

    auto faucetSend(int argc, char *argv[], database _data)
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
        return [help_log, argc, argv, _data](served::response &res, const served::request &req)
        {
            database data = _data;
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
            data.getvalue(address);
            auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
            if (data.last_entry_time == 0)
            {
                //new entry
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
                        his = true;
                        auto updatetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                        data.insertData(address, updatetime);
                        if (help_log)
                        {
                            json success_body = {
                                {{"Result", "OK"}, {"Data", "Congrats you got the today's reward:1bdx"}}};
                            cout << success_body << endl;
                        }

                        successbody(res, resultTx);
                        loop_address[address] = false;
                        break;
                    }
                }
            }
            else if ((givemetime - data.last_entry_time) > 86400)
            {
                //update entry
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
                        his = true;
                        auto updatetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                        data.updateData(address, updatetime);
                        if (help_log)
                        {
                            json success_body = {
                                {{"Result", "OK"}, {"Data", "Congrats you got the today's reward:1bdx"}}};
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
                auto updatetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                int hrs, mins, secs, rem_time = (data.last_entry_time + 86400) - updatetime;
                hrs = rem_time / 3600;
                mins = (rem_time % 3600) / 60;
                secs = rem_time % 60;
                std::cout << "address already got reward\n";
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
                auto updatetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                string current_time_str = std::ctime(&updatetime);
                current_time_str.pop_back();
                data.historyinsert(address, current_time_str);
            }
        };
    }

    void EndpointHandler(int argc, char *argv[], database _data)
    {
        mux.handle(faucet).post(faucetSend(argc, argv, _data));
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
    // database operations
    database data;
    data.opendatabase();
    data.tableCreation();

    // server operations
    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler(argc, argv, data);
    router.StartServer();

    data.closeDatabase();
    return 0;
}