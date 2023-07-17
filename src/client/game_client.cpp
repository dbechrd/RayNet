#include "game_client.h"
#include "client_world.h"

void GameClient::Start(void)
{
    if (!InitializeYojimbo()) {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return;
    }
    yojimbo_log_level(CL_YJ_LOG_LEVEL);
    yojimbo_set_printf_function(yj_printf);

    yojimbo::ClientServerConfig config{};
    InitClientServerConfig(config);

    yj_client = new yojimbo::Client(
        yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"),
        config,
        adapter,
        now
    );

#if 0
    char addressString[256];
    yj_client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
}

Err GameClient::TryConnect(void)
{
    if (!yj_client->IsDisconnected()) {
        printf("yj: client already connected, disconnect first\n");
        return RN_SUCCESS;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::Address serverAddress("127.0.0.1", SV_PORT);
    //yojimbo::Address serverAddress("192.168.0.143", SV_PORT);
    //yojimbo::Address serverAddress("slime.theprogrammingjunkie.com", SV_PORT);

    if (!serverAddress.IsValid()) {
        printf("yj: invalid address\n");
        return RN_INVALID_ADDRESS;
    }

    yj_client->InsecureConnect(privateKey, clientId, serverAddress);
    world = new ClientWorld;
    world->camera2d.zoom = 2.0f;
    world->musBackgroundMusic = data::MUS_FILE_AMBIENT_OUTDOORS;
    //world->map.Load(LEVEL_001, now);
    entityDb = new EntityDB;
    return RN_SUCCESS;
}

void GameClient::SendInput(const Controller &controller)
{
    if (yj_client->CanSendMessage(MSG_C_INPUT_COMMANDS)) {
        Msg_C_InputCommands *msg = (Msg_C_InputCommands *)yj_client->CreateMessage(MSG_C_INPUT_COMMANDS);
        if (msg) {
            msg->cmdQueue = controller.cmdQueue;
            yj_client->SendMessage(CHANNEL_U_INPUT_COMMANDS, msg);
        } else {
            printf("Failed to create INPUT_COMMANDS message.\n");
        }
    } else {
        printf("Outgoing INPUT_COMMANDS channel message queue is full.\n");
    }
}

void GameClient::SendEntityInteract(uint32_t entityId)
{
    if (yj_client->CanSendMessage(MSG_C_ENTITY_INTERACT)) {
        Msg_C_EntityInteract *msg = (Msg_C_EntityInteract *)yj_client->CreateMessage(MSG_C_ENTITY_INTERACT);
        if (msg) {
            msg->entityId = entityId;
            yj_client->SendMessage(MSG_C_ENTITY_INTERACT, msg);
        } else {
            printf("Failed to create ENTITY_INTERACT message.\n");
        }
    } else {
        printf("Outgoing ENTITY_INTERACT channel message queue is full.\n");
    }
}

void GameClient::SendTileInteract(uint32_t x, uint32_t y)
{
    if (yj_client->CanSendMessage(MSG_C_TILE_INTERACT)) {
        Msg_C_TileInteract *msg = (Msg_C_TileInteract *)yj_client->CreateMessage(MSG_C_TILE_INTERACT);
        if (msg) {
            msg->x = x;
            msg->y = y;
            yj_client->SendMessage(MSG_C_TILE_INTERACT, msg);
        } else {
            printf("Failed to create TILE_INTERACT message.\n");
        }
    } else {
        printf("Outgoing TILE_INTERACT channel message queue is full.\n");
    }
}

void GameClient::ProcessMessages(void)
{
    for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
        yojimbo::Message *yjMsg = yj_client->ReceiveMessage(channelIdx);
        while (yjMsg) {
            //printf("[game_client] RECV msgId=%d msgType=%s\n", yjMsg->GetId(), MsgTypeStr((MsgType)yjMsg->GetType()));
            switch (yjMsg->GetType()) {
                case MSG_S_CLOCK_SYNC:
                {
                    Msg_S_ClockSync *msg = (Msg_S_ClockSync *)yjMsg;
                    yojimbo::NetworkInfo netInfo{};
                    yj_client->GetNetworkInfo(netInfo);
                    const double approxServerNow = msg->serverTime + netInfo.RTT / 2000;
                    clientTimeDeltaVsServer = now - approxServerNow;
                    world->localPlayerEntityId = msg->playerEntityId;
                    break;
                }
                case MSG_S_ENTITY_DESPAWN:
                {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yjMsg;
                    entityDb->DestroyEntity(msg->entityId);
                    break;
                }
                case MSG_S_ENTITY_DESPAWN_TEST:
                {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yjMsg;
                    printf("[game_client] ENTITY_DESPAWN_TEST entityId=%u\n", msg->entityId);
                    break;
                }
                case MSG_S_ENTITY_SAY:
                {
                    Msg_S_EntitySay *msg = (Msg_S_EntitySay *)yjMsg;
                    world->CreateDialog(msg->entityId, msg->message, now);
                    break;
                }
                case MSG_S_ENTITY_SNAPSHOT:
                {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yjMsg;
                    size_t entityIndex = entityDb->FindEntityIndex(msg->entityId);
                    if (entityIndex) {
                        AspectGhost &ghost = entityDb->ghosts[entityIndex];
                        GhostSnapshot ghostSnapshot{ *msg };
                        ghost.push(ghostSnapshot);
                    }
                    break;
                }
                case MSG_S_ENTITY_SPAWN:
                {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yjMsg;
                    //printf("[ENTITY_SPAWN] id=%u mapId=%u\n", msg->entityId, msg->mapId);
                    data::Entity *entity = entityDb->FindEntity(msg->entityId);
                    if (!entity) {
                        Tilemap *map = world->FindOrLoadMap(msg->mapId);
                        assert(map && "why no map? we get chunks before entities, right!?");
                        if (map) {
                            if (entityDb->SpawnEntity(msg->entityId, msg->type, now)) {
                                world->ApplySpawnEvent(*msg);
                            }
                        } else {
                            printf("[game_client] Failed to load map id %u to spawn entity id %u\n", msg->mapId, msg->entityId);
                        }
                    } else {
                        assert(!"why two spawn events for same entityId??");
                    }
                    break;
                }
                case MSG_S_TILE_CHUNK:
                {
                    Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yjMsg;

                    Tilemap *map = world->FindOrLoadMap(msg->mapId);
                    if (map) {
                        map->CL_DeserializeChunk(*msg);
                    } else {
                        // TODO: Load the right map by ID somehow
                        //if (msg->mapId != world->map.id) {
                        //    //world->map.Load(tileChunk.mapName, 0);
                        //}
                    }
                    break;
                }
            }
            yj_client->ReleaseMessage(yjMsg);
            yjMsg = yj_client->ReceiveMessage(channelIdx);
        };
    }
}

void GameClient::Update(void)
{
    // NOTE(dlb): This sends keepalive packets
    yj_client->AdvanceTime(now);

    // TODO(dlb): Is it a good idea / necessary to receive packets every frame,
    // rather than at a fixed rate? It seems like it is... right!?
    yj_client->ReceivePackets();
    if (!yj_client->IsConnected()) {
        return;
    }

    ProcessMessages();

    // Sample accumulator once per server tick and push command into command queue
    if (controller.sampleInputAccum >= CL_SAMPLE_INPUT_DT) {
        controller.cmdAccum.seq = ++controller.nextSeq;
        controller.cmdAccum.sampledAt = now;
        controller.cmdQueue.push(controller.cmdAccum);
        controller.cmdAccum = {};
        controller.lastInputSampleAt = now;
        controller.sampleInputAccum -= CL_SAMPLE_INPUT_DT;
    }

    // Send rolled up input at fixed interval
    if (now - controller.lastCommandSentAt >= CL_SEND_INPUT_DT) {
        SendInput(controller);
        controller.lastCommandSentAt = now;
    }

    yj_client->SendPackets();
}

void GameClient::Stop(void)
{
    yj_client->Disconnect();
    clientTimeDeltaVsServer = 0;
    netTickAccum = 0;
    lastNetTick = 0;
    now = 0;

    controller = {};
    delete world;
    world = {};
    delete entityDb;
    entityDb = {};
}