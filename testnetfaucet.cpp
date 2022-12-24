#include <iostream>
#include <fstream>
#include <sstream>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"
#include <thread>
#include <chrono>

using json = nlohmann::json;
using namespace std::chrono;
using namespace std;

std::string faucet ="/faucet";
std::string port = "19090";
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
// Transfering to the required users
namespace Faucet
{

    std::string transferFaucet(std::string _address)
    {

        json transferBody = {
            {"jsonrpc", "2.0"},
            {"id", "0"},
            {"method", "transfer"},
            {"params", {{"destinations", {{{"amount", 1000000000}, {"address", _address}}}}, {"account_index", 0}, {"subaddr_indices", {0}}, {"priority", 0}, {"ring_size", 7}, {"get_tx_key", true}}}};

        cpr::Response r = cpr::Post(cpr::Url{"http://38.242.196.76:19092/json_rpc"},
                                    cpr::Body{transferBody.dump()},
                                    cpr::Header{{"Content-Type", "application/json"}});
        return r.text;
    }
}
auto successbody(served::response &res, json resultTx)
{

    json result_body = {
        {"BDX", "Congrats you got today's reward"},
        {"STATUS", "OK"},
        {"HASH", resultTx["result"]["tx_hash"]},
        {"WAIT", "Wait for 24hrs !"}};

    res << result_body.dump();
}

auto unsuccessbody(served::response &res)
{
    json result_body = {
        {"STATUS", "FAIL"},
        {"WARNING", "ADDRESS ALREADY GOT THE REWARD"}};
    res << result_body.dump();
}

class routers
{
private:
    served::multiplexer mux;

public:
    routers(served::multiplexer mux_) : mux(mux_) {}

    auto faucetSend()
    {
        return [&](served::response &res, const served::request &req)
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

                if (content[i][0] != address) //
                {
                    continue;
                }
                else
                {

                    int i_dec = std::stoi(content[i][1]);
                    auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());

                    if (i_dec >= givemetime)
                    {
                        dup = true;
                        unsuccessbody(res);
                        break;
                    }
                    dup_line_num = i;
                    dup_add = true;
                }
            }

            if (!dup)
            {

                for (;;)
                {
                    auto givemetime = chrono::system_clock::to_time_t(chrono::system_clock::now());
                    auto endtime = givemetime + 86400;
                    std::string result = Faucet::transferFaucet(address);
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

                        /*------------------------------------History creation for old and new address----------------------------*/
                        ofstream myfile;
                        if (dup_add)
                        {

                            myfile.open("Data.csv");
                            for (int i = 0; i < content.size(); i++)
                            {

                                if (i == dup_line_num)
                                {

                                    myfile << content[i][0] << "," << endtime << "\n";
                                }
                                else
                                {

                                    myfile << content[i][0] << "," << content[i][1] << "\n";
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
            }
        };
    }

    void EndpointHandler()
    {
        mux.handle(faucet).post(faucetSend());
    }

    void StartServer()
    {
        std::cout << "server listioning at " << port << std::endl;
        served::net::server server(IpAddress, port, mux);
        server.run(ThreadCount);
    }
};

int main()
{

    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler();
    router.StartServer();
}