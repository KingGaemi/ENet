#include <iostream>
#include <enet/enet.h>
#include <stdio.h>
#include <map>
#include <mutex>
#include <memory>

class ClientData
{
private:
    int m_id;
    std::string m_username;

public:
    ClientData(int id) : m_id(id){}

    void SetUsername(std::string username) { m_username = username; }

    int GetId(){return m_id;}
    std::string GetUserName(){return m_username;}
};


void BroadcastPacket(ENetHost* server, const char* data) {
    // Null 데이터 확인
    if (!data) {
        std::cerr << "BroadcastPacket: Null data provided!" << std::endl;
        return;
    }

    // 데이터 크기 확인
    size_t data_size = strlen(data);
    if (data_size > ENET_PROTOCOL_MAXIMUM_WINDOW_SIZE) {
        std::cerr << "BroadcastPacket: Data size exceeds maximum packet size!" << std::endl;
        return;
    }

    // 피어 연결 여부 확인
    if (server->connectedPeers == 0) {
        std::cerr << "BroadcastPacket: No connected peers to broadcast to!" << std::endl;
        return;
    }

    // 패킷 생성 및 브로드캐스트
    ENetPacket* packet = enet_packet_create(data, data_size + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);

    // 패킷 전송 완료 후 로그 출력 (디버깅용)
    // std::cout << "Broadcasted Packet: " << data << std::endl;
}

class ClientManager {
private:
    std::map<int, std::unique_ptr<ClientData>> client_map;
    std::mutex client_map_mutex;
    int next_id = 0;

public:
    // 클라이언트 추가
    int AddClient(ENetPeer* peer) {
        next_id++;
        client_map[next_id] = std::make_unique<ClientData>(next_id);
        peer->data = client_map[next_id].get();
        return next_id;
    }

    // 클라이언트 삭제
    void RemoveClient(ENetPeer* peer) {
        if (!peer->data) {
            std::cerr << "RemoveClient: peer->data is NULL. Nothing to remove." << std::endl;
            return;
        }
        
        std::lock_guard<std::mutex> lock(client_map_mutex);

        auto* client_data = static_cast<ClientData*>(peer->data);
        if (!client_data) {
            std::cerr << "RemoveClient: peer->data does not point to valid ClientData." << std::endl;
            return;
        }

        int id = client_data->GetId();
        if (client_map.erase(id) == 0) {
            std::cerr << "RemoveClient: Client ID " << id << " not found in client_map." << std::endl;
        } else {
            std::cout << "RemoveClient: Client ID " << id << " removed successfully." << std::endl;
        }

        delete client_data; // 메모리 해제
        peer->data = NULL;  // 포인터 초기화
    }


    // GetClients
    const std::map<int, std::unique_ptr<ClientData>>& GetClients() const {
        return client_map;
    }


    // 특정 클라이언트 데이터 가져오기
    ClientData* GetClientData(int id) {
        return client_map.count(id) ? client_map[id].get() : nullptr;
    }

  

    // 모든 클라이언트에 브로드캐스트
    void BroadcastAll(ENetHost* server, const char* message) {
        for (const auto& [id, client] : client_map) {
            if (client) {  // 유효한 클라이언트인지 확인
                BroadcastPacket(server, message);
            }
        }
    }
};


void  SendPacket(ENetPeer* peer, const char* data)
{
    // std::cout << "SendPacket: ";
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    // std::cout << packet->data << std::endl;
    enet_peer_send(peer, 0, packet);


}

void ParseData(ENetHost* server,const int& id, char* data, ClientManager& client_manager)
{
    std::cout<< "PARSE: " << data << "id: " << id << std::endl;
    
    int data_type;
    sscanf(data, "%d|", &data_type);
    
    char send_data[1024] = {'\0'};

    switch(data_type)
    {
        case 1:
        {

            std::string msg;
            msg.resize(80);
            sscanf(data, "%*d|%79[^\n]", &msg[0]);
            msg.resize(strlen(msg.c_str())); // 정확한 길이로 축소


            
            sprintf(send_data, "1|%d|%s", id, msg.c_str());
            std::cout << "Send Data: " << send_data << std::endl;
            BroadcastPacket(server, send_data);
            break;
        }

        case 2:
        {
            std::string username;
            username.resize(80);

            
            if (sscanf(data, "2|%79[^\n]", &username[0]) != 1) 
            {
                std::cerr << "Failed to parse username from: " << data << std::endl;
                return;
            }

            
            sprintf(send_data, "2|%d|%s", id, username.c_str());
            

            BroadcastPacket(server, send_data);
            client_manager.GetClientData(id) -> SetUsername(username.c_str());
            

            break;
         
        }
       
        default:
            break;
    }
}





int main(int argc, char **argv)
{

    ClientManager client_manager;
    if(enet_initialize() != 0)
    {
        fprintf(stderr, "Initializing Error\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    ENetEvent event;
    ENetHost* server;


    address.host = ENET_HOST_ANY;
    address.port = 7777;

    
    // address, maxPlayer(peers), channels, bandwith Limit (0 is infinit)
    server = enet_host_create(&address, 32, 2, 0, 0);

    if( server == NULL){
        fprintf(stderr, "Server host creating Error");
        return EXIT_FAILURE;
    }


    // GAME LOOP START
    int new_player_id = 0;

    char send_data[1024] = {'\0'}; // 루프 외부 선언
    char data_to_send[126] = {'\0'};
    char disconnect_data[126] = {'\0'};

    while(true)
    {
        while(enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
                //New Connection
            case ENET_EVENT_TYPE_CONNECT:
                {
                    printf("A new cient connected from %x:%u.\n", 
                        event.peer -> address.host,
                        event.peer -> address.port);

                        for(auto const& x : client_manager.GetClients())
                        {
                            sprintf(send_data, "2|%d|%s", x.first, x.second->GetUserName().c_str());
                            BroadcastPacket(server, send_data);
                        }
                        
                        
                        int id = client_manager.AddClient(event.peer);

                        sprintf(data_to_send, "3|%d", id);
                        SendPacket(event.peer, data_to_send);

                        
                    break;
                }
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    printf ("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
                            event.packet -> dataLength,
                            event.packet -> data,
                            event.peer ->  address.host,
                            event.peer -> address.port,
                            event.channelID);

                            ParseData(server, static_cast<ClientData*>(event.peer->data)->GetId(), (char*)event.packet->data, client_manager);
                            enet_packet_destroy(event.packet);
                    break;
                }
            case ENET_EVENT_TYPE_DISCONNECT:
                {
                    printf("%x:%u Disconnected\n",
                        event.peer->address.host,
                        event.peer->address.port);

                    
                    sprintf(disconnect_data, "4|%d", static_cast<ClientData*>(event.peer ->data)->GetId());
                    BroadcastPacket(server, disconnect_data);

                    client_manager.RemoveClient(event.peer);    
                    break;
                }
            }
        }
    }

    // GAME LOOP END

    enet_host_destroy(server);

    return EXIT_SUCCESS;
}