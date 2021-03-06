// Copyright (c) 2016 Jon Taylor
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cli.h"

#include <sstream>
#include <unistd.h>   // open and close
#include <sys/stat.h> // temp because we removed util
#include <fcntl.h> // temp removed util.h
#include <time.h>
#include "ecdsacrypto.h"
#include "functions/functions.h"
#include "wallet.h"
#include "network/p2p.h"
#include "network/relayclient.h"
#include "blockdb.h"
#include "userdb.h"

/**
* printCommands 
*
* Description: Print a list of commands that are accepted.
*/
void CCLI::printCommands(){
	std::cout << "Commands:\n" <<
	" join [network name]     - request membership in the network. The default network is 'main'.\n" <<
    " switch [network name]   - switch current network. Default is 'main'.\n" <<
    //" swtch wallet            - change wallet" <<
	" balance                 - print balance and transaction summary.\n" <<
	" sent                    - print sent transaction list details.\n" <<
	" received                - print received transaction list details.\n" <<
	" network                 - print network stats including currency and volumes.\n" <<
	" send                    - send a payment to another user address.\n" <<
	" receive                 - prints your public key address to have others send you payments.\n" <<
	" users                   - prints user address and balance information.\n" <<
    " vote                    - vote on network behaviour and settings.\n" <<
    " debug                   - log debug information.\n" <<
    " advanced                - more commands for admin and testing functions.\n" <<
	" quit                    - shutdown the application.\n" << std::endl;
}


void CCLI::printAdvancedCommands(){
    std::cout << " Advanced: \n" <<
    " reindex                - Clear and parse the entire blockchain dataset.\n" <<
    " tests                  - Run tests to verify this build is functioning correctly.\n" <<
    " chain                  - Scan the complete blockchain for verification. Reports findings.\n" <<
    " printchain             - Print the blockchain summary and validation.\n" <<
    " printqueue             - Print the record queue.\n" <<
    " resetall               - Delete node data.\n" <<
    " requestblock           - Send network request for block data. \n" <<
    std::endl;
}


/**
* processUserInput
*
* Description: process user commands and print results
*/
void CCLI::processUserInput(){
	bool running = true;
	CECDSACrypto ecdsa;
	CFunctions functions;
    CRelayClient relayClient;
	std::string privateKey;
	std::string publicKey;
	CWallet wallet;
	bool e = wallet.fileExists("wallet.dat");
	if(e != 0){
		// Load wallet
		wallet.read(privateKey, publicKey);
		//functions.parseBlockFile(publicKey, false);
        functions.scanChain(publicKey, false);
	}

	printCommands();
	while(running){
		std::cout << ">";		

		std::string command;
		std::cin >> command;

		if( command.find("join") != std::string::npos ){
			
			//CFunctions functions;
            //std::string privateKey;
            //std::string publicKey;
            //CWallet wallet;
            //bool e = wallet.fileExists("wallet.dat");
            //if(e != 0){
                // Load wallet
                //wallet.read(privateKey, publicKey);
                //functions.parseBlockFile(publicKey, false);
                functions.scanChain(publicKey, false);
            //}

			std::cout << "Enter network name to join (blank for default): \n" << std::endl;
			std::string networkName;
			std::cin >> networkName;
			if(networkName.compare("") == 0 ){
				networkName = "main";
			}
            
            std::cout << "Enter public user name (blank for default): \n" << std::endl;
            std::string userName;
            std::cin >> userName;
            // TODO: add

			if(functions.joined == true){ // TODO this needs to track different networks.
				std::cout << "Allready joined network. \n" << std::endl;
			} else {
				std::cout << "Joining request sending... \n" << std::endl;

				CFunctions::record_structure joinRecord;
				joinRecord.network = networkName;
				time_t  timev;
				time(&timev);
				std::stringstream ss;
				ss << timev;
				std::string ts = ss.str();
				joinRecord.time = ts;
				joinRecord.transaction_type = CFunctions::JOIN_NETWORK;
				joinRecord.amount = 0.0;
                joinRecord.fee = 0.0;
				joinRecord.sender_public_key = publicKey;
				joinRecord.recipient_public_key = "";
				joinRecord.hash = functions.getRecordHash(joinRecord);
                std::string message_siganture = "";
				ecdsa.SignMessage(privateKey, joinRecord.hash, message_siganture);
				joinRecord.signature = message_siganture;	
				
                //functions.addToQueue( joinRecord );
                relayClient.sendRecord(joinRecord);
	
				// TODO: send request or say allready sent. 	
			}

			// Print if pending or allready accepted.
		} else if ( command.find("balance") != std::string::npos ){
			

			//CFunctions functions;
			//std::string privateKey;
			//std::string publicKey;
			//CWallet wallet;
			//bool e = wallet.fileExists("wallet.dat");
			//if(e != 0){
				// Load wallet
				//wallet.read(privateKey, publicKey);
    
				//functions.parseBlockFile(publicKey, false);
                functions.scanChain(publicKey, false);
            
            // Print chain sync status.
            
            
				std::cout << " Your balance: " << functions.balance << " sfr" << std::endl;

			//}
        } else if ( command.find("sent") != std::string::npos ){
			std::cout << "This feature is not implemented yet.\n" << std::endl;
            
        } else if ( command.find("receive") != std::string::npos ){
            
            std::cout << "Your receiving address is: " << publicKey << "\n" << std::endl;
 
        } else if ( command.find("send") != std::string::npos ){
            std::cout << "Enter destination address: \n" << std::endl;
            std::string destination_address;
            std::cin >> destination_address;
            std::cout << "Enter amount to send: \n" << std::endl;
            std::string amount;
            std::cin >> amount;
            std::cout << "Sending " << amount << " to user: " << destination_address << " \n" << std::endl;

            double d_amount = ::atof(amount.c_str());

            // TODO also check balance adjusted using sent requests in queue....
            // TODO check destination address is an accepted user
            if(d_amount > functions.balance ){
                std::cout << "Insuficient balance. Unable to send transfer request. " << std::endl;
            } else {

                CFunctions::record_structure sendRecord;
                time_t  timev;
                time(&timev);
                std::stringstream ss;
                ss << timev;
                std::string ts = ss.str();
                sendRecord.time = ts;
                sendRecord.transaction_type = CFunctions::TRANSFER_CURRENCY;
                sendRecord.amount = d_amount;
                            sendRecord.fee = 0.0;
                sendRecord.sender_public_key = publicKey;
                sendRecord.recipient_public_key = publicKey;
                std::string message_siganture = destination_address;
                ecdsa.SignMessage(privateKey, "" + publicKey, message_siganture);
                sendRecord.signature = message_siganture;
                functions.addToQueue( sendRecord );
                relayClient.sendRecord(sendRecord);
                std::cout << "Sent transfer request. " << std::endl;
            }

        } else if ( command.find("network") != std::string::npos ){

            //functions.parseBlockFile(publicKey, false);
            functions.scanChain(publicKey, false);
            
            std::cout << " Network up to date: " << (functions.IsChainUpToDate() == true ? "yes" : "no ") << std::endl;
            std::cout << " Sync Porgress: " << functions.SyncProgress() << "% " << std::endl;
            
            std::cout << " Joined network: " << (functions.joined > 0 ? "yes" : "no") << std::endl;
            std::cout << " Your balance: " << functions.balance << " sfr" << std::endl;
                    std::cout << " Currency supply: " << functions.currency_circulation << " sfr" << std::endl;
                    std::cout << " User count: " << functions.user_count << std::endl;

            // Block chain size?
            std::cout << " Blockchain size: " << " 0MB" << std::endl;
        
            std::cout << " Pending transactions: " << " 0" << std::endl;
            
            // Active connections?
            //std::cout << "This feature is not implemented yet.\n" << std::endl;
            std::vector<CRelayClient::node_status> peers = relayClient.getPeers();
            std::cout << " Peers: " << peers.size() << std::endl;
            //for(int i = 0; i < peers.size(); i++){ std::cout << peers.at(i).public_key << " "; } std::cout << std::endl;
     
            //CP2P p2p;
            //std::cout << " Peer Address: " << p2p.myPeerAddress << std::endl;
                    //p2p.sendData("DATA DATA DATA 123 \0");

 
        } else if ( command.find("quit") != std::string::npos ){
                running = false;
            
            } else if ( command.find("advanced") != std::string::npos ){
                printAdvancedCommands();
            } else if ( command.find("tests") != std::string::npos ){
                CECDSACrypto ecdsa;
                ecdsa.runTests();



        } else if ( command.compare("chain") == 0){
           std::cout << " Blockchain state: " << " Not implemented. " << std::endl;
	
        } else if ( command.compare("printchain") == 0){
            
            // TODO: Only print issues and the last 5 blocks.
            // Otherwise it's too much data.
            
            std::cout << " Blockchain detail: " << std::endl;

            //functions.parseBlockFile(publicKey, true);
            functions.scanChain(publicKey, true);

            CBlockDB blockDB;
            //blockDB.GetBlocks();
            CFunctions::block_structure last_block = functions.getLastBlock("");
            CFunctions::block_structure block = blockDB.getBlock(last_block.number);
            
            long firstBlockId = blockDB.getFirstBlockId();
            long latestBlockId = blockDB.getLatestBlockId();
            std::cout << "First Block:  " << firstBlockId << std::endl;
            std::cout << "Latest Block: " << latestBlockId << std::endl;
            
            // long CBlockDB::getNextBlockId(long previousBlockId)


        } else if( command.compare("printqueue") == 0){
            std::cout << " Record Queue: " << std::endl;
            //std::cout << "     Not implemented " << std::endl; 
            functions.printQueue();
            
        } else if( command.compare("resetall") == 0){
            std::cout << " Purging node data: " << std::endl;
            functions.DeleteAll();
            
            CBlockDB blockDB;
            blockDB.DeleteAll();
            
            // quit
            running = false;
            
        } else if( command.compare("reindex") == 0){
            std::cout << " Reindex: " << std::endl;
            
            CBlockDB blockDB;
            CUserDB userDB;
            userDB.DeleteIndex();
            
            functions.scanChain(publicKey, true); // debug true to show progress. perhaps add a progress bar/percentage output option later.
           
        } else if(command.compare("requestblock") == 0){
            std::cout << " Request block... " << std::endl;
          
            // What is the latest block this node has.
            CBlockDB blockDB;
            long latestBlockId = blockDB.getLatestBlockId();
            std::cout << " id: " << latestBlockId << std::endl;
            
            // Get connected nodes.
            CRelayClient relayClient;
            relayClient.sendRequestBlocks(latestBlockId);
            
        } else if(command.compare("users") == 0){
            std::cout << "Users: " << std::endl;
            
            CUserDB userDB;
            
            long count = userDB.getUserCount();
            std::cout << "User count: " << count << std::endl;
            
            std::vector<CFunctions::user_structure> users = userDB.getUsers();
            for(int i = 0; i < users.size(); i++){
                CFunctions::user_structure user = users.at(i);
                std::cout << "  user: " << user.public_key << " " << user.balance << " sfr " << std::endl;
            }
            /*
            for(int i = 0; i < functions.users.size(); i++){
                CFunctions::user_structure user = functions.users.at(i);
                std::cout << "  user: " << user.public_key << " " << user.balance << " sfr " << std::endl;
                // if current user publicKey mark
            }
             */
        } else if ( command.compare("vote") == 0){ 
             std::cout << " Block reward (min 0.1 - max 100): " << std::endl;
	
             std::string blockReward;
             std::cin >> blockReward;
             if(blockReward.compare("") == 0 ){
                 blockReward = "1";
             }
           
             // TEMP this record would be added to the queue and broadcast but for testing we just add it to the queue file.

             CFunctions::record_structure voteRecord;
             voteRecord.network = "main"; // networkName;
             time_t  timev;
             time(&timev);
             std::stringstream ss;
             ss << timev;
             std::string ts = ss.str();
             voteRecord.time = ts;
             CFunctions::transaction_types voteType = CFunctions::VOTE;
             voteRecord.transaction_type = voteType;
             voteRecord.name = "blockreward";
             voteRecord.value = blockReward;
             voteRecord.amount = 0.0;
             voteRecord.fee = 0;
             voteRecord.sender_public_key = publicKey;
             voteRecord.recipient_public_key = "";
             
             voteRecord.hash = functions.getRecordHash(voteRecord);
             std::string signature = "";
             ecdsa.SignMessage(privateKey, voteRecord.hash, signature);
             voteRecord.signature = signature;
 
             functions.addToQueue(voteRecord);
	     relayClient.sendRecord(voteRecord); 

		std::cout << "Vote sent." << std::endl;

	} else {
		printCommands();	
	}
    }
}




