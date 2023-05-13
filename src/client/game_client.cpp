#include "game_client.h"
#include "client_world.h"
#include "../common/audio/audio.h"

uint32_t GameClient::LocalPlayerEntityId(void) {
    if (yj_client->IsConnected()) {
        // TODO(dlb)[cleanup]: Yikes.. the server should send us our entityId when we connect
        return yj_client->GetClientIndex() + 1;
    }
    return 0;
}

Entity *GameClient::LocalPlayer(void) {
    if (yj_client->IsConnected()) {
        Entity *localPlayer = world->GetEntity(LocalPlayerEntityId());
        if (localPlayer && localPlayer->type == Entity_Player) {
            return localPlayer;
        }
    }
    return 0;
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

    //yojimbo::Address serverAddress("127.0.0.1", SV_PORT);
    //yojimbo::Address serverAddress("192.168.0.143", SV_PORT);
    yojimbo::Address serverAddress("68.9.219.64", SV_PORT);
    //yojimbo::Address serverAddress("slime.theprogrammingjunkie.com", SV_PORT);

    if (!serverAddress.IsValid()) {
        printf("yj: invalid address\n");
        return RN_INVALID_ADDRESS;
    }

    yj_client->InsecureConnect(privateKey, clientId, serverAddress);
    world = new ClientWorld;
    world->camera2d.zoom = 2.0f;
    //world->map.Load(LEVEL_001, now);
    return RN_SUCCESS;
}

void GameClient::Start(void)
{
    if (!InitializeYojimbo()) {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);

    yojimbo::ClientServerConfig config{};
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_R_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_ENTITY_EVENT].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.bandwidthSmoothingFactor = CL_BANDWIDTH_SMOOTHING_FACTOR;

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
            switch (yjMsg->GetType()) {
                case MSG_S_CLOCK_SYNC:
                {
                    Msg_S_ClockSync *msg = (Msg_S_ClockSync *)yjMsg;
                    yojimbo::NetworkInfo netInfo{};
                    yj_client->GetNetworkInfo(netInfo);
                    const double approxServerNow = msg->serverTime + netInfo.RTT / 2000;
                    clientTimeDeltaVsServer = now - approxServerNow;
                    break;
                }
                case MSG_S_ENTITY_DESPAWN:
                {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yjMsg;
                    Entity *entity = world->GetEntity(msg->entityId);
                    if (entity) {
                        world->DestroyDialog(entity->latestDialog);
                        *entity = {};
                        EntityGhost &ghost = world->ghosts[msg->entityId];
                        ghost.snapshots = {};
                    }
                    break;
                }
                case MSG_S_ENTITY_SAY:
                {
                    Msg_S_EntitySay *msg = (Msg_S_EntitySay *)yjMsg;
                    if (msg->messageLength > 0 && msg->messageLength <= SV_MAX_ENTITY_SAY_MSG_LEN) {
                        world->CreateDialog(
                            msg->entityId,
                            msg->messageLength,
                            msg->message,
                            now
                        );
                    } else {
                        printf("Wtf dis server smokin'? Sent entity say message of length %u but max is %u\n",
                            msg->messageLength,
                            SV_MAX_ENTITY_SAY_MSG_LEN);
                    }
                    break;
                }
                case MSG_S_ENTITY_SNAPSHOT:
                {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yjMsg;
                    EntityGhost &ghost = world->ghosts[msg->entitySnapshot.id];
                    ghost.snapshots.push(msg->entitySnapshot);
                    break;
                }
                case MSG_S_ENTITY_SPAWN:
                {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yjMsg;
                    uint32_t entityId = msg->entitySpawnEvent.id;
                    Entity *entity = world->GetEntityDeadOrAlive(msg->entitySpawnEvent.id);
                    if (entity) {
                        // WARN(dlb): Other things could refer to a previous version of this entityId.
                        // TODO(dlb): Generations?
                        *entity = {};
                        entity->ApplySpawnEvent(msg->entitySpawnEvent);
                        if (entity->type == Entity_Projectile) {
                            rnSoundSystem.Play(RN_Sound_Tick_Soft);
                        }
                    }
                    break;
                }
                case MSG_S_TILE_CHUNK:
                {
                    Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yjMsg;
                    world->map.CL_DeserializeChunk(*msg);
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
}