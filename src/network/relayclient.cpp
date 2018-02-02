/**
* CRelayClient
*
* Description:
*  CRelayClient implements an interface to a relay server that routs network
*  traffic when CP2P peers are not available.
*
*   http://173.255.218.54/relay.php?action=getnodes&sender_key=109
*   http://173.255.218.54/relay.php?action=getnodes&sender_key=110
*   http://173.255.218.54/relay.php?action=sendmessage&type=trans&sender_key=109&receiver_key=110&message=jsondata
*   http://173.255.218.54/relay.php?action=getmessages&type=block&sender_key=xxx&receiver_key=110
*
*/

#include "relayclient.h"
#include "wallet.h"
#include <sstream>

std::string CRelayClient::myPeerAddress;
bool CRelayClient::running;
bool CRelayClient::connected;
std::string CRelayClient::remotePeerAddress;

std::vector< CRelayClient::node_status > CRelayClient::node_statuses;


CRelayClient::CRelayClient(){
    //std::cout << "CPeerClient " << "\n " << std::endl;
    running = true;
    connected = false;
    controlling = true;
    stun_port = 3478;
    stun_addr = "$(host -4 -t A stun.stunprotocol.org | awk '{ print $4 }')";
}


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


/**
* getNetworkPeer
*
* Description: requests a new peer node connection from a: the current network or b: a connection server.
*/
std::string CRelayClient::getNewNetworkPeer(std::string myPeerAddress){
    std::string readBuffer;
    CURLcode res;
    CURL * curl;
    curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    curl = curl_easy_init();
    if(curl) {
        std::string publicKey;
        std::string privateKey;
        CWallet wallet;
        wallet.read(privateKey, publicKey);

	// http://173.255.218.54/relay.php?action=getnodes&sender_key=109
        std::string url_string = "http://173.255.218.54/relay.php";
        std::string post_data = "action=getnodes&sender_key=";
        post_data.append(publicKey);

        curl_easy_setopt(curl, CURLOPT_URL, url_string.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        // TODO: parse results and write to local peer node file.
        std::size_t open = readBuffer.find("\"public_key\":\"");
	if(open != std::string::npos){
        	std::size_t close = readBuffer.find("\"", open + 15);
        	while(open != std::string::npos && close != std::string::npos){
			std::string key_section = readBuffer.substr( open + 15, close - open -15);
        		//std::cout << "_" << key_section << "_ " << std::endl;
			bool exists = false;
			for(int i = 0; i < node_statuses.size(); i++){
				CRelayClient::node_status node = node_statuses.at(i);
				if(node.public_key.compare(key_section) == 0){
					exists = true;
				}
			}
			if(exists == false){
				CRelayClient::node_status node;
				node.public_key = key_section;
				node_statuses.push_back(node);
			}
			readBuffer = readBuffer.substr( close + 1 );	
			open = readBuffer.find("\"public_key\":\"");
			if(open != std::string::npos){
				close = readBuffer.find("\"", open + 15);		
			}
		}
	}
    }
    return "";
}






/**
* relayNetworkThread
*
* Description: Implements relay networking with other nodes in the network through a server.
*/
void CRelayClient::relayNetworkThread(int argc, char* argv[]){

    std::string publicKey;
    std::string privateKey;
    CWallet wallet;
    wallet.read(privateKey, publicKey);

    while(running){
	//for(int i = 0; i < node_statuses.size(); i++){
        //        CRelayClient::node_status node = node_statuses.at(i);
        //	std::cout << " node " << node.public_key << std::endl;                
	//}

        //std::string response = getNewNetworkPeer(publicKey);
        //std::cout << " relay client  " << response << std::endl;

        if(running){
            usleep(1000000 * 10); // 1 second

        }
    }
    std::cout << "Relay Network Shutdown." << std::endl;
}

void CRelayClient::exit(){
    running = false;
    //std::cout << " P2P exit " << "\n " << std::endl;
}


/**
* sendRecord
*
* Description: send a transaction record to another node
*/
void CRelayClient::sendRecord(CFunctions::record_structure record){


    std::string readBuffer;
    CURLcode res;
    CURL * curl;
    curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    curl = curl_easy_init();
    if(curl) {
        std::string publicKey;
        std::string privateKey;
        CWallet wallet;
        wallet.read(privateKey, publicKey);

        // http://173.255.218.54/relay.php?action=sendmessage&type=trans&sender_key=109&receiver_key=110&message=jsondata 
	std::string url_string = "http://173.255.218.54/relay.php";
        std::string post_data = "action=sendmessage&type=trans&sender_key=";
        post_data.append(record.sender_public_key);


        curl_easy_setopt(curl, CURLOPT_URL, url_string.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        // TODO: parse results and write to local peer node file.
        std::size_t open = readBuffer.find("\"public_key\":\"");
        if(open != std::string::npos){
                std::size_t close = readBuffer.find("\"", open + 15);
                while(open != std::string::npos && close != std::string::npos){
                        std::string key_section = readBuffer.substr( open + 15, close - open -15);
                        //std::cout << "_" << key_section << "_ " << std::endl;
                        bool exists = false;
                        for(int i = 0; i < node_statuses.size(); i++){
                                CRelayClient::node_status node = node_statuses.at(i);
                                if(node.public_key.compare(key_section) == 0){
                                        exists = true;
                                }
                        }
                        if(exists == false){
                                CRelayClient::node_status node;
                                node.public_key = key_section;
                                node_statuses.push_back(node);
                        }
                        readBuffer = readBuffer.substr( close + 1 );
                        open = readBuffer.find("\"public_key\":\"");
                        if(open != std::string::npos){
                                close = readBuffer.find("\"", open + 15);
                        }
                }
        }
    }
}

void CRelayClient::sendBlock(CFunctions::block_structure block){

}
