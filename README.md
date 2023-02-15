# faucet-testnet
Api for providing BDX testnet faucets

>CLONING THE REPOSITORY

`git clone --recursive https://github.com/victor-tucci/faucet-testnet.git`

>PROCEDURE TO BUILD
```
 cd faucet-testnet 
 mkdir build
 cd build
 cmake ..
 make
 ./testnetfaucet
```
>FOR JSON

`After build the faucet if you need to print the json in terminal means just add`

`./testnetfaucet --json` | `./testnetfaucet -json`

>FOR USER RPC

`./testnetfaucet --ip-port="ip:port"`
