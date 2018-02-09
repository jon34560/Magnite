
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include "utilstrencodings.h"
#include "global.h"
#include "network/relayclient.h"
#include "network/p2p.h"
#include "functions/selector.h"
#include "functions/chain.h"
//#include "wallet/wallet.h"
//#include "key.h"
//#include "pubkey.h"

//#include <openssl/crypto.h> // no worky
//#include <openssl/ec.h>
#include <stdio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

//#include "rsacrypto.h"
#include "ecdsacrypto.h"
#include "wallet.h"
#include "transaction.h"

//#include "userdb.h"
//#include "leveldb/db.h" // TEMP 
#include "blockdb.h"

#include "functions/functions.h"

//#include "network/server.h"
#include "cli.h"
#include <unistd.h>		// sleep
#include <ctime>

//static const uint64_t BUFFER_SIZE = 1000*1000; // Temp
using namespace std;

volatile bool buildingBlocks = true;

/**
* blockBuilderThread TODO: Move this to blockbuilder.cpp
*
* Description:
* std::thread blockThread (blockBuilderThread);
*/
void blockBuilderThread(int argc, char* argv[]){
	CECDSACrypto ecdsa;
	CFunctions functions;
        CRelayClient relayClient;
        CSelector selector;
        selector.syncronizeTime();
        CChain chain;
	// Check block chain for latest block information.
	// TODO...

        // Syncronize local time with server
        // get local time and server time and calculate difference to add to local time.
        // ....

	// Get current user keys
	std::string privateKey;
	std::string publicKey;
	CWallet wallet;
	bool e = wallet.fileExists("wallet.dat");
	//printf(" wallet exists: %d  \n", e);
	if(e == 0){
        	//printf("No wallet found. Creating a new one...\n");
	} else {
		// Load wallet
        	wallet.read(privateKey, publicKey);
        	//std::cout << "  private  " << privateKey << "\n  public " << publicKey << "\n " << std::endl;
	}
        functions.parseBlockFile( publicKey, false );
        relayClient.getNewNetworkPeer(publicKey);

	bool networkGenesis = false;
        std::string networkName = "main";
        if(argc >= 3 ){ // && argv[1] == "genesis"
            	networkGenesis = true;
		networkName = argv[2];
		//std::cout << " 1 " << argv[1] << " " << argv[2] ;
        }

        long timeBlock = selector.getCurrentTimeBlock();

	if(networkGenesis){
		std::cout << "Creating genesis block for a new network named " << networkName << std::endl;	

		CFunctions::record_structure joinRecord;
		joinRecord.network = networkName;
                time_t  timev;
                time(&timev);
                std::stringstream ss;
                ss << timev;
                std::string ts = ss.str();
                joinRecord.time = ts;
                CFunctions::transaction_types joinType = CFunctions::JOIN_NETWORK;
                joinRecord.transaction_type = joinType;
                joinRecord.amount = 0.0;
                joinRecord.fee = 0;
                joinRecord.sender_public_key = publicKey;
                joinRecord.recipient_public_key = "";
                joinRecord.hash = functions.getRecordHash(joinRecord);
		std::string signature = "";
                ecdsa.SignMessage(privateKey, joinRecord.hash, signature);
		joinRecord.signature = signature;   

                // *** TESTING ONLY ***
                std::string fictionPrivateKey;
                std::string fictionPublicKey;
                std::string uncompressed;
                int r = ecdsa.RandomPrivateKey(fictionPrivateKey);
                r = ecdsa.GetPublicKey(fictionPrivateKey, uncompressed, fictionPublicKey);
                CFunctions::record_structure fictionJoinRecord;
                fictionJoinRecord.network = networkName; 
                fictionJoinRecord.time = ts;
                fictionJoinRecord.transaction_type = joinType;
                fictionJoinRecord.amount = 0.0;
                fictionJoinRecord.fee = 0;
                fictionJoinRecord.sender_public_key = fictionPublicKey;
                fictionJoinRecord.recipient_public_key = "";
                fictionJoinRecord.hash = functions.getRecordHash(fictionJoinRecord);
                ecdsa.SignMessage(fictionPrivateKey, fictionJoinRecord.hash, signature);
                fictionJoinRecord.signature = signature;
		// *** END TESTING ***

                CFunctions::record_structure blockRewardRecord;
                blockRewardRecord.network = networkName;
		blockRewardRecord.time = ts;
                CFunctions::transaction_types transaction_type = CFunctions::ISSUE_CURRENCY;
                blockRewardRecord.transaction_type = transaction_type;
                blockRewardRecord.amount = 1.0;
                blockRewardRecord.sender_public_key = publicKey; 
                blockRewardRecord.recipient_public_key = publicKey;
		blockRewardRecord.hash = functions.getRecordHash(blockRewardRecord); 
                ecdsa.SignMessage(privateKey, blockRewardRecord.hash, signature);
                blockRewardRecord.signature = signature;

		CFunctions::block_structure block;
                block.creator_key = publicKey;
		block.network = networkName;
                block.records.push_back(joinRecord);
                block.records.push_back(fictionJoinRecord);
                block.records.push_back(blockRewardRecord);

		time_t t = time(0);
                std::string block_time = std::asctime(std::localtime(&t));

		block.number = selector.getCurrentTimeBlock();
                block.time = block_time;

                block.hash = functions.getBlockHash(block);
                ecdsa.SignMessage(privateKey, block.hash, signature);
                block.signature = signature; 

                chain.setFirstBlock(block);
                functions.addToBlockFile(block);
                relayClient.sendBlock(block);

                // Wait until the block period is over
                long currTimeBlock = selector.getCurrentTimeBlock();
                while( currTimeBlock == timeBlock ){
                    usleep(1000000);
                    currTimeBlock = selector.getCurrentTimeBlock();
                    //std::cout << "." << std::endl;
                }
	}

	// Does a blockfile exist? if networkGenesis==false and there is no block file we wait until the network syncs before wrting to the blockfile. 
        CFunctions::block_structure previous_block;

        // syncronize chain
        if(networkGenesis == false && chain.getFirstBlock() == -1 ){
            // 
            std::cout << "Syncronizing Blockchain." << std::endl;

            // Expected latest block number.  
            long currBlock = selector.getCurrentTimeBlock();
            std::cout << "Current time: " << currBlock << " latest: " << chain.getLatestBlock() << std::endl;

            relayClient.sendRequestBlocks(-1); // Request the beginning of the blockchain from our peer nodes.

            //std::cout << "-" << std::endl; 
            int progressPos = 0;
            while( chain.getLatestBlock() < currBlock - 1 && buildingBlocks ){
                if(progressPos == 0){
                    std::cout << "\rSynchronizing: " << "|" << std::flush; progressPos++; 
                } else if(progressPos == 1){
                    std::cout << "\rSynchronizing: " << "/" << std::flush; progressPos++; 
                } else if(progressPos == 2){
                    std::cout << "\rSynchronizing: " << "-" << std::flush; progressPos++;
                } else if(progressPos == 3){
                    std::cout << "\rSynchronizing: " << "-" << std::flush; progressPos = 0;
                }

                // if no progress, send request again.

                usleep(1000000);  
            }
        }
 
	int blockNumber = functions.latest_block.number + 1;
        //long timeBlock = 0; 
	while(buildingBlocks){
            functions.parseBlockFile(publicKey, false);
            timeBlock = selector.getCurrentTimeBlock();	
            //std::cout << "here " << std::endl;
	
		// is it current users turn to generate a block.
		bool build_block = selector.isSelected(publicKey);
                if(functions.joined == false){ // Can't build block unless a member of the block.
                    build_block = false;       
                }
                //std::cout << "here 2 " << std::endl;

		time_t t = time(0);
		std::string block_time = std::asctime(std::localtime(&t));
  
		if(build_block){

			// While time remaining in block
			if(!buildingBlocks){
				return;
			}

			time_t  timev;
                        time(&timev);
                        std::stringstream ss;
                        ss << timev;
                        std::string ts = ss.str();

                        CFunctions::record_structure blockRewardRecord;
                        blockRewardRecord.network = networkName;
                        blockRewardRecord.time = ts;
                        blockRewardRecord.transaction_type = CFunctions::ISSUE_CURRENCY;
                        blockRewardRecord.amount = 1.0;
                        blockRewardRecord.fee = 0;
                        blockRewardRecord.sender_public_key = publicKey;
                        blockRewardRecord.recipient_public_key = publicKey;
                        blockRewardRecord.hash = functions.getRecordHash(blockRewardRecord);
                        std::string signature = "";
                        ecdsa.SignMessage(privateKey, blockRewardRecord.hash, signature);
                        blockRewardRecord.signature = signature;


                        // ***
                        // Example transaction from transaction queue file.
                        // ***
			CFunctions::record_structure sendRecord;
                        sendRecord.time = ts;
                        sendRecord.transaction_type = CFunctions::TRANSFER_CURRENCY;
                        sendRecord.amount = 0.0123;
                        sendRecord.fee = 0.001;
			sendRecord.sender_public_key = publicKey;
                        sendRecord.recipient_public_key = "BADADDRESS___";
		        sendRecord.hash = functions.getRecordHash(sendRecord);	
                        ecdsa.SignMessage(privateKey, sendRecord.hash, signature);
                        sendRecord.signature = signature;


			// TODO: review this
			CFunctions::record_structure periodSummaryRecord;
			periodSummaryRecord.time = ts;
			periodSummaryRecord.transaction_type = CFunctions::PERIOD_SUMMARY;
                        periodSummaryRecord.sender_public_key = publicKey;
			periodSummaryRecord.recipient_public_key = "___MINER_ADDRESS___"; // reward for summary inclusion goes to block creator. (Only if record does not exist.)
			periodSummaryRecord.signature = "TO DO";	
		        periodSummaryRecord.hash = functions.getRecordHash(periodSummaryRecord);              
                        ecdsa.SignMessage(privateKey, periodSummaryRecord.hash, signature);
                        periodSummaryRecord.signature = signature;
	
                        previous_block = functions.getLastBlock("main");

			CFunctions::block_structure block;
			block.creator_key = publicKey; 
                        block.records.push_back(blockRewardRecord);
			block.records.push_back(sendRecord);
			block.records.push_back(periodSummaryRecord);
            
			block.number = selector.getCurrentTimeBlock(); //  blockNumber++;
			block.time = block_time;
            
            
			// Add records from queue...
			std::vector< CFunctions::record_structure > records = functions.parseQueueRecords();
			for(int i = 0; i < records.size(); i++){
				//printf(" record n");
                                block.records.push_back(records[i]);

                                // TODO: watch time so that there is enough to broadcast the block in order to have it accepted.
			}

                        block.previous_block_hash = previous_block.hash;
                        block.hash = functions.getBlockHash(block);
                        ecdsa.SignMessage(privateKey, block.hash, signature);
                        block.signature = signature;

			functions.addToBlockFile(block);                          
                        // TODO: Broadcast block to network
                        relayClient.sendBlock(block);

                        //previous_block = block; // temp 

			// Wait until the block period is over
                        long currTimeBlock = selector.getCurrentTimeBlock();
			while( currTimeBlock == timeBlock ){
                            usleep(1000000);
                            currTimeBlock = selector.getCurrentTimeBlock(); 
                            //std::cout << ".";
                        }
                        //std::cout << "block done" << std::endl;

		} else {
                    usleep(1000000);
                    //std::cout << "wait" << std::endl;
                }

		if(!buildingBlocks){
			usleep(1000000);
		}
       
	}	
}

void stop() {
    if ( !buildingBlocks ) return;
    buildingBlocks = false;
    // Join thread
}

int main(int argc, char* argv[])
{
    std::cout << ANSI_COLOR_RED << "Safire Digital Currency v0.0.1" << ANSI_COLOR_RESET << std::endl;
    std::cout << std::endl;
    // Start New BlockChain Mode
    // Read command line arg

    CFunctions functions;    
    /*
    CFunctions::record_structure record;
    record.time = "2017/06/03";
    CFunctions::transaction_types type = CFunctions::JOIN_NETWORK;
    record.transaction_type = type;
    record.amount = 0.1;
    record.sender_public_key = "Fsadhjsd6576asdaDGHSjghjasdASD";
    record.recipient_public_key = "SdhahBDGDhahsdbb6Bsdjj2dhjk";
    record.message_signature = "HDAJSHABbhdhsjagdbJHSDGJha";
    
    CFunctions functions;
    //functions.addToQueue(record);
    std::vector< CFunctions::record_structure > records = functions.parseQueueRecords();
    for(int i = 0; i < records.size(); i++){
        printf(" record n");
    }
    */
    CChain chain; 
    
    //std::string p;
    //std::string v;
    //std::string u;
    //std::cout << "  " << std::endl;	
    CECDSACrypto ecdsa;
    //int r = ecdsa.GetKeyPair(p, v, u );
    //std::cout << "  private  " << p << "\n  public " << v << "\n  Public compressed: " << u << std::endl; 
    //ecdsa.runTests();

    std::string privateKey;
    std::string publicKey;

    CWallet wallet;
    bool e = wallet.fileExists("wallet.dat");
    //printf(" wallet exists: %d  \n", e);
    if(e == 0){
        printf("No wallet found. Creating a new one...\n");
        std::string publicKeyUncompressed;
        int r = ecdsa.RandomPrivateKey(privateKey);
        r = ecdsa.GetPublicKey(privateKey, publicKeyUncompressed, publicKey);        
        wallet.write(privateKey, publicKey);
    } else {
        // Load wallet
        wallet.read(privateKey, publicKey);
    }
    std::cout <<
        //"  private  " << privateKey << "\n  " <<
        " Your public address: " << publicKey << std::endl;

    functions.parseBlockFile( publicKey, false );
    std::cout << " Your balance: " << functions.balance << " sfr" << std::endl; 

    std::cout << " Joined network: " << (functions.joined > 0 ? "yes" : "no") << std::endl;

    std::cout << std::endl;

    // Transactions
    /*
    CTransaction transaction;
    std::string join = transaction.joinNetwork(publicKey);
    std::cout << " join: " << join << std::endl;	
    std::string sendPayment = transaction.sendPayment( privateKey, publicKey, u, 23.45, 1);
    std::cout << " payment: " << sendPayment << std::endl;
     */
    
    //CUserDB userDB;
    //userDB.AddUser("test", "127.0.0.1");
    //userDB.GetUsers();


    //CBlockDB blockDB;
    //blockDB.AddBlock("First");
    //blockDB.GetBlocks();


    // Start Networking
    std::cout << " Starting networking.    " << ANSI_COLOR_GREEN << "[ok] " << ANSI_COLOR_RESET << std::endl;
    //CP2P p2p;
    //p2p.getNewNetworkPeer("123"); //TEMP
    CRelayClient relayClient;
    relayClient.getNewNetworkPeer(publicKey);

    // Validate chain
    std::cout << " Validating chain.       " << ANSI_COLOR_GREEN << "[ok] " << ANSI_COLOR_RESET << std::endl;

    // Interface type [CLI | GUI]
    // TODO: detect based on platform if GUI is supported.
    std::cout << " Interface type.         [cli] " << std::endl; 

    #ifdef __APPLE__
    std::cout << " Platform.               [OSX] " << std::endl;
    #endif
    #ifdef __linux__
    std::cout << " Platform.               [Linux] " << std::endl;
    #endif
    #ifdef _WIN32
    std::cout << " Platform.               [Windows] " << std::endl;
    #endif

    //std::size_t num_threads = 10;
    //http::server3::server s("0.0.0.0", "80", "/Users/jondtaylor/Dropbox/Currency", num_threads);

    // Run the server until stopped.
    //s.run();
    // std::thread webserver_thread (foo); // void foo()  
    
    // If not allready, send network request to join.
/*
    CFunctions::record_structure joinRecord;
    time_t  timev;
    time(&timev);
    std::stringstream ss;
    ss << timev;
    std::string ts = ss.str();
    joinRecord.time = ts;
    CFunctions::transaction_types joinType = CFunctions::JOIN_NETWORK;
    joinRecord.transaction_type = type;
    joinRecord.amount = 0.0;
    joinRecord.sender_public_key = publicKey;
    joinRecord.recipient_public_key = "";
    std::string message_siganture = "";
    ecdsa.SignMessage(privateKey, "add_user" + publicKey, message_siganture);
    joinRecord.message_signature = message_siganture;
    //functions.addToQueue(joinRecord);

    CFunctions::block_structure block;
    block.records.push_back(joinRecord);
    block.number = 0;
    
    //functions.addToBlockFile( block );
*/
    //functions.parseBlockFile();

    std::thread blockThread(blockBuilderThread, argc, argv);    
    //std::thread p2pNetworkThread(&CP2P::p2pNetworkThread, p2p, argc, argv); // TODO: implement a main class to pass into threads instead of 'p2p' instance. For communication.
    std::thread relayNetworkThread(&CRelayClient::relayNetworkThread, relayClient, argc, argv); 

    CCLI cli;
    std::cout << std::endl; // Line break
    cli.processUserInput();    

    std::cout << "Shutting down... " << std::endl;
    chain.writeFile();
    stop();
    blockThread.join();
    //p2p.exit();
    relayClient.exit();
    //p2pNetworkThread.join();
    relayNetworkThread.join();
 
    usleep(100000);
    std::cout << "Done " << std::endl;
    
}
