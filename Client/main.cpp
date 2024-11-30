#include <iostream>
#include <enet/enet.h>
#include "chat_screen.hpp"
#include <pthread.h>
#include <string>
#include <cstring>
#include <map>




static ChatScreen chatScreen;
static int CLIENT_ID = -1;



class ClientData
{
private:
    int m_id;
    std::string m_username;

public:
    ClientData(int id) : m_id(id){}

    void SetUserName(std::string username) { m_username = username; }

    int GetId(){return m_id;}
    std::string GetUsername(){return m_username;}
};

std::map<int, ClientData*> client_map;


void SendPacket(ENetPeer* peer, const char* data)
{
    ENetPacket* packet = enet_packet_create(data, strlen(data) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);


}



void ParseData(const char* data)
{
    int data_type; // 1, 2, 3...
    int id;
    sscanf(data, "%d|%d", &data_type, &id);

    switch (data_type)
    {
    case 1:
        if(id != CLIENT_ID)
        {
            char msg[80];
            sscanf(data, "%*d|%*d|[^|]", msg);
            chatScreen.ShowChatMessage("client_map[id]->GetUsername().c_str()", msg);
        }

        break;
    case 2:
        //join, set
        if(id != CLIENT_ID)
        {
            char username[80];
            sscanf(data, "%d|%*d|%79[^|]", &username);

            client_map[id] = new ClientData(id);
            client_map[id]->SetUserName(username);
        }
        break;

    case 3:
        CLIENT_ID = id;
        break;
    default:
        break;
    }
}

void* MsgLoop(void* arg)
{
    ENetHost* client = (ENetHost*)arg;

    while(true)
    {
        ENetEvent event;
        while(enet_host_service(client, &event, 0) > 0)
        {
            switch ((event.type))
            {
            case ENET_EVENT_TYPE_RECEIVE: 
               
                ParseData(reinterpret_cast<char*>(event.packet->data));

                enet_packet_destroy(event.packet);
                break;
            
            default:
                break;
            }

        }
 
    }  

    return nullptr; 
}



int main(int argc, char ** argv)
{
    char* username = (char*)malloc(100 * sizeof(char));
    if(!username){
        printf("Memory allcation failed\n");
    }

    printf("Enter user name: ");
    scanf("%99s", username);



    

    if(enet_initialize() != 0)
    {
        fprintf(stderr, "Initializing Error\n");

        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);


    ENetHost* client;
    client = enet_host_create(NULL, 1, 1, 0, 0);

    if(client == NULL)
    {
        fprintf(stderr, "Creating client Error\n");
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;

    enet_address_set_host(&address,"127.0.0.1");

    address.port = 7777;


    peer = enet_host_connect(client, &address, 1, 0);


    if(peer == NULL)
    {
        fprintf(stderr, "No Available peers for initiation an ENet connection");
        return EXIT_FAILURE;
    }

    if ( enet_host_service(client, &event, 5000) > 0  && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("Connection to 127.0.0.1:7777 succeeded");
    }else{
        enet_peer_reset(peer);
        puts("Connection to 127.0.0.1:7777 failed");
        return EXIT_SUCCESS;
    }

    // Send the server the user's name
    char str_data[80] = "2|";
    strcat(str_data, username);
    SendPacket(peer, str_data);
    // GAME LOOP START

    chatScreen.Init();
    
    // Create a thread for receiving data
    pthread_t thread;
    pthread_create(&thread, NULL, MsgLoop, client);

    bool running = true;

    while(running)
    {
            std::string msg = chatScreen.CheckBoxInput();

            chatScreen.ShowChatMessage(username, msg.c_str() );
            
            char message_data[80] = "1|";
            strcat(message_data, msg.c_str());
            SendPacket(peer, message_data);



            if(msg == "/exit") running = false;
    }
    

    // GAME LOOP END
    enet_peer_disconnect(peer, 0);

    pthread_join(thread, NULL);

    

    while(enet_host_service(client, &event, 3000) > 0)
    {
        switch(event.type)
        {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                puts("Disconnection succeeded");
                break;
        }
 
    }
  

 


    return EXIT_SUCCESS;
}