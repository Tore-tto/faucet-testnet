#include <iostream>
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "served/served.hpp"

using json =nlohmann::json;

std::string faucet ="/faucet";
std::string port = "19090";
std::string IpAddress = "127.0.0.1";
int ThreadCount = 10;
class routers {
    private:
    served::multiplexer mux;

    public:
    routers(served::multiplexer mux_) : mux(mux_) {}

    auto faucetSend(){
        return [&](served::response &res, const served::request &req)
            {
                res << "hi, i am userfaucet send endpoint";
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
    json getMasterNode = {
        {"jsonrpc", "2.0"},
        {"id", "0"},
        {"method", "get_master_nodes"},
        {"params", {{"master_node_pubkeys", {"cfbb3d7abb041fec8c43ff2351f21142d7558ab76acf6d9af188c350cecd1e4f"}}}}
    };
    cpr::Response r =
        cpr::Post(cpr::Url{"http://explorer.beldex.io:19091/json_rpc"},
                  cpr::Body{getMasterNode.dump()},
                  cpr::Header{{"Content-Type", "application/json"}});
    std::cout << r.text;

    served::multiplexer mux_;
    routers router(mux_);
    router.EndpointHandler();
    router.StartServer();

}