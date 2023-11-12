#include "game_server.h"
#include "../common/data.h"

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

void GameServer::OnClientJoin(int clientIdx)
{
    static const Color playerColors[]{
        MAROON,
        LIME,
        SKYBLUE
    };

    data::Entity *player = SpawnEntity(data::ENTITY_PLAYER);
    if (player) {
        ServerPlayer &sv_player = players[clientIdx];
        sv_player.needsClockSync = true;
        sv_player.joinedAt = now;
        sv_player.entityId = player->id;

        player->type = data::ENTITY_PLAYER;
        player->map_id = LEVEL_001;  // TODO: Something smarter
        const Vector3 caveEntrance{ 3100, 1100, 0 };
        const Vector3 townCenter{ 1660, 2360, 0 };
        player->position = caveEntrance;
        player->radius = 8;
        player->hp_max = 100;
        player->hp = player->hp_max;
        player->speed = 3000;
        player->drag = 0.9f;
        player->sprite = "sprite_chr_mage";
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
#if 0
    // TODO: Huh? What dis mean?
    if (!maps.size()) return;

    // TODO: Which map is this sv_player currently on? Need to despawn them from that map.
    data::Tilemap &map = *maps[0];
#endif

    ServerPlayer &serverPlayer = players[clientIdx];
    DespawnEntity(serverPlayer.entityId);
    serverPlayer = {};
}

uint32_t GameServer::GetPlayerEntityId(uint32_t clientIdx)
{
    return clientIdx + 1;
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

data::Tilemap *GameServer::FindOrLoadMap(const std::string &map_id)
{
#if 1
    // TODO: Go back to assuming it's not already loaded once we figure out packs
    data::Tilemap &map = data::packs[0]->FindTilemap(map_id);
    if (!map.id.empty()) {
        return &map;
    } else {
        return 0;
    }
#else
    const auto &mapEntry = mapsByName.find(filename);
    if (mapEntry != mapsByName.end()) {
        size_t mapIndex = mapEntry->second;
        return maps[mapIndex];
    } else {
        Err err = RN_SUCCESS;
        data::Tilemap *map = new data::Tilemap();
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
#endif
}
data::Tilemap *GameServer::FindMap(const std::string &map_id)
{
    // TODO: Remove this alias and call data::* directly?
    auto &tile_map = data::packs[0]->FindTilemap(map_id);
    return &tile_map;
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
    strncpy(entitySpawn.name,   entity->name  .c_str(), SV_MAX_ENTITY_NAME_LEN);
    strncpy(entitySpawn.map_id, entity->map_id.c_str(), SV_MAX_TILE_MAP_NAME_LEN);
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
    strncpy(entitySpawn.sprite, entity->sprite.c_str(), SV_MAX_SPRITE_NAME_LEN);
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
void GameServer::SendEntitySay(int clientIdx, uint32_t entityId, uint32_t dialogId, const std::string &title, const std::string &message)
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
            strncpy(msg->title, title.c_str(), SV_MAX_ENTITY_SAY_TITLE_LEN);
            strncpy(msg->message, message.c_str(), SV_MAX_ENTITY_SAY_MSG_LEN);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntitySay(uint32_t entityId, const std::string &title, const std::string &message)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntitySay(clientIdx, entityId, 0, title, message);
    }
}
void GameServer::SendTileChunk(int clientIdx, data::Tilemap &map, uint32_t x, uint32_t y)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_TILE_EVENT)) {
        Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yj_server->CreateMessage(clientIdx, MSG_S_TILE_CHUNK);
        if (msg) {
            map.SV_SerializeChunk(*msg, x, y);
            yj_server->SendMessage(clientIdx, CHANNEL_R_TILE_EVENT, msg);
        }
    }
}
void GameServer::BroadcastTileChunk(data::Tilemap &map, uint32_t x, uint32_t y)
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
    uint32_t dialog_id = dialog.id;
    const std::string_view *msg = &dialog.msg;

    DialogListener listener = dialog_library.FindListenerByKey(dialog.key);
    if (listener) {
        ServerPlayer &player = players[clientIdx];
        uint32_t redirect_id = listener(player.entityId, entity.id, dialog.id);
        if (redirect_id != dialog_id) {
            Dialog *redirect_dialog = dialog_library.FindById(redirect_id);
            if (redirect_dialog) {
                dialog_id = redirect_id;
                msg = &redirect_dialog->msg;
            } else {
                // something bad happened in listener, just show the original
                // dialog i guess?
                assert(!"wtf, listener redirected us to a bad dialog id?");
            }
        }
    }

    if (dialog_id) {
        SendEntitySay(clientIdx, entity.id, dialog_id, entity.name, std::string(*msg));
        entity.dialog_spawned_at = now;
    }
}

void GameServer::ProcessMsg(int clientIdx, Msg_C_EntityInteract &msg)
{
    // TODO: Check if sv_player is allowed to actually interact with this
    // particular entity. E.g. are they even in the same map as them!?
    // Proximity, etc.
    data::Entity *entity = entityDb->FindEntity(msg.entityId, data::ENTITY_NPC);
    if (entity) {
        ServerPlayer &player = players[clientIdx];
        data::Entity *e_player = entityDb->FindEntity(player.entityId, data::ENTITY_PLAYER);
        if (!e_player) {
            assert(!"player entity not found.. huh?");
            return;
        }

        const float dist_x = fabs(e_player->position.x - entity->position.x);
        const float dist_y = fabs(e_player->position.y - entity->position.y);
        if (dist_x > SV_MAX_ENTITY_INTERACT_DIST || dist_y > SV_MAX_ENTITY_INTERACT_DIST) {
            if (dist_x > SV_MAX_ENTITY_INTERACT_DIST * 2 || dist_y > SV_MAX_ENTITY_INTERACT_DIST * 2) {
                // Player WAY too far away, kick
                printf("WAY too far\n");
            } else {
                // Player too far away, ignore request
                printf("too far\n");
            }
            return;
        }

        Dialog *dialog = dialog_library.FindByKey(entity->dialog_root_key);
        if (dialog) {
            RequestDialog(clientIdx, *entity, *dialog);
        }
    }
}
void GameServer::ProcessMsg(int clientIdx, Msg_C_EntityInteractDialogOption &msg)
{
    ServerPlayer &player = players[clientIdx];

    Dialog *prevDialog = dialog_library.FindById(msg.dialog_id);
    if (!prevDialog) {
        // Client being stupid?
        assert(!"invalid dialog id");
        return;
    }

    if (msg.option_id >= prevDialog->nodes.size()) {
        // Client being stupid?
        assert(!"invalid dialog option id (out of bounds)");
        return;
    }

    // TODO: Check if player is allowed to actually interact with this
    // particular entity. E.g. are they even in the same map as them!?
    // Proximity, etc. If they leave proximity, send EntityInteractCancel
    data::Entity *entity = entityDb->FindEntity(msg.entity_id, data::ENTITY_NPC);
    if (entity) {
        DialogNode &optionTag = prevDialog->nodes[msg.option_id];

        if (optionTag.type != DIALOG_NODE_LINK) {
            // Client being stupid?
            assert(!"invalid dialog option id (not a link)");
            return;
        }

        std::string_view nextDialogKey = optionTag.data;
        Dialog *nextDialog = dialog_library.FindByKey(nextDialogKey);
        if (!nextDialog) {
            // Client being stupid? (or bug in data where msg string has more options than options id array)
            assert(!"missing dialog option id?");
            return;
        }

        RequestDialog(clientIdx, *entity, *nextDialog);
    }
}
void GameServer::ProcessMsg(int clientIdx, Msg_C_InputCommands &msg)
{
    players[clientIdx].inputQueue = msg.cmdQueue;
}
void GameServer::ProcessMsg(int clientIdx, Msg_C_TileInteract &msg)
{
    ServerPlayer &player = players[clientIdx];

    data::Entity *e_player = entityDb->FindEntity(player.entityId, data::ENTITY_PLAYER);
    if (msg.map_id != e_player->map_id) {
        // Wrong map, kick player?
        return;
    }

    // TODO: Figure out what tool player is holding
    //player.activeItem

    // TODO: Check if sv_player is allowed to actually interact with this
    // particular tile. E.g. are they even in the same map as it!?
    // Holding the right tool, proximity, etc.
    data::Tilemap *map = FindMap(msg.map_id);
    if (!map) {
        // Map not loaded, kick player?
        return;
    }

    data::Tilemap::Coord player_coord{};
    if (!map->WorldToTileIndex(e_player->position.x, e_player->position.y, player_coord)) {
        // Player somehow not on a tile!? Server error!?!?
        assert(!"player outside of map, wot?");
        return;
    }

    const int dist_x = abs((int)player_coord.x - (int)msg.x);
    const int dist_y = abs((int)player_coord.y - (int)msg.y);
    if (dist_x > SV_MAX_TILE_INTERACT_DIST_IN_TILES ||
        dist_y > SV_MAX_TILE_INTERACT_DIST_IN_TILES)
    {
        if (dist_x > SV_MAX_TILE_INTERACT_DIST_IN_TILES * 2 ||
            dist_y > SV_MAX_TILE_INTERACT_DIST_IN_TILES * 2)
        {
            // Player WAY too far away, kick
            printf("WAY too far\n");
        } else {
            // Player too far away, ignore request
            printf("too far\n");
        }
        return;
    }

    Tile tile{};
    if (!map->AtTry(msg.x, msg.y, tile)) {
        // Tile at x/y is not a valid tile type.. hmm.. is it void?
        assert(!"not a valid tile");
        return;
    }

    const char *new_tile_def = 0;
    uint8_t obj = map->At_Obj(msg.x, msg.y);
    const data::ObjectData *obj_data = map->GetObjectData(msg.x, msg.y);

    bool handled = false;
    if (obj_data) {
        if (obj_data->type == "lootable") {
            SendEntitySay(clientIdx, player.entityId, 0, "Chest", obj_data->loot_table_id);
            handled = true;
        } else if (obj_data->type == "sign") {
            const char *signText = TextFormat("%s\n%s\n%s\n%s",
                obj_data->sign_text[0].c_str(),
                obj_data->sign_text[1].c_str(),
                obj_data->sign_text[2].c_str(),
                obj_data->sign_text[3].c_str()
            );
            SendEntitySay(clientIdx, player.entityId, 0, "Sign", signText);
            handled = true;
        } else if (obj_data->type == "warp") {
            const char *warpInfo = TextFormat("%s (%u, %u)",
                obj_data->warp_map_id.c_str(),
                obj_data->warp_dest_x,
                obj_data->warp_dest_y
            );
            SendEntitySay(clientIdx, player.entityId, 0, "Warp", warpInfo);
            handled = true;
        }
    } else {
        const data::TileDef &tile_def = map->GetTileDef(tile);
        if (tile_def.id == "til_water_dark") {
            new_tile_def = "til_stone_path";
        } else if (tile_def.id == "til_stone_path") {
            new_tile_def = "til_water_dark";
        }
        handled = true;
    }

    if (handled) {
        if (new_tile_def) {
            // TODO(perf): Make some kind of map from string -> tile_def_index in the map?
            // * OR * make the maps all have global tile def ids instead of local tile def ids
            int new_tile_def_idx = 0;
            for (int i = 0; i < map->tileDefs.size(); i++) {
                if (map->tileDefs[i] == new_tile_def) {
                    new_tile_def_idx = i;
                    break;
                }
            }

            if (new_tile_def_idx) {
                map->Set(msg.x, msg.y, new_tile_def_idx, now);
            }
        }
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
                    case MSG_C_ENTITY_INTERACT:               ProcessMsg(clientIdx, *(Msg_C_EntityInteract             *)yjMsg); break;
                    case MSG_C_ENTITY_INTERACT_DIALOG_OPTION: ProcessMsg(clientIdx, *(Msg_C_EntityInteractDialogOption *)yjMsg); break;
                    case MSG_C_INPUT_COMMANDS:                ProcessMsg(clientIdx, *(Msg_C_InputCommands              *)yjMsg); break;
                    case MSG_C_TILE_INTERACT:                 ProcessMsg(clientIdx, *(Msg_C_TileInteract               *)yjMsg); break;
                }
                yj_server->ReleaseMessage(clientIdx, yjMsg);
                yjMsg = yj_server->ReceiveMessage(clientIdx, channelIdx);
            }
        }
    }
}

data::Entity *GameServer::SpawnProjectile(const std::string &map_id, Vector3 position, Vector2 direction, Vector3 initial_velocity)
{
    data::Entity *projectile = SpawnEntity(data::ENTITY_PROJECTILE);
    if (!projectile) return 0;

    projectile->spec = data::ENTITY_SPEC_PRJ_FIREBALL;

    // [Entity] position
    projectile->position = position;
    projectile->map_id = map_id;

    // [Physics] shoot in facing direction
    Vector2 dir = Vector2Scale(direction, 100);
    // [Physics] add a bit of random spread
    dir.x += GetRandomValue(-20, 20);
    dir.y += GetRandomValue(-20, 20);
    dir = Vector2Normalize(dir);
    const float random_speed = GetRandomValue(400, 500);
    Vector3 velocity = initial_velocity;
    velocity.x += dir.x * random_speed;
    velocity.y += dir.y * random_speed;
    // [Physics] random speed
    projectile->velocity = velocity;
    projectile->drag = 0.02f;

    projectile->sprite = "sprite_prj_fireball";
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
                Vector3 move_force = input_cmd->GenerateMoveForce(player->speed);
                player->ApplyForce(move_force);

                if (input_cmd->fire) {
                    if (now - player->last_attacked_at > player->attack_cooldown) {
                        Vector3 projectile_spawn_pos = player->position;
                        projectile_spawn_pos.z += 24;  // "hand" height
                        data::Entity *projectile = SpawnProjectile(player->map_id, projectile_spawn_pos, input_cmd->facing, player->velocity);
                        if (projectile) {
                            Vector3 recoil_force{};
                            recoil_force.x = -projectile->velocity.x;
                            recoil_force.y = -projectile->velocity.y;
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
data::Entity *SpawnEntityProto(GameServer &server, const std::string &map_id, Vector3 position, data::EntityProto &proto)
{
    data::Tilemap *map = server.FindMap(map_id);
    if (!map) return 0;

    data::Entity *entity = server.SpawnEntity(data::ENTITY_NPC);
    if (!entity) return 0;

    entity->spec                 = proto.spec;
    entity->map_id               = map_id;
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
    entity->direction            = proto.direction;

    data::AiPathNode *aiPathNode = map->GetPathNode(entity->path_id, 0);
    if (aiPathNode) {
        entity->position.x = aiPathNode->pos.x;
        entity->position.y = aiPathNode->pos.y;
    }

    return entity;
}
void GameServer::TickSpawnTownNPCs(const std::string &map_id)
{
    // TODO: Load protos from an mdesk file
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

    // TODO: put entities in some map mdesk file or something?
    // when join game, make new save directory that copies all the datas:
    //  "saves/20231016/map/overworld.mdesk"  (contains all entity states, modified blocks, etc.)
    //  "saves/20231016/dungeon/cave.mdesk"   (contains RNG generated cave map for this playthrough)
    // entities: [
    //     {
    //         proto_name: npc_chicken
    //         position: 300 200 108
    //         health: 300
    //     }
    // ]

    if (!lily.type) {
        lily.type = data::ENTITY_NPC;
        lily.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        lily.name = "Lily";
        lily.radius = 16;
        lily.dialog_root_key = "DIALOG_LILY_INTRO";
        lily.hp_max = 777;
        lily.path_id = 0;
        lily.speed_min = 300;
        lily.speed_max = 600;
        lily.drag = 1.0f;
        lily.sprite = "sprite_npc_lily";
        lily.direction = data::DIR_E;

        freye.type = data::ENTITY_NPC;
        freye.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        freye.name = "Freye";
        freye.radius = 16;
        freye.dialog_root_key = "DIALOG_FREYE_INTRO";
        freye.hp_max = 200;
        freye.path_id = 0;
        freye.speed_min = 300;
        freye.speed_max = 600;
        freye.drag = 1.0f;
        freye.sprite = "sprite_npc_freye";
        freye.direction = data::DIR_E;

        nessa.type = data::ENTITY_NPC;
        nessa.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        nessa.name = "Nessa";
        nessa.radius = 16;
        nessa.dialog_root_key = "DIALOG_NESSA_INTRO";
        nessa.hp_max = 150;
        nessa.path_id = 0;
        nessa.speed_min = 300;
        nessa.speed_max = 600;
        nessa.drag = 1.0f;
        nessa.sprite = "sprite_npc_nessa";
        nessa.direction = data::DIR_E;

        elane.type = data::ENTITY_NPC;
        elane.spec = data::ENTITY_SPEC_NPC_TOWNFOLK;
        elane.name = "Elane";
        elane.radius = 16;
        elane.dialog_root_key = "DIALOG_ELANE_INTRO";
        elane.hp_max = 100;
        elane.path_id = 0;
        elane.speed_min = 300;
        elane.speed_max = 600;
        elane.drag = 1.0f;
        elane.sprite = "sprite_npc_elane";
        elane.direction = data::DIR_E;

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
        chicken.sprite = "sprite_npc_chicken";
    }

    assert(ARRAY_SIZE(townfolk) == ARRAY_SIZE(townfolk_ids));

    const double townfolkSpawnRate = 2.0;
    if (now - lastTownfolkSpawnedAt >= townfolkSpawnRate) {
        for (int i = 0; i < ARRAY_SIZE(townfolk_ids); i++) {
            data::Entity *entity = entityDb->FindEntity(townfolk_ids[i], data::ENTITY_NPC);
            if (!entity) {
                entity = SpawnEntityProto(*this, map_id, { 0, 0 }, *townfolk[i]);
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
    if (now - lastChickenSpawnedAt >= chickenSpawnCooldown) {
        for (int i = 0; i < ARRAY_SIZE(chicken_ids); i++) {
            data::Entity *entity = entityDb->FindEntity(chicken_ids[i], data::ENTITY_NPC);
            if (!entity) {
                Vector3 spawn{};
                // TODO: If chicken spawns inside something, immediately despawn
                //       it (or find better strat for this)
                spawn.x = GetRandomValue(400, 2400);
                spawn.y = GetRandomValue(2000, 3400);
                entity = SpawnEntityProto(*this, map_id, spawn, chicken);
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
void GameServer::TickSpawnCaveNPCs(const std::string &map_id)
{
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        data::Entity *entity = entityDb->FindEntity(eid_bots[i], data::ENTITY_NPC);
        if (!entity && ((int)tick % 100 == i * 10)) {
            entity = SpawnEntity(data::ENTITY_NPC);
            if (!entity) continue;

            entity->name = "Cave Lily";

            entity->map_id = map_id;
            entity->position = { 1620, 450 };

            entity->radius = 10;

            entity->hp_max = 100;
            entity->hp = entity->hp_max;

            entity->speed = GetRandomValue(300, 600);
            entity->drag = 8.0f;

            entity->sprite = "sprite_npc_lily";
            //entity->direction = data::DIR_E;

            BroadcastEntitySpawn(entity->id);
            eid_bots[i] = entity->id;
        }
    }
}
void GameServer::TickEntityNPC(data::Entity &e_npc, double dt)
{
    data::Tilemap *map = FindMap(e_npc.map_id);
    if (!map) return;

    if (now - e_npc.dialog_spawned_at > SV_ENTITY_DIALOG_INTERESTED_DURATION) {
        e_npc.dialog_spawned_at = 0;
    }

    // TODO: tick_pathfind?
    // TODO: Instead of path_id -1 for "no path" change it to 0 like everything else. After tilemap refactor.
    if (e_npc.path_id >= 0 && !e_npc.dialog_spawned_at) {
        data::AiPathNode *ai_path_node = map->GetPathNode(e_npc.path_id, e_npc.path_node_target);
        if (ai_path_node) {
            Vector3 target = ai_path_node->pos;
            Vector3 to_target = Vector3Subtract(target, e_npc.position);
            if (Vector3LengthSqr(to_target) < 10 * 10) {
                if (e_npc.path_node_last_reached != e_npc.path_node_target) {
                    // Arrived at a new node
                    e_npc.path_node_last_reached = e_npc.path_node_target;
                    e_npc.path_node_arrived_at = now;
                }
                if (now - e_npc.path_node_arrived_at > ai_path_node->waitFor) {
                    // Been at node long enough, move on
                    e_npc.path_node_target = map->GetNextPathNodeIndex(e_npc.path_id, e_npc.path_node_target);
                }
            } else {
                e_npc.path_node_last_reached = 0;
                e_npc.path_node_arrived_at = 0;

                Vector3 move_force = to_target;
                move_force = Vector3Normalize(move_force);
                move_force = Vector3Scale(move_force, e_npc.speed);
                e_npc.ApplyForce(move_force);
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
    if (e_npc.spec == data::ENTITY_SPEC_NPC_CHICKEN) {
        if (e_npc.colliding) {
            e_npc.path_rand_direction = {};
        }

        if (now - e_npc.path_rand_started_at >= e_npc.path_rand_duration) {
            double duration = 0;
            Vector3 dir{};
            if (Vector3LengthSqr(e_npc.path_rand_direction) == 0) {
                dir.x = GetRandomFloatMinusOneToOne();
                dir.y = GetRandomFloatMinusOneToOne();
                dir = Vector3Normalize(dir);
                duration = GetRandomValue(2, 4);
            } else {
                dir = {};
                duration = GetRandomValue(5, 10);
            }
            e_npc.path_rand_started_at = now;
            e_npc.path_rand_direction = dir;
            e_npc.path_rand_duration = duration;
        }
        Vector3 move = Vector3Scale(e_npc.path_rand_direction, e_npc.speed);
        e_npc.ApplyForce(move);
    }

    entityDb->EntityTick(e_npc, dt);
}
void GameServer::TickEntityPlayer(data::Entity &e_player, double dt)
{
    entityDb->EntityTick(e_player, dt);
}
void GameServer::TickEntityProjectile(data::Entity &e_projectile, double dt)
{
    // Gravity
    //AspectPhysics &ePhysics = map.ePhysics[entityIndex];
    //playerEntity.ApplyForce({ 0, 5 });

    entityDb->EntityTick(e_projectile, dt);

    if (now - e_projectile.spawned_at > 1.0) {
        DespawnEntity(e_projectile.id);
    }

    if (e_projectile.despawned_at) {
        return;
    }

    for (data::Entity &e_target : entityDb->entities) {
        if (e_target.type == data::ENTITY_NPC
            && !e_target.despawned_at
            && e_target.Alive()
            && e_target.map_id == e_projectile.map_id)
        {
            assert(e_target.id);
            if (e_target.Dead()) {
                continue;
            }

            Rectangle projectileHitbox = entityDb->EntityRect(e_projectile.id);
            Rectangle targetHitbox = entityDb->EntityRect(e_target.id);
            if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                e_target.TakeDamage(GetRandomValue(3, 8));
                if (e_target.Alive()) {
                    if (!e_target.dialog_spawned_at) {
                        switch (e_target.spec) {
                            case data::ENTITY_SPEC_NPC_TOWNFOLK: {
                                //BroadcastEntitySay(victim.id, TextFormat("Ouch! You hit me with\nprojectile #%u!", entity.id));
                                BroadcastEntitySay(e_target.id, e_target.name, "Ouch!");
                                break;
                            }
                            case data::ENTITY_SPEC_NPC_CHICKEN: {
                                BroadcastEntitySay(e_target.id, e_target.name, "*squawk*!");
                                break;
                            }
                        }
                    }
                } else {
                    DespawnEntity(e_target.id);
                }
                DespawnEntity(e_projectile.id);
                lastCollisionA = projectileHitbox;
                lastCollisionB = targetHitbox;
            }
        }
    }
}
void GameServer::WarpEntity(data::Entity &entity, const std::string &dest_map_id, Vector3 dest_pos)
{
    data::Tilemap *dest_map = FindOrLoadMap(dest_map_id);
    if (dest_map) {
        entity.map_id = dest_map->id;
        entity.position = dest_pos;
        entity.force_accum = {};
        entity.velocity = {};
    } else {
        // TODO: This needs to ask GameServer to load the new map and
        // we also need to move our victim to the new map

        assert(!"TODO: Load map dynamically");
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
void GameServer::TickResolveEntityWarpCollisions(data::Tilemap &map, data::Entity &entity, double now)
{
    if (!entity.on_warp || entity.on_warp_cooldown) {
        return;
    }
    if (entity.type != data::ENTITY_PLAYER) {
        return;
    }

    data::ObjectData *obj_data = map.GetObjectData(entity.on_warp_coord.x, entity.on_warp_coord.y);
    if (obj_data) {
        assert(obj_data->type == "warp");
        Vector3 dest{};
        dest.x = obj_data->warp_dest_x * TILE_W + TILE_W * 0.5f;
        dest.y = obj_data->warp_dest_y * TILE_W + TILE_W - entity.radius; // * 0.5f;
        dest.z = obj_data->warp_dest_z;
        WarpEntity(entity, obj_data->warp_map_id, dest);
        entity.on_warp_cooldown = true;
    } else {
        TraceLog(LOG_WARNING, "We're on a warp, but there's no warp object found at that coord. Did it disappear?");
    }
}
void GameServer::Tick(void)
{
    // HACK: Only spawn NPCs in map 1, whatever map that may be (hopefully it's Level_001)
    TickSpawnTownNPCs(LEVEL_001);
    //TickSpawnCaveNPCs(2);

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
            case data::ENTITY_NPC:        TickEntityNPC        (entity, SV_TICK_DT); break;
            case data::ENTITY_PLAYER:     TickEntityPlayer     (entity, SV_TICK_DT); break;
            case data::ENTITY_PROJECTILE: TickEntityProjectile (entity, SV_TICK_DT); break;
        }

        data::Tilemap *map = FindMap(entity.map_id);
        if (map) {
            map->ResolveEntityCollisions(entity);
            TickResolveEntityWarpCollisions(*map, entity, now);
        }

        bool newlySpawned = entity.spawned_at == now;
        data::UpdateSprite(entity, SV_TICK_DT, newlySpawned);
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}
void GameServer::SerializeSnapshot(data::Entity &entity, Msg_S_EntitySnapshot &entitySnapshot)
{
    entitySnapshot.server_time = lastTickedAt;

    // Entity
    entitySnapshot.entity_id = entity.id;
    entitySnapshot.type      = entity.type;
    strncpy(entitySnapshot.map_id, entity.map_id.c_str(), SV_MAX_TILE_MAP_NAME_LEN);
    entitySnapshot.position  = entity.position;
    entitySnapshot.on_warp_cooldown = entity.on_warp_cooldown;

    // Collision
    //entitySnapshot.radius = entity.radius;

    // Life
    entitySnapshot.hp_max = entity.hp_max;
    entitySnapshot.hp     = entity.hp;

    // Physics
    //entitySnapshot.drag     = entity.drag;
    //entitySnapshot.speed    = entity.speed;
    entitySnapshot.velocity = entity.velocity;
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

        data::Tilemap *map = FindMap(entity->map_id);
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
                        SerializeSnapshot(entity, *msg);
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
