#include <iostream>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"
#include <thread>
#include <chrono>

using json =nlohmann::json;

std::string faucet ="/faucet";
std::string port = "19090";
std::string IpAddress = "127.0.0.1";
int ThreadCount = 10;

namespace validation{
    // user address validation
    bool addressValidation(const served::request &req,served::response &res,std::string &_address)
    {
        json request = json::parse(req.body());
        if(!(request.contains("address") && (request["address"].get_ref<json::string_t&>().size() == 95))){
            res <<"invalid input address";
            return false;
        }
        _address = request["address"];
        return true;
    }
}

namespace Faucet{
// $ curl -X POST http://127.0.0.1:19092/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"transfer","params":{"destinations":[{"amount":100000000000,"address":"8BnERTpvL5MbCLtj5n9No7J5oE5hHiB3tVCK5cjSvCsYWD2WRJLFuWeKTLiXo5QJqt2ZwUaLy2Vh1Ad51K7FNgqcHgjW85o"},{"amount":200000000000,"address":"75sNpRwUtekcJGejMuLSGA71QFuK1qcCVLZnYRTfQLgFU5nJ7xiAHtR5ihioS53KMe8pBhH61moraZHyLoG4G7fMER8xkNv"}],"account_index":0,"subaddr_indices":[0],"priority":1,"ring_size":7,"get_tx_key": true}}' -H 'Content-Type: application/json'
                // {"params", {"destinations":[{"amount":200000000000,"address":"75sNpRwUtekcJGejMuLSGA71QFuK1qcCVLZnYRTfQLgFU5nJ7xiAHtR5ihioS53KMe8pBhH61moraZHyLoG4G7fMER8xkNv"}],"account_index":0,"subaddr_indices":[0],"priority":1,"ring_size":7,"get_tx_key": true}}}}

    std::string transferFaucet(std::string _address){
        json transfer = R"({"jsonrpc":"2.0","id":"0","method":"transfer","params":{"destinations":[{"amount":100000000,"address":"BafzHZ1bF9BHjYNj4FyBsPip4atUyTX3LaPr4B7VGHRC5iuP2n4xAsDTViEL3h6CvaBy414gQe4HHcxEkJPGwNVZCaGkYFJ"}],"account_index":0,"subaddr_indices":[0],"priority":0,"ring_size":7,"get_tx_key": true}}
        )"_json;

        json transferBody = {
            {"jsonrpc", "2.0"},
            {"id", "0"},
            {"method", "transfer"},
            {"params", {{"destinations", {
                        {{"amount",1000000000},{"address",_address}}
                        }},
                        {"account_index",0},{"subaddr_indices",{0}},{"priority",0},{"ring_size",7},{"get_tx_key",true}
                }}
         };

        cpr::Response r =
        cpr::Post(cpr::Url{"http://38.242.196.76:19092/json_rpc"},
                  cpr::Body{transferBody.dump()},
                  cpr::Header{{"Content-Type", "application/json"}});
        return r.text;
    }
}

class routers {
    private:
    served::multiplexer mux;

    public:
    routers(served::multiplexer mux_) : mux(mux_) {}

    auto faucetSend(){
        return [&](served::response &res, const served::request &req)
            {
                std::string address;
                if(!validation::addressValidation(req,res,address))
                {
                    return;
                }
            for(;;)
            {
                std::string result = Faucet::transferFaucet(address);
                json resultTx = json::parse(result);
                std::cout <<resultTx << std::endl;
                if(resultTx.contains("error")){
                    if(resultTx["error"]["code"] != -2)
                    {
                        if(resultTx["error"]["code"] == -37){
                            std::cout <<"not enough money\n";
                            std::this_thread::sleep_for(std::chrono::seconds(70));
                            continue;
                        }
                        std::cout <<"output distribution\n";
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                        continue;
                    }
                    res << resultTx["error"]["message"];
                    break;
                }
                else{
                    res<< resultTx["result"]["tx_hash"];
                    break;
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

int main(){
    // json getMasterNode = {
    //     {"jsonrpc", "2.0"},
    //     {"id", "0"},
    //     {"method", "get_master_nodes"},
    //     {"params", {{"master_node_pubkeys", {"cfbb3d7abb041fec8c43ff2351f21142d7558ab76acf6d9af188c350cecd1e4f"}}}}
    // };
    // cpr::Response r =
    //     cpr::Post(cpr::Url{"http://explorer.beldex.io:19091/json_rpc"},
    //               cpr::Body{getMasterNode.dump()},
    //               cpr::Header{{"Content-Type", "application/json"}});
    // std::cout << r.text;

    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler();
    router.StartServer();

}