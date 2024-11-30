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

    void SetUserName(std::string username) { m_username = username; }

    int GetId(){return m_id;}
    std::string GetUserName(){return m_username;}
};


void  BroadcastPacket(ENetHost* server, const char* data)
{
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    
    enet_host_broadcast(server, 0, packet); // host , channel, data


}


class ClientManager {
private:
    std::map<int, std::unique_ptr<ClientData>> client_map;
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
        if (peer->data) {
            int id = static_cast<ClientData*>(peer->data)->GetId();
            client_map.erase(id);
            peer->data = NULL;
        }
    }

    // 특정 클라이언트 데이터 가져오기
    ClientData* GetClient(int id) {
        return client_map.count(id) ? client_map[id].get() : nullptr;
    }

    // 모든 클라이언트에 브로드캐스트
    void BroadcastAll(ENetHost* server, const char* message) {
        for (const auto& [id, client] : client_map) {
            BroadcastPacket(server, message);
        }
    }
};


void  SendPacket(ENetPeer* peer, const char* data)
{
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);


}

void ParseData(ENetHost* server, int id, char* data, ClientManager& client_manager)
{
    std::cout<< "PARSE: " << data << std::endl;
    
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
            client_map[id] -> SetUserName(username.c_str());

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
                printf("A new cient connected from %x:%u.\n", 
                    event.peer -> address.host,
                    event.peer -> address.port);
                    for(auto const& x : client_map)
                    {
                        sprintf(send_data, "2|%d|%s", x.first, x.second->GetUserName().c_str());
                        BroadcastPacket(server, send_data);
                    }
                    
                    
                    int id = client_manager.AddClient(event.peer);

                    sprintf(data_to_send, "3|%d", id);
                    SendPacket(event.peer, data_to_send);

                    
                break;
            
            case ENET_EVENT_TYPE_RECEIVE:
                printf ("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
                        event.packet -> dataLength,
                        event.packet -> data,
                        event.peer ->  address.host,
                        event.peer -> address.port,
                        event.channelID);

                        ParseData(server, static_cast<ClientData*>(event.peer->data)->GetId(), (char*)event.packet->data);
                        enet_packet_destroy(event.packet);

                break;
            case ENET_EVENT_TYPE_DISCONNECT:
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

    // GAME LOOP END

    enet_host_destroy(server);

    return EXIT_SUCCESS;
}