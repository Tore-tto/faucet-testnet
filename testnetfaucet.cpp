#include <iostream>
#include <fstream>
#include <sstream>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"
#include <thread>
#include <chrono>
#include <vector>

using json = nlohmann::json;
using namespace std::chrono;
using namespace std;

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
    const char *a = " : ";
    json result_body = {
        {"STATUS", "FAIL"},
        {"REMAINING TIME", hrs, a, mins, a, secs},
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

            int dup_line_num = 0;
            bool dup = false;
            vector<vector<string>> content;
            vector<string> row;
            string a, line, word;
            fstream file("Data.csv", ios::in);
            bool dup_add = false;

            if (file.is_open())
            {
                while (getline(file, line))
                {
                    row.clear();
                    stringstream str(line);
                    while (getline(str, word, ','))
                    {
                        row.push_back(word);
                    }

                    content.push_back(row);
                }
                file.close();
            }
            else
            {
                cout << "Could not open the file\n";
            }

            for (int i = 0; i < content.size(); i++)
            {
                //  If address is not same it continuous.
                if (content[i][0] != address)
                {
                    continue;
                }
                else // If adddress is same or before 24hrs.
                {
                    int i_dec = std::stoi(content[i][1]);
                    auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());

                    if (i_dec >= givemetime)
                    {
                        dup = true;
                        if (help_log)
                        {
                            json unsuccess_body = {
                                {"Result", "FAIL"}, {"Data", "Address already got the reward !"}};
                            cout << unsuccess_body << endl;
                        }
                        cout << "Address already got the reward" << endl;

                        int hrs;
                        int mins;
                        int secs;
                        int rem_time = i_dec - givemetime;
                        hrs = rem_time / 3600;
                        mins = (rem_time % 3600) / 60;
                        secs = rem_time % 60;

                        unsuccessbody(res, hrs, mins, secs);
                        break;
                    }

                    dup_line_num = i;
                    dup_add = true;
                }
            }

            if (!dup)
            {
                auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                auto endtime = givemetime + 86400;
                for (;;)
                {
                    auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                    auto endtime = givemetime + 86400;

                    // calling transfer faucet for transaction.

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
                        /*------------------------------------History and data creation for old and new address----------------------------*/
                        ofstream myfile;
                        if (dup_add)
                        {
                            myfile.open("Data.csv");
                            for (int i = 0; i < content.size(); i++)
                            {

                                if (i == dup_line_num)
                                {
                                    myfile << content[i][0] << "," << endtime << "\n"; // same address and replacing time
                                }
                                else
                                {
                                    myfile << content[i][0] << "," << content[i][1] << "\n"; // same address and same time
                                }
                            }
                            myfile.close();
                        }
                        else
                        {
                            myfile.open("Data.csv", ios::out | ios::app);
                            myfile << address << "," << endtime << "\n";
                            myfile.close();
                        }
                        ofstream myfile1;
                        myfile1.open("History.csv", ios::out | ios::app);
                        myfile1 << address << "," << ctime(&givemetime) << endl;

                        myfile1.close();
                        successbody(res, resultTx);
                        break;
                    }
                }
                if (help_log)
                {
                    json success_body = {
                        {{"Result", "OK"}, {"Data", "Congrats you got the today's reward:1bdx"}}};
                    cout << success_body << endl;
                }
            }
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

    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler(argc, argv);
    router.StartServer();
}
