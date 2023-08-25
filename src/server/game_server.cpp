#include "game_server.h"
#include "../common/net/messages/msg_s_entity_spawn.h"
#include "../common/net/messages/msg_s_entity_snapshot.h"
#include "../common/tilemap.h"

Rectangle lastCollisionA{};
Rectangle lastCollisionB{};

GameServerNetAdapter::GameServerNetAdapter(GameServer *server)
{
    this->server = server;
}

void GameServerNetAdapter::OnServerClientConnected(int clientIdx)
{
    server->OnClientJoin(clientIdx);
}

void GameServerNetAdapter::OnServerClientDisconnected(int clientIdx)
{
    server->OnClientLeave(clientIdx);
}

GameServer::~GameServer(void)
{
    for (Tilemap *map : maps) {
        delete map;
    }
}

void GameServer::OnClientJoin(int clientIdx)
{
    static const Color playerColors[]{
        MAROON,
        LIME,
        SKYBLUE
    };

    // TODO: Huh? What dis mean?
    if (!maps.size()) {
        return;
    }

    data::Entity *player = SpawnEntity(data::ENTITY_PLAYER);
    if (player) {
        ServerPlayer &sv_player = players[clientIdx];
        sv_player.needsClockSync = true;
        sv_player.joinedAt = now;
        sv_player.entityId = player->id;

        player->type = data::ENTITY_PLAYER;
        player->map_id = maps[0]->id;
        const Vector2 caveEntrance{ 1650, 435 };
        const Vector2 townCenter{ 830, 1180 };
        player->position = townCenter;
        player->radius = 10;
        player->hp_max = 100;
        player->hp = player->hp_max;
        player->speed = 1000;
        player->drag = 1.0f;
        player->sprite = data::SPRITE_CHR_MAGE;
        //projectile->direction = data::DIR_E;  // what's it do if it defaults to North?

        TileChunkRecord mainMap{};
        sv_player.chunkList.push(mainMap);

        BroadcastEntitySpawn(player->id);
    } else {
        assert(!"world full? hmm.. need to disconnect client somehow");
    }
}
void GameServer::OnClientLeave(int clientIdx)
{
    // TODO: Huh? What dis mean?
    if (!maps.size()) return;

    // TODO: Which map is this sv_player currently on? Need to despawn them from that map.
    Tilemap &map = *maps[0];

    ServerPlayer &serverPlayer = players[clientIdx];
    DespawnEntity(serverPlayer.entityId);
    serverPlayer = {};
}

uint32_t GameServer::GetPlayerEntityId(uint32_t clientIdx)
{
    return clientIdx + 1;
}

Tilemap *GameServer::FindOrLoadMap(std::string filename)
{
    const auto &mapEntry = mapsByName.find(filename);
    if (mapEntry != mapsByName.end()) {
        size_t mapIndex = mapEntry->second;
        return maps[mapIndex];
    } else {
        Err err = RN_SUCCESS;
        Tilemap *map = new Tilemap();
        do {
            if (!map) {
                printf("Failed to load map %s with code %d\n", filename.c_str(), err);
                err = RN_BAD_ALLOC;
                break;
            }

            err = map->Load(filename);
            if (err) break;

            map->id = nextMapId++;
            map->chunkLastUpdatedAt = now;
            mapsById[map->id] = maps.size();
            mapsByName[map->name] = maps.size();
            maps.push_back(map);
            return map;
        } while (0);

        if (err) {
            assert(!"failed to load map, what to do here?");
            printf("Failed to load map %s with code %d\n", filename.c_str(), err);
        }
        return 0;
    }
}
Err GameServer::Start(void)
{
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
    if (!InitializeYojimbo()) {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return RN_NET_INIT_FAILED;
    }
    yojimbo_log_level(SV_YJ_LOG_LEVEL);
    yojimbo_set_printf_function(yj_printf);

#if 0
    if (argc == 2)
    {
        Address commandLineAddress(argv[1]);
        if (commandLineAddress.IsValid())
        {
            if (commandLineAddress.GetPort() == 0)
                commandLineAddress.SetPort(ServerPort);
            serverAddress = commandLineAddress;
        }
    }
#endif

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    yojimbo::ClientServerConfig config{};
    InitClientServerConfig(config);

    // Loopback interface
    yojimbo::Address address("127.0.0.1", SV_PORT);
    //yojimbo::Address address("::1", SV_PORT);

    // Any interface
    //yojimbo::Address address("0.0.0.0", SV_PORT);
    //yojimbo::Address address("::", SV_PORT);

    //yojimbo::Address address("192.168.0.143", SV_PORT);
    //yojimbo::Address address("slime.theprogrammingjunkie.com", SV_PORT);

    if (!address.IsValid()) {
        printf("yj: Invalid address\n");
        return RN_INVALID_ADDRESS;
    }
    yj_server = new yojimbo::Server(
        yojimbo::GetDefaultAllocator(),
        privateKey,
        address,
        config,
        adapter,
        GetTime()
    );

    // NOTE(dlb): This must be the same size as world->players[] array!
    yj_server->Start(SV_MAX_PLAYERS);
    if (!yj_server->IsRunning()) {
        printf("yj: Failed to start server\n");
        return RN_NET_INIT_FAILED;
    }
#if 0
    printf("yj: started server on port %d (insecure)\n", SV_PORT);
    char addressString[256];
    GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: server address is %s\n", addressString);
#endif

    entityDb = new EntityDB();

    return RN_SUCCESS;
}

Tilemap *GameServer::FindMap(uint32_t mapId)
{
    const auto &mapEntry = mapsById.find(mapId);
    if (mapEntry != mapsById.end()) {
        return maps[mapEntry->second];
    }
    return 0;
}

data::Entity *GameServer::SpawnEntity(data::EntityType type)
{
    uint32_t entityId = nextEntityId++;
    data::Entity *entity = entityDb->SpawnEntity(entityId, type, now);
    return entity;
}
void GameServer::DespawnEntity(uint32_t entityId)
{
    if (entityDb->DespawnEntity(entityId, now)) {
        BroadcastEntityDespawn(entityId);
    }
}
void GameServer::DestroyDespawnedEntities(void)
{
    for (data::Entity &entity : entityDb->entities) {
        if (entity.type && entity.despawned_at) {
            assert(entity.id);
            entityDb->DestroyEntity(entity.id);
        }
    }
}

void GameServer::SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn)
{
    assert(entityId);
    if (!entityId) return;

    data::Entity *entity = entityDb->FindEntity(entityId);
    assert(entity);
    if (!entity) return;

    entitySpawn.server_time = lastTickedAt;

    // Entity
    entitySpawn.entity_id = entity->id;
    entitySpawn.type      = entity->type;
    entitySpawn.spec      = entity->spec;
    strncpy(entitySpawn.name, entity->name.c_str(), SV_MAX_ENTITY_NAME_LEN);
    entitySpawn.map_id    = entity->map_id;
    entitySpawn.position  = entity->position;

    // Collision
    entitySpawn.radius    = entity->radius;

    // Life
    entitySpawn.hp_max    = entity->hp_max;
    entitySpawn.hp        = entity->hp;

    // Physics
    entitySpawn.drag      = entity->drag;
    entitySpawn.speed     = entity->speed;
    entitySpawn.velocity  = entity->velocity;

    // Sprite
    entitySpawn.sprite    = entity->sprite;
}
void GameServer::SendEntitySpawn(int clientIdx, uint32_t entityId)
{
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity) {
        printf("[game_server] could not find entity id %u. cannot send entity spawn msg.\n", entityId);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
        if (msg) {
            SerializeSpawn(entityId, *msg);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntitySpawn(uint32_t entityId)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }
        ServerPlayer &serverPlayer = players[clientIdx];
        if (serverPlayer.joinedAt == now) {
            // they'll be receiving a full snapshot this frame
            continue;
        }

        SendEntitySpawn(clientIdx, entityId);
    }
}

void GameServer::SendEntityDespawn(int clientIdx, uint32_t entityId)
{
    data::Entity *entity = entityDb->FindEntity(entityId, true);
    if (!entity) {
        printf("[game_server] could not find entity id %u. cannot send entity despawn msg.\n", entityId);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
        if (msg) {
            msg->entityId = entityId;
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntityDespawn(uint32_t entityId)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }
        ServerPlayer &serverPlayer = players[clientIdx];
        if (serverPlayer.joinedAt == now) {
            // they'll be receiving a full snapshot this frame
            continue;
        }

        SendEntityDespawn(clientIdx, entityId);
    }
}

void GameServer::SendEntityDespawnTest(int clientIdx, uint32_t testId)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN_TEST);
        if (msg) {
            msg->entityId = testId;
            printf("[game_server][client %d] ENTITY_DESPAWN_TEST testId=%u\n", clientIdx, msg->entityId);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntityDespawnTest(uint32_t testId)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntityDespawnTest(clientIdx, testId);
    }
}

void GameServer::SendEntitySay(int clientIdx, uint32_t entityId, uint32_t dialogId, std::string message)
{
    // TODO: Send only if the client is nearby, or the message is a global event
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity) {
        printf("[game_server] could not find entity id %u. cannot send entity say msg.\n", entityId);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntitySay *msg = (Msg_S_EntitySay *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SAY);
        if (msg) {
            msg->entity_id = entityId;
            msg->dialog_id = dialogId;
            strncpy(msg->message, message.c_str(), SV_MAX_ENTITY_SAY_MSG_LEN);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntitySay(uint32_t entityId, std::string message)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntitySay(clientIdx, entityId, 0, message);
    }
}

void GameServer::SendTileChunk(int clientIdx, Tilemap &map, uint32_t x, uint32_t y)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_TILE_EVENT)) {
        Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yj_server->CreateMessage(clientIdx, MSG_S_TILE_CHUNK);
        if (msg) {
            map.SV_SerializeChunk(*msg, x, y);
            yj_server->SendMessage(clientIdx, CHANNEL_R_TILE_EVENT, msg);
        }
    }
}
void GameServer::BroadcastTileChunk(Tilemap &map, uint32_t x, uint32_t y)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendTileChunk(clientIdx, map, x, y);
    }
}

void GameServer::RequestDialog(int clientIdx, data::Entity &entity, Dialog &dialog)
{
    // Overridable by listeners
    uint32_t final_dialog_id = dialog.id;

    DialogListener listener = dialog_library.FindListenerByKey(dialog.key);
    if (listener) {
        ServerPlayer &player = players[clientIdx];
        final_dialog_id = listener(player.entityId, entity.id, dialog.id);
    }

    if (final_dialog_id) {
        std::string_view msg = dialog.msg;
        if (final_dialog_id != dialog.id) {
            Dialog *final_dialog = dialog_library.FindById(final_dialog_id);
            if (final_dialog) {
                msg = final_dialog->msg;
            } else {
                assert(!"wtf, listener redirected us to a bad dialog id?");
                // something bad happened in listener, just show the original
                // dialog i guess?
                final_dialog_id = final_dialog->id;
            }
        }
        SendEntitySay(clientIdx, entity.id, final_dialog_id, std::string(msg));
        entity.dialog_spawned_at = now;
    }
}

void GameServer::ProcessMessages(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
            yojimbo::Message *yjMsg = yj_server->ReceiveMessage(clientIdx, channelIdx);
            while (yjMsg) {
                switch (yjMsg->GetType()) {
                    case MSG_C_ENTITY_INTERACT:
                    {
                        Msg_C_EntityInteract *msg = (Msg_C_EntityInteract *)yjMsg;
                        ServerPlayer &player = players[clientIdx];

                        // TODO: Check if sv_player is allowed to actually interact with this
                        // particular entity. E.g. are they even in the same map as them!?
                        // Proximity, etc.
                        data::Entity *entity = entityDb->FindEntity(msg->entityId, data::ENTITY_NPC);
                        if (entity) {
                            Dialog *dialog = dialog_library.FindByKey(entity->dialog_root_key);
                            if (dialog) {
                                RequestDialog(clientIdx, *entity, *dialog);
                            }
                        }
                        break;
                    }
                    case MSG_C_ENTITY_INTERACT_DIALOG_OPTION:
                    {
                        Msg_C_EntityInteractDialogOption *msg = (Msg_C_EntityInteractDialogOption *)yjMsg;
                        ServerPlayer &player = players[clientIdx];

                        Dialog *prevDialog = dialog_library.FindById(msg->dialog_id);
                        if (!prevDialog) {
                            // Client being stupid?
                            assert(!"invalid dialog id");
                            break;
                        }

                        if (msg->option_id >= prevDialog->nodes.size()) {
                            // Client being stupid?
                            assert(!"invalid dialog option id (out of bounds)");
                            break;
                        }

                        // TODO: Check if player is allowed to actually interact with this
                        // particular entity. E.g. are they even in the same map as them!?
                        // Proximity, etc. If they leave proximity, send EntityInteractCancel
                        data::Entity *entity = entityDb->FindEntity(msg->entity_id, data::ENTITY_NPC);
                        if (entity) {
                            DialogNode &optionTag = prevDialog->nodes[msg->option_id];

                            if (optionTag.type != DIALOG_NODE_LINK) {
                                // Client being stupid?
                                assert(!"invalid dialog option id (not a link)");
                                break;
                            }

                            std::string_view nextDialogKey = optionTag.data;
                            Dialog *nextDialog = dialog_library.FindByKey(nextDialogKey);
                            if (!nextDialog) {
                                // Client being stupid? (or bug in data where msg string has more options than options id array)
                                assert(!"missing dialog option id?");
                                break;
                            }

                            RequestDialog(clientIdx, *entity, *nextDialog);
                        }
                        break;
                    }
                    case MSG_C_INPUT_COMMANDS:
                    {
                        Msg_C_InputCommands *msg = (Msg_C_InputCommands *)yjMsg;
                        players[clientIdx].inputQueue = msg->cmdQueue;
                        break;
                    }
                    case MSG_C_TILE_INTERACT:
                    {
                        Msg_C_TileInteract *msg = (Msg_C_TileInteract *)yjMsg;

                        // TODO: Check if sv_player is allowed to actually interact with this
                        // particular tile. E.g. are they even in the same map as it!?
                        // Holding the right tool, proximity, etc.
                        Tilemap *map = FindMap(msg->map_id);
                        Tile tile{};
                        if (map && map->AtTry(msg->x, msg->y, tile)) {
                            //map.Set(msg->x, msg->y, 0);
                        }
                        break;
                    }
                }
                yj_server->ReleaseMessage(clientIdx, yjMsg);
                yjMsg = yj_server->ReceiveMessage(clientIdx, channelIdx);
            }
        }
    }
}

data::Entity *GameServer::SpawnProjectile(uint32_t map_id, Vector2 position, Vector2 direction)
{
    data::Entity *projectile = SpawnEntity(data::ENTITY_PROJECTILE);
    if (!projectile) return 0;

    // [Entity] position
    projectile->position = position;
    projectile->map_id = map_id;

    // [Physics] shoot in facing direction
    Vector2 dir = Vector2Scale(direction, 100);
    // [Physics] add a bit of random spread
    dir.x += GetRandomValue(-20, 20);
    dir.y += GetRandomValue(-20, 20);
    dir = Vector2Normalize(dir);
    Vector2 velocity = Vector2Scale(dir, 200); //GetRandomValue(800, 1000));;
    // [Physics] random speed
    projectile->velocity = velocity;
    projectile->drag = 0.02f;

    projectile->sprite = data::SPRITE_PRJ_FIREBALL;
    //projectile->direction = data::DIR_E;

    BroadcastEntitySpawn(projectile->id);
    return projectile;
}
void GameServer::UpdateServerPlayers(void)
{
    for (ServerPlayer &sv_player : players) {
        const InputCmd *input_cmd = 0;
        for (int i = 0; i < sv_player.inputQueue.size(); i++) {
            const InputCmd &cmd = sv_player.inputQueue[i];
            if (cmd.seq > sv_player.lastInputSeq) {
                input_cmd = &cmd;
                sv_player.lastInputSeq = input_cmd->seq;
                //printf("Processed command %d\n", (int32_t)tick - (int32_t)input_cmd->seq);
                break;
            }
        }

        if (input_cmd) {
            data::Entity *player = entityDb->FindEntity(sv_player.entityId, data::ENTITY_PLAYER);
            if (player) {
                Vector2 move_force = input_cmd->GenerateMoveForce(player->speed);
                player->ApplyForce(move_force);

                if (input_cmd->fire) {
                    if (now - player->last_attacked_at > player->attack_cooldown) {
                        Vector2 projectile_spawn_pos = player->position;
                        projectile_spawn_pos.y -= 24;
                        data::Entity *projectile = SpawnProjectile(player->map_id, projectile_spawn_pos, input_cmd->facing);
                        if (projectile) {
                            Vector2 recoil_force = Vector2Negate(projectile->velocity);
                            player->ApplyForce(recoil_force);
                        }
                        player->last_attacked_at = now;
                        player->attack_cooldown = 0.3;
                    }
                }
            } else {
                assert(0);
                printf("[game_server] Player entityId %u out of range. Cannot process inputCmd.\n", sv_player.entityId);
            }
        }
    }
}

data::Entity *SpawnEntityProto(GameServer &server, uint32_t mapId, Vector2 position, data::EntityProto &proto)
{
    Tilemap *map = server.FindMap(mapId);
    if (!map) return 0;

    data::Entity *entity = server.SpawnEntity(data::ENTITY_NPC);
    if (!entity) return 0;

    entity->spec                 = proto.spec;
    entity->map_id               = mapId;
    entity->name                 = proto.name;
    entity->position             = position;
    entity->ambient_fx           = proto.ambient_fx;
    entity->ambient_fx_delay_min = proto.ambient_fx_delay_min;
    entity->ambient_fx_delay_max = proto.ambient_fx_delay_max;
    entity->radius               = proto.radius;
    entity->dialog_root_key      = proto.dialog_root_key;
    entity->hp_max               = proto.hp_max;
    entity->hp                   = entity->hp_max;
    entity->path_id              = proto.path_id;  // TODO: Give the path a name, e.g. "PATH_TOWN_LILY"
    entity->drag                 = proto.drag;
    entity->speed                = GetRandomValue(proto.speed_min, proto.speed_max);
    entity->sprite               = proto.sprite;

    AiPathNode *aiPathNode = map->GetPathNode(entity->path_id, 0);
    if (aiPathNode) {
        entity->position = aiPathNode->pos;
    }

    return entity;
}
void GameServer::TickSpawnTownNPCs(uint32_t mapId)
{
    static data::EntityProto lily;
    static data::EntityProto freye;
    static data::EntityProto nessa;
    static data::EntityProto elane;
    static data::EntityProto *townfolk[]{
        &lily, &freye, &nessa, &elane
    };
    static uint32_t townfolk_ids[4];

    static data::EntityProto chicken;
    static uint32_t chicken_ids[10];

    if (!lily.type) {
        lily.type = data::ENTITY_NPC;
        lily.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        lily.name = "Lily";
        lily.radius = 10;
        lily.dialog_root_key = "DIALOG_LILY_INTRO";
        lily.hp_max = 777;
        lily.path_id = 0;
        lily.speed_min = 300;
        lily.speed_max = 600;
        lily.drag = 1.0f;
        lily.sprite = data::SPRITE_NPC_LILY;

        freye.type = data::ENTITY_NPC;
        freye.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        freye.name = "Freye";
        freye.radius = 10;
        freye.dialog_root_key = "DIALOG_FREYE_INTRO";
        freye.hp_max = 200;
        freye.path_id = 0;
        freye.speed_min = 300;
        freye.speed_max = 600;
        freye.drag = 1.0f;
        freye.sprite = data::SPRITE_NPC_FREYE;

        nessa.type = data::ENTITY_NPC;
        nessa.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        nessa.name = "Nessa";
        nessa.radius = 10;
        nessa.dialog_root_key = "DIALOG_NESSA_INTRO";
        nessa.hp_max = 150;
        nessa.path_id = 0;
        nessa.speed_min = 300;
        nessa.speed_max = 600;
        nessa.drag = 1.0f;
        nessa.sprite = data::SPRITE_NPC_NESSA;

        elane.type = data::ENTITY_NPC;
        elane.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        elane.name = "Elane";
        elane.radius = 10;
        elane.dialog_root_key = "DIALOG_ELANE_INTRO";
        elane.hp_max = 100;
        elane.path_id = 0;
        elane.speed_min = 300;
        elane.speed_max = 600;
        elane.drag = 1.0f;
        elane.sprite = data::SPRITE_NPC_ELANE;

        chicken.type = data::ENTITY_NPC;
        chicken.spec = data::ENTITY_SPEC_NPC_CHICKEN;
        chicken.name = "Chicken";
        chicken.ambient_fx = "sfx_chicken_cluck";
        chicken.ambient_fx_delay_min = 15;
        chicken.ambient_fx_delay_max = 30;
        chicken.radius = 5;
        chicken.hp_max = 15;
        chicken.path_id = -1;
        chicken.speed_min = 50;
        chicken.speed_max = 150;
        chicken.drag = 1.0f;
        chicken.sprite = data::SPRITE_NPC_CHICKEN;
    }

    assert(ARRAY_SIZE(townfolk) == ARRAY_SIZE(townfolk_ids));

    const double townfolkSpawnRate = 2.0;
    static double lastTownfolkSpawnedAt = 0;
    if (now - lastTownfolkSpawnedAt >= townfolkSpawnRate) {
        for (int i = 0; i < ARRAY_SIZE(townfolk_ids); i++) {
            data::Entity *entity = entityDb->FindEntity(townfolk_ids[i], data::ENTITY_NPC);
            if (!entity) {
                entity = SpawnEntityProto(*this, mapId, { 0, 0 }, *townfolk[i]);
                if (entity) {
                    BroadcastEntitySpawn(entity->id);
                    townfolk_ids[i] = entity->id;
                    lastTownfolkSpawnedAt = now;
                }
                break;
            }
        }
    }

    const double chickenSpawnCooldown = 3.0;
    static double lastChickenSpawnedAt = 0;
    if (now - lastChickenSpawnedAt >= chickenSpawnCooldown) {
        for (int i = 0; i < ARRAY_SIZE(chicken_ids); i++) {
            data::Entity *entity = entityDb->FindEntity(chicken_ids[i], data::ENTITY_NPC);
            if (!entity) {
                Vector2 spawn{};
                // TODO: If chicken spawns inside something, immediately despawn
                //       it (or find better strat for this)
                spawn.x = GetRandomValue(200, 1200);
                spawn.y = GetRandomValue(1000, 1700);
                entity = SpawnEntityProto(*this, mapId, spawn, chicken);
                if (entity) {
                    BroadcastEntitySpawn(entity->id);
                    chicken_ids[i] = entity->id;
                    lastChickenSpawnedAt = now;
                }
                break;
            }
        }
    }
}
void GameServer::TickSpawnCaveNPCs(uint32_t mapId)
{
    static uint32_t eid_bots[1];
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        data::Entity *entity = entityDb->FindEntity(eid_bots[i], data::ENTITY_NPC);
        if (!entity && ((int)tick % 100 == i * 10)) {
            entity = SpawnEntity(data::ENTITY_NPC);
            if (!entity) continue;

            entity->name = "Cave Lily";

            entity->map_id = mapId;
            entity->position = { 1620, 450 };

            entity->radius = 10;

            entity->hp_max = 100;
            entity->hp = entity->hp_max;

            entity->speed = GetRandomValue(300, 600);
            entity->drag = 8.0f;

            entity->sprite = data::SPRITE_NPC_LILY;
            //entity->direction = data::DIR_E;

            BroadcastEntitySpawn(entity->id);
            eid_bots[i] = entity->id;
        }
    }
}
void GameServer::TickEntityNPC(uint32_t entityId, double dt)
{
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity) return;

    Tilemap *map = FindMap(entity->map_id);
    if (!map) return;

    if (now - entity->dialog_spawned_at > SV_ENTITY_DIALOG_INTERESTED_DURATION) {
        entity->dialog_spawned_at = 0;
    }

    // TODO: tick_pathfind?
    // TODO: Instead of path_id -1 for "no path" change it to 0 like everything else. After tilemap refactor.
    if (entity->path_id >= 0 && !entity->dialog_spawned_at) {
        AiPathNode *ai_path_node = map->GetPathNode(entity->path_id, entity->path_node_target);
        if (ai_path_node) {
            Vector2 target = ai_path_node->pos;
            Vector2 to_target = Vector2Subtract(target, entity->position);
            if (Vector2LengthSqr(to_target) < 10 * 10) {
                if (entity->path_node_last_reached != entity->path_node_target) {
                    // Arrived at a new node
                    entity->path_node_last_reached = entity->path_node_target;
                    entity->path_node_arrived_at = now;
                }
                if (now - entity->path_node_arrived_at > ai_path_node->waitFor) {
                    // Been at node long enough, move on
                    entity->path_node_target = map->GetNextPathNodeIndex(entity->path_id, entity->path_node_target);
                }
            } else {
                entity->path_node_last_reached = 0;
                entity->path_node_arrived_at = 0;

                Vector2 move_force = to_target;
                move_force = Vector2Normalize(move_force);
                move_force = Vector2Scale(move_force, entity->speed);
                entity->ApplyForce(move_force);
            }

    #if 0
            InputCmd aiCmd{};
            if (eBot.position.x < target.x) {
                aiCmd.east = true;
            } else if (eBot.position.x > target.x) {
                aiCmd.west = true;
            }
            if (eBot.position.y < target.y) {
                aiCmd.south = true;
            } else if (eBot.position.y > target.y) {
                aiCmd.north = true;
            }

            Vector2 move_force = aiCmd.GenerateMoveForce(eBot.speed);
    #else

    #endif
        }
    }

    // Chicken pathing AI (really bad)
    if (entity->spec == data::ENTITY_SPEC_NPC_CHICKEN) {
        if (entity->colliding) {
            entity->path_rand_direction = {};
        }

        if (now - entity->path_rand_started_at >= entity->path_rand_duration) {
            double duration = 0;
            Vector2 dir{};
            if (Vector2LengthSqr(entity->path_rand_direction) == 0) {
                dir.x = GetRandomFloatMinusOneToOne();
                dir.y = GetRandomFloatMinusOneToOne();
                dir = Vector2Normalize(dir);
                duration = GetRandomValue(2, 4);
            } else {
                dir = {};
                duration = GetRandomValue(5, 10);
            }
            entity->path_rand_started_at = now;
            entity->path_rand_direction = dir;
            entity->path_rand_duration = duration;
        }
        Vector2 move = Vector2Scale(entity->path_rand_direction, entity->speed);
        entity->ApplyForce(move);
    }

    entityDb->EntityTick(entityId, dt);
}
void GameServer::TickEntityPlayer(uint32_t entityId, double dt)
{
    entityDb->EntityTick(entityId, dt);
}
void GameServer::TickEntityProjectile(uint32_t entityId, double dt)
{
    data::Entity *projectile = entityDb->FindEntity(entityId);
    assert(projectile);

    // Gravity
    //AspectPhysics &ePhysics = map.ePhysics[entityIndex];
    //playerEntity.ApplyForce({ 0, 5 });

    entityDb->EntityTick(entityId, dt);

    if (now - projectile->spawned_at > 1.0) {
        DespawnEntity(projectile->id);
    }

    if (projectile->despawned_at) {
        return;
    }

    for (data::Entity &victim : entityDb->entities) {
        if (victim.type == data::ENTITY_NPC && !victim.despawned_at && victim.Alive() && victim.map_id == projectile->map_id) {
            assert(victim.id);
            if (victim.Dead()) {
                continue;
            }

            Rectangle projectileHitbox = entityDb->EntityRect(projectile->id);
            Rectangle targetHitbox = entityDb->EntityRect(victim.id);
            if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                victim.TakeDamage(GetRandomValue(3, 8));
                if (victim.Alive()) {
                    if (!victim.dialog_spawned_at) {
                        switch (victim.spec) {
                            case data::ENTITY_SPEC_NPC_TOWNFOLK: {
                                //BroadcastEntitySay(victim.id, TextFormat("Ouch! You hit me with\nprojectile #%u!", projectile->id));
                                BroadcastEntitySay(victim.id, "Ouch!");
                                break;
                            }
                            case data::ENTITY_SPEC_NPC_CHICKEN: {
                                BroadcastEntitySay(victim.id, "*squawk*!");
                                break;
                            }
                        }
                    }
                } else {
                    DespawnEntity(victim.id);
                }
                DespawnEntity(projectile->id);
                lastCollisionA = projectileHitbox;
                lastCollisionB = targetHitbox;
            }
        }
    }
}
void GameServer::WarpEntity(Tilemap &map, uint32_t entityId, data::Entity &warp)
{
    assert(entityId);

    data::Entity *victim = entityDb->FindEntity(entityId);
    if (!victim) return;

    if (warp.warp_dest_map.size()) {
        // TODO: We need to move our victim to the new map
        Tilemap *map = FindOrLoadMap(warp.warp_dest_map);
        if (map) {
            // TODO: Move victim to other map?
            victim->map_id = map->id;
            victim->position = warp.warp_dest_pos;
            victim->force_accum = {};
            victim->velocity = {};
        } else {
            assert(!"UH-OH");
        }
    } else {
        // TODO: This needs to ask GameServer to load the new map and
        // we also need to move our victim to the new map

        //Err err = Load(warp.templateMap);
        //if (err) {
        //    assert(!"UH-OH");
        //    exit(EXIT_FAILURE);
        //}

#if 0
        // TODO: Make a copy of the template map if you wanna edit it
        err = Save(warp.destMap);
        if (err) {
            assert(!"UH-OH");
            exit(EXIT_FAILURE);
        }
#endif

#if 0
        // TODO: The GameServer should be making a new map using the
        // template. Not this function. Return something useful to the
        // game server (e.g. map_id or mapTemplateId).
        WangTileset wangTileset{};
        err = wangTileset.Load(*this, warp.templateTileset);
        if (err) {
            assert(!"UH-OH");
            exit(EXIT_FAILURE);
        }

        WangMap wangMap{};
        err = wangTileset.GenerateMap(width, height, *this, wangMap);
        if (err) {
            assert(!"UH-OH");
            exit(EXIT_FAILURE);
        }

        SetFromWangMap(wangMap, now);
#endif
    }
}
void GameServer::TickResolveEntityWarpCollisions(Tilemap &map, uint32_t entityId, double now)
{
    assert(entityId);

    data::Entity *victim = entityDb->FindEntity(entityId);
    if (!victim) {
        return;
    }
    if (victim->type != data::ENTITY_PLAYER) {
        return;
    }
    if (!victim->radius) {
        return;
    }
    if (victim->on_warp) {
        // De-bounce warps. Only allow warping after victim has moved off any warps
        // they may have just used to get here.
        return;
    }

    for (data::Entity &entity : entityDb->entities) {
        // TODO: Make a better way to find matching entities, e.g. FindEntity(map_id, type, alive)
        if (entity.spec != data::ENTITY_SPEC_OBJ_WARP || !entity.id || entity.despawned_at || entity.map_id != victim->map_id) {
            continue;
        }

        data::Entity &warp = entity;
        const bool on_warp = dlb_CheckCollisionCircleRec(victim->position, victim->radius, warp.warp_collider, 0);
        if (on_warp) {
            victim->on_warp = true;
            WarpEntity(map, entityId, warp);
            break;
        }
    }
}
void GameServer::Tick(void)
{
    // HACK: Only spawn NPCs in map 1, whatever map that may be (hopefully it's Level_001)
    TickSpawnTownNPCs(1);
    TickSpawnCaveNPCs(2);

    // Tick entites
    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at) {
            continue;
        }
        assert(entity.id);

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        assert(entityIndex);

        // TODO: if (map.sleeping) continue

        switch (entity.type) {
            case data::ENTITY_NPC:        TickEntityNPC        (entity.id, SV_TICK_DT); break;
            case data::ENTITY_PLAYER:     TickEntityPlayer     (entity.id, SV_TICK_DT); break;
            case data::ENTITY_PROJECTILE: TickEntityProjectile (entity.id, SV_TICK_DT); break;
        }

        Tilemap *map = FindMap(entity.map_id);
        if (map) {
            map->ResolveEntityTerrainCollisions(entity.id);
            TickResolveEntityWarpCollisions(*map, entity.id, now);
        }

        bool newlySpawned = entity.spawned_at == now;
        data::UpdateSprite(entity, SV_TICK_DT, newlySpawned);
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}
void GameServer::SerializeSnapshot(uint32_t entityId, Msg_S_EntitySnapshot &entitySnapshot)
{
    assert(entityId);
    if (!entityId) return;

    data::Entity *entity = entityDb->FindEntity(entityId);
    assert(entity);
    if (!entity) return;

    entitySnapshot.server_time = lastTickedAt;

    // Entity
    entitySnapshot.entity_id = entity->id;
    entitySnapshot.type      = entity->type;
    entitySnapshot.map_id    = entity->map_id;
    entitySnapshot.position  = entity->position;

    // Collision
    //entitySnapshot.radius = projectile->radius;

    // Life
    entitySnapshot.hp_max = entity->hp_max;
    entitySnapshot.hp     = entity->hp;

    // Physics
    //entitySnapshot.drag     = projectile->drag;
    //entitySnapshot.speed    = entity->speed;
    entitySnapshot.velocity = entity->velocity;
}
void GameServer::SendClientSnapshots(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = players[clientIdx];
        data::Entity *entity = entityDb->FindEntity(serverPlayer.entityId);
        if (!entity) {
            assert(0);
            printf("[game_server] could not find client id %d's entity id %u. cannot send snapshots\n", clientIdx, serverPlayer.entityId);
            continue;
        }

        Tilemap *map = FindMap(entity->map_id);
        if (!map) {
            assert(0);
            printf("[game_server] could not find client id %d's entity id %u's map. cannot send snapshots\n", clientIdx, serverPlayer.entityId);
            continue;
        }

        // TODO: Wut dis do?
        for (int tileChunkIdx = 0; tileChunkIdx < serverPlayer.chunkList.size(); tileChunkIdx++) {
            TileChunkRecord &chunkRec = serverPlayer.chunkList[tileChunkIdx];
            if (chunkRec.lastSentAt < map->chunkLastUpdatedAt) {
                SendTileChunk(clientIdx, *map, chunkRec.coord.x, chunkRec.coord.y);
                chunkRec.lastSentAt = now;
                printf("[game_server] sending chunk to client\n");
            }
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (data::Entity &entity : entityDb->entities) {
            if (!entity.id || !entity.type) {
                assert(!entity.id && !entity.type);  // why has one but not other!?
                continue;
            }

            if (serverPlayer.joinedAt == now && !entity.despawned_at) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        SerializeSpawn(entity.id, *msg);
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
           /* } else if (entity.despawned_at == now) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
                    if (msg) {
                        msg->entityId = entity.id;
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }*/
            } else if (!entity.despawned_at) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                    if (msg) {
                        SerializeSnapshot(entity.id, *msg);
                        // TODO: We only need this for the sv_player entity, not EVERY entity. Make it suck less.
                        msg->last_processed_input_cmd = serverPlayer.lastInputSeq;
                        yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
                    }
                }
            }
        }
    }
}
void GameServer::SendClockSync(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = players[clientIdx];
        if (serverPlayer.needsClockSync && yj_server->CanSendMessage(clientIdx, CHANNEL_R_CLOCK_SYNC)) {
            Msg_S_ClockSync *msg = (Msg_S_ClockSync *)yj_server->CreateMessage(clientIdx, MSG_S_CLOCK_SYNC);
            if (msg) {
                msg->serverTime = GetTime();
                msg->playerEntityId = serverPlayer.entityId;
                yj_server->SendMessage(clientIdx, CHANNEL_R_CLOCK_SYNC, msg);
                serverPlayer.needsClockSync = false;
            }
        }
    }
}
void GameServer::Update(void)
{
    if (!yj_server->IsRunning())
        return;

    yj_server->AdvanceTime(now);
    yj_server->ReceivePackets();
    ProcessMessages();

    bool hasDelta = false;
    while (tickAccum >= SV_TICK_DT) {
        UpdateServerPlayers();
        Tick();
        tickAccum -= SV_TICK_DT;
        hasDelta = true;
    }

    // TODO(dlb): Calculate actual state deltas
    if (hasDelta) {
        SendClientSnapshots();
        DestroyDespawnedEntities();
    }

    SendClockSync();
    yj_server->SendPackets();
}

void GameServer::Stop(void)
{
    yj_server->Stop();
    delete entityDb;
}
