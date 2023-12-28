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
void GameClient::SendEntityInteractDialogOption(Entity &entity, uint32_t optionId)
{
    if (yj_client->CanSendMessage(MSG_C_ENTITY_INTERACT_DIALOG_OPTION)) {
        Msg_C_EntityInteractDialogOption *msg = (Msg_C_EntityInteractDialogOption *)yj_client->CreateMessage(MSG_C_ENTITY_INTERACT_DIALOG_OPTION);
        if (msg) {
            msg->entity_id = entity.id;
            msg->dialog_id = entity.dialog_id;
            msg->option_id = optionId;
            yj_client->SendMessage(MSG_C_ENTITY_INTERACT_DIALOG_OPTION, msg);
        } else {
            printf("Failed to create ENTITY_INTERACT_DIALOG_OPTION message.\n");
        }
    } else {
        printf("Outgoing ENTITY_INTERACT_DIALOG_OPTION channel message queue is full.\n");
    }
}
void GameClient::SendTileInteract(uint32_t map_id, uint32_t x, uint32_t y, bool primary)
{
    if (yj_client->CanSendMessage(MSG_C_TILE_INTERACT)) {
        Msg_C_TileInteract *msg = (Msg_C_TileInteract *)yj_client->CreateMessage(MSG_C_TILE_INTERACT);
        if (msg) {
            msg->map_id = map_id;
            msg->x = x;
            msg->y = y;
            msg->primary = primary;
            yj_client->SendMessage(MSG_C_TILE_INTERACT, msg);
        } else {
            printf("Failed to create TILE_INTERACT message.\n");
        }
    } else {
        printf("Outgoing TILE_INTERACT channel message queue is full.\n");
    }
}

void GameClient::ProcessMsg(Msg_S_ClockSync &msg)
{
    yojimbo::NetworkInfo netInfo{};
    yj_client->GetNetworkInfo(netInfo);
    const double approxServerNow = msg.serverTime + netInfo.RTT / 2000;
    clientTimeDeltaVsServer = now - approxServerNow;
    world->localPlayerEntityId = msg.playerEntityId;
}
void GameClient::ProcessMsg(Msg_S_EntityDespawn &msg)
{
    entityDb->DestroyEntity(msg.entityId);
}
void GameClient::ProcessMsg(Msg_S_EntitySay &msg)
{
    Entity *entity = entityDb->FindEntity(msg.entity_id);
    if (entity) {
        world->CreateDialog(*entity, msg.dialog_id, msg.title, msg.message, now);
    } else {
        printf("[game_client] Failed to create dialog. Could not find entity id %u.\n", msg.entity_id);
    }
}
void GameClient::ProcessMsg(Msg_S_EntitySnapshot &msg)
{
    Entity *entity = entityDb->FindEntity(msg.entity_id);
    if (entity) {
        GhostSnapshot ghostSnapshot{ msg };
        entity->ghost->push(ghostSnapshot);
    }
}
void GameClient::ProcessMsg(Msg_S_EntitySpawn &msg)
{
    //printf("[ENTITY_SPAWN] id=%u mapId=%u\n", msg->entity_id, msg->map_id);
    Entity *entity = entityDb->FindEntity(msg.entity_id);
    if (!entity) {
        Tilemap *map = world->FindOrLoadMap(msg.map_id);
        assert(map && "why no map? we get chunks before entities, right!?");
        if (map) {
            Entity *entity = entityDb->SpawnEntity(msg.entity_id, msg.type, now);
            if (entity) {
                world->ApplySpawnEvent(msg);
                GhostSnapshot ghostSnapshot{ msg };
                entity->ghost->push(ghostSnapshot);
            }
        } else {
            printf("[game_client] Failed to load map id %u to spawn entity id %u\n", msg.map_id, msg.entity_id);
        }
    } else {
        assert(!"why two spawn events for same entityId??");
    }
}
void GameClient::ProcessMsg(Msg_S_TileChunk &msg)
{
    Tilemap *map = world->FindOrLoadMap(msg.map_id);
    if (map) {
        if (map->id = msg.map_id) {
            printf("Receiving tile chunk, (beeg %d, smol %d)\n", msg.beeg_size, msg.smol_size);

            int block_size = msg.GetBlockSize();
            uint8_t *block = msg.GetBlockData();
            assert(block_size == msg.smol_size);

            for (int i = 0; i < block_size; i++) {
                printf("%x ", block[i]);
            }
            printf("\n");

            int chunk_beeg_bytes{};
            uint16_t *chunk_beeg = (uint16_t *)DecompressData(block, block_size, &chunk_beeg_bytes);
            assert(chunk_beeg_bytes == msg.beeg_size);

            int index = 0;
            for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
                for (uint32_t ty = msg.y; ty < msg.w; ty++) {
                    for (uint32_t tx = msg.x; tx < msg.h; tx++) {
                        map->Set((TileLayerType)layer, msg.x + tx, msg.y + ty, chunk_beeg[index], 0);
                        index++;
                    }
                }
            }

            MemFree(chunk_beeg);
        } else {
            printf("[game_client] Failed to deserialize chunk with mapId %u into map with id %u\n", msg.map_id, map->id);
        }
    } else {
        // TODO: LoadPack the right map by ID somehow
        //if (msg->mapId != world->map.id) {
        //    //world->map.LoadPack(tileChunk.mapName, 0);
        //}
    }
}
void GameClient::ProcessMsg(Msg_S_TileUpdate &msg)
{
    Tilemap *map = world->FindOrLoadMap(msg.map_id);
    if (map) {
        if (map->id = msg.map_id) {
            for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
                map->Set((TileLayerType)layer, msg.x, msg.y, msg.tile_ids[layer], 0);
            }
        } else {
            printf("[game_client] Failed to deserialize chunk with mapId %u into map with id %u\n", msg.map_id, map->id);
        }
    } else {
        // TODO: LoadPack the right map by ID somehow
        //if (msg->mapId != world->map.id) {
        //    //world->map.LoadPack(tileChunk.mapName, 0);
        //}
    }
}
void GameClient::ProcessMsg(Msg_S_TitleShow &msg)
{
    world->title.Show(msg.text, now);
}
void GameClient::ProcessMessages(void)
{
    for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
        yojimbo::Message *yjMsg = yj_client->ReceiveMessage(channelIdx);
        while (yjMsg) {
            if (yjMsg->GetType() != MSG_S_ENTITY_SNAPSHOT) {
                printf("[game_client] RECV msgId=%d msgType=%s", yjMsg->GetId(), MsgTypeStr((MsgType)yjMsg->GetType()));
                if (yjMsg->GetType() == MSG_S_ENTITY_SPAWN) {
                    const auto &spawn = *(Msg_S_EntitySpawn*)yjMsg;
                    printf(" entType=%s", EntityTypeStr(spawn.type));
                }
                printf("\n");
            }
            switch (yjMsg->GetType()) {
                case MSG_S_CLOCK_SYNC:      ProcessMsg(*(Msg_S_ClockSync      *)yjMsg); break;
                case MSG_S_ENTITY_DESPAWN:  ProcessMsg(*(Msg_S_EntityDespawn  *)yjMsg); break;
                case MSG_S_ENTITY_SAY:      ProcessMsg(*(Msg_S_EntitySay      *)yjMsg); break;
                case MSG_S_ENTITY_SNAPSHOT: ProcessMsg(*(Msg_S_EntitySnapshot *)yjMsg); break;
                case MSG_S_ENTITY_SPAWN:    ProcessMsg(*(Msg_S_EntitySpawn    *)yjMsg); break;
                case MSG_S_TILE_CHUNK:      ProcessMsg(*(Msg_S_TileChunk      *)yjMsg); break;
                case MSG_S_TILE_UPDATE:     ProcessMsg(*(Msg_S_TileUpdate     *)yjMsg); break;
                case MSG_S_TITLE_SHOW:      ProcessMsg(*(Msg_S_TitleShow      *)yjMsg); break;
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