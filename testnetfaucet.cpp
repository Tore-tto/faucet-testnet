#include <iostream>
#include "cpr/cpr.h"
#include <nlohmann/json.hpp>

using json =nlohmann::json;

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
}