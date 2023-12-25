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

    Entity *player = SpawnEntity(ENTITY_PLAYER);
    if (player) {
        Tilemap &level_001 = packs[0].FindByName<Tilemap>(MAP_OVERWORLD);

        ServerPlayer &sv_player = players[clientIdx];
        sv_player.needsClockSync = true;
        sv_player.joinedAt = now;
        sv_player.entityId = player->id;

        player->type = ENTITY_PLAYER;
        player->map_id = level_001.id;
        const Vector3 caveEntrance{ 3100, 1100, 0 };
        const Vector3 townCenter{ 1660, 2360, 0 };
        const Vector3 inTree{ 2980.15f, 832.31f };
        player->position = caveEntrance;
        player->radius = 16;
        player->hp_max = 100;
        player->hp = player->hp_max;
        player->speed = 3000;
        player->drag = 0.9f;
        player->sprite_id = packs[0].FindByName<Sprite>("sprite_chr_mage").id;
        //projectile->direction = DIR_E;  // what's it do if it defaults to North?

        TileChunkRecord mainMap{};
        sv_player.chunkList.push(mainMap);

        BroadcastEntitySpawn(player->id);
        SendTitleShow(clientIdx, level_001.title);
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
    Tilemap &map = *maps[0];
#endif

    ServerPlayer &serverPlayer = players[clientIdx];
    DespawnEntity(serverPlayer.entityId);
    serverPlayer = {};
}

uint32_t GameServer::GetPlayerEntityId(uint32_t clientIdx)
{
    return clientIdx + 1;
}

ServerPlayer *GameServer::FindServerPlayer(uint32_t entity_id, int *client_idx)
{
    for (int i = 0; i < ARRAY_SIZE(players); i++) {
        ServerPlayer &player = players[i];
        if (player.entityId == entity_id) {
            if (client_idx) {
                *client_idx = i;
            }
            return &player;
        }
    }
    return 0;
}

void ProtoDb::Load(void)
{
    // TODO: Load protos from an mdesk file
    if (npc_lily.type) {
        return;
    }

    npc_lily.type = ENTITY_NPC;
    npc_lily.spec = ENTITY_SPEC_NPC_TOWNFOLK;
    npc_lily.name = "Lily";
    npc_lily.radius = 16;
    npc_lily.dialog_root_key = "DIALOG_LILY_INTRO";
    npc_lily.hp_max = 777;
    npc_lily.path_id = 1;
    npc_lily.speed_min = 300;
    npc_lily.speed_max = 600;
    npc_lily.drag = 1.0f;
    npc_lily.sprite_id = packs[0].FindByName<Sprite>("sprite_npc_lily").id;
    npc_lily.direction = DIR_E;

    npc_freye.type = ENTITY_NPC;
    npc_freye.spec = ENTITY_SPEC_NPC_TOWNFOLK;
    npc_freye.name = "Freye";
    npc_freye.radius = 16;
    npc_freye.dialog_root_key = "DIALOG_FREYE_INTRO";
    npc_freye.hp_max = 200;
    npc_freye.path_id = 1;
    npc_freye.speed_min = 300;
    npc_freye.speed_max = 600;
    npc_freye.drag = 1.0f;
    npc_freye.sprite_id = packs[0].FindByName<Sprite>("sprite_npc_freye").id;
    npc_freye.direction = DIR_E;

    npc_nessa.type = ENTITY_NPC;
    npc_nessa.spec = ENTITY_SPEC_NPC_TOWNFOLK;
    npc_nessa.name = "Nessa";
    npc_nessa.radius = 16;
    npc_nessa.dialog_root_key = "DIALOG_NESSA_INTRO";
    npc_nessa.hp_max = 150;
    npc_nessa.path_id = 1;
    npc_nessa.speed_min = 300;
    npc_nessa.speed_max = 600;
    npc_nessa.drag = 1.0f;
    npc_nessa.sprite_id = packs[0].FindByName<Sprite>("sprite_npc_nessa").id;
    npc_nessa.direction = DIR_E;

    npc_elane.type = ENTITY_NPC;
    npc_elane.spec = ENTITY_SPEC_NPC_TOWNFOLK;
    npc_elane.name = "Elane";
    npc_elane.radius = 16;
    npc_elane.dialog_root_key = "DIALOG_ELANE_INTRO";
    npc_elane.hp_max = 100;
    npc_elane.path_id = 1;
    npc_elane.speed_min = 300;
    npc_elane.speed_max = 600;
    npc_elane.drag = 1.0f;
    npc_elane.sprite_id = packs[0].FindByName<Sprite>("sprite_npc_elane").id;
    npc_elane.direction = DIR_E;

    npc_chicken.type = ENTITY_NPC;
    npc_chicken.spec = ENTITY_SPEC_NPC_CHICKEN;
    npc_chicken.name = "Chicken";
    npc_chicken.ambient_fx = "sfx_chicken_cluck";
    npc_chicken.ambient_fx_delay_min = 15;
    npc_chicken.ambient_fx_delay_max = 30;
    npc_chicken.radius = 5;
    npc_chicken.hp_max = 15;
    npc_chicken.speed_min = 50;
    npc_chicken.speed_max = 150;
    npc_chicken.drag = 1.0f;
    npc_chicken.sprite_id = packs[0].FindByName<Sprite>("sprite_npc_chicken").id;

    itm_poop.type = ENTITY_ITEM;
    itm_poop.spec = ENTITY_SPEC_ITM_NORMAL;
    itm_poop.name = "Chicken Poop";
    itm_poop.radius = 5;
    itm_poop.drag = 1.0f;
    itm_poop.sprite_id = packs[0].FindByName<Sprite>("sprite_itm_poop").id;
}

Err GameServer::Start(void)
{
    protoDb.Load();

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

Tilemap *GameServer::FindOrLoadMap(uint16_t map_id)
{
#if 1
    // TODO: Go back to assuming it's not already loaded once we figure out packs
    Tilemap &map = packs[0].FindById<Tilemap>(map_id);
    if (map.id) {
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
#endif
}
Tilemap *GameServer::FindMap(uint16_t map_id)
{
    // TODO: Remove this alias and call * directly?
    auto &tile_map = packs[0].FindById<Tilemap>(map_id);
    return &tile_map;
}

Entity *GameServer::SpawnEntity(EntityType type)
{
    uint32_t entityId = nextEntityId++;
    Entity *entity = entityDb->SpawnEntity(entityId, type, now);
    return entity;
}
Entity *GameServer::SpawnEntityProto(uint16_t map_id, Vector3 position, EntityProto &proto)
{
    assert(proto.type);

    Tilemap *map = FindMap(map_id);
    if (!map) return 0;

    Entity *entity = SpawnEntity(proto.type);
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
    entity->sprite_id            = proto.sprite_id;
    entity->direction            = proto.direction;

    AiPathNode *aiPathNode = map->GetPathNode(entity->path_id, 0);
    if (aiPathNode) {
        entity->position.x = aiPathNode->pos.x;
        entity->position.y = aiPathNode->pos.y;
    }

    return entity;
}
Entity *GameServer::SpawnProjectile(uint16_t map_id, Vector3 position, Vector2 direction, Vector3 initial_velocity)
{
    Entity *projectile = SpawnEntity(ENTITY_PROJECTILE);
    if (!projectile) return 0;

    projectile->spec = ENTITY_SPEC_PRJ_FIREBALL;

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

    projectile->sprite_id = packs[0].FindByName<Sprite>("sprite_prj_fireball").id;
    //projectile->direction = DIR_E;

    BroadcastEntitySpawn(projectile->id);
    return projectile;
}
void GameServer::WarpEntity(Entity &entity, uint16_t dest_map_id, Vector3 dest_pos)
{
    Tilemap *dest_map = FindOrLoadMap(dest_map_id);
    if (dest_map) {
        entity.map_id = dest_map->id;
        entity.position = dest_pos;
        entity.force_accum = {};
        entity.velocity = {};
        entity.last_moved_at = now;

        if (entity.type == ENTITY_PLAYER) {
            Tilemap &map = packs[0].FindById<Tilemap>(entity.map_id);
            if (map.title.size()) {
                int clientIdx = 0;
                ServerPlayer *player = FindServerPlayer(entity.id, &clientIdx);
                if (player) {
                    SendTitleShow(clientIdx, map.title);
                }
            }
        }
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
void GameServer::DespawnEntity(uint32_t entityId)
{
    if (entityDb->DespawnEntity(entityId, now)) {
        BroadcastEntityDespawn(entityId);

        Entity *entity = entityDb->FindEntity(entityId, true);

        // TODO: Generic loot table check
        if (entity && entity->spec == ENTITY_SPEC_NPC_CHICKEN && entity->hp == 0.0f) {
            Entity *chicken_poop = SpawnEntityProto(entity->map_id, entity->position, protoDb.itm_poop);
            if (chicken_poop) {
                BroadcastEntitySpawn(chicken_poop->id);
            }
        }
    }
}
void GameServer::DestroyDespawnedEntities(void)
{
    for (Entity &entity : entityDb->entities) {
        if (entity.type && entity.despawned_at) {
            assert(entity.id);
            entityDb->DestroyEntity(entity.id);
        }
    }
}

void GameServer::TickPlayers(void)
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
            Entity *player = entityDb->FindEntity(sv_player.entityId, ENTITY_PLAYER);
            if (player) {
                Vector3 move_force = input_cmd->GenerateMoveForce(player->speed);
                player->ApplyForce(move_force);

                if (input_cmd->fire) {
                    if (player->Attack(now)) {
                        Vector3 projectile_spawn_pos = player->position;
                        projectile_spawn_pos.z += 24;  // "hand" height
                        Entity *projectile = SpawnProjectile(player->map_id, projectile_spawn_pos, input_cmd->facing, player->velocity);
                        if (projectile) {
                            Vector3 recoil_force{};
                            recoil_force.x = -projectile->velocity.x;
                            recoil_force.y = -projectile->velocity.y;
                            player->ApplyForce(recoil_force);
                        }
                    }
                }
            } else {
                assert(0);
                printf("[game_server] Player entityId %u out of range. Cannot process inputCmd.\n", sv_player.entityId);
            }
        }
    }
}
void GameServer::TickSpawnTownNPCs(uint16_t map_id)
{
    static EntityProto *townfolk[]{
        &protoDb.npc_lily,
        &protoDb.npc_freye,
        &protoDb.npc_nessa,
        &protoDb.npc_elane
    };
    static uint32_t townfolk_ids[4];
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

    assert(ARRAY_SIZE(townfolk) == ARRAY_SIZE(townfolk_ids));

    const double townfolkSpawnRate = 2.0;
    if (now - lastTownfolkSpawnedAt >= townfolkSpawnRate) {
        for (int i = 0; i < ARRAY_SIZE(townfolk_ids); i++) {
            Entity *entity = entityDb->FindEntity(townfolk_ids[i], ENTITY_NPC);
            if (!entity) {
                entity = SpawnEntityProto(map_id, { 0, 0 }, *townfolk[i]);
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
            Entity *entity = entityDb->FindEntity(chicken_ids[i], ENTITY_NPC);
            if (!entity) {
                Vector3 spawn{};
                // TODO: If chicken spawns inside something, immediately despawn
                //       it (or find better strat for this)
                spawn.x = GetRandomValue(400, 2400);
                spawn.y = GetRandomValue(2000, 3400);
                entity = SpawnEntityProto(map_id, spawn, protoDb.npc_chicken);
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
void GameServer::TickSpawnCaveNPCs(uint16_t map_id)
{
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        Entity *entity = entityDb->FindEntity(eid_bots[i], ENTITY_NPC);
        if (!entity && ((int)tick % 100 == i * 10)) {
            entity = SpawnEntity(ENTITY_NPC);
            if (!entity) continue;

            entity->name = "Cave Lily";

            entity->map_id = map_id;
            entity->position = { 100, 100 };

            entity->radius = 10;

            entity->hp_max = 100;
            entity->hp = entity->hp_max;

            entity->speed = GetRandomValue(300, 600);
            entity->drag = 8.0f;

            entity->sprite_id = packs[0].FindByName<Sprite>("sprite_npc_lily").id;
            //entity->direction = DIR_E;

            BroadcastEntitySpawn(entity->id);
            eid_bots[i] = entity->id;
        }
    }
}
void GameServer::TickEntityNPC(Entity &e_npc, double dt, double now)
{
    Tilemap *map = FindMap(e_npc.map_id);
    if (!map) return;

    if (now - e_npc.dialog_spawned_at > SV_ENTITY_DIALOG_INTERESTED_DURATION) {
        e_npc.dialog_spawned_at = 0;
    }

    // TODO: tick_pathfind?
    if (e_npc.path_id && !e_npc.dialog_spawned_at) {
        AiPathNode *ai_path_node = map->GetPathNode(e_npc.path_id, e_npc.path_node_target);
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
    if (e_npc.spec == ENTITY_SPEC_NPC_CHICKEN) {
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

    entityDb->EntityTick(e_npc, dt, now);
}
void GameServer::TickEntityPlayer(Entity &e_player, double dt, double now)
{
    entityDb->EntityTick(e_player, dt, now);
}
void GameServer::TickEntityProjectile(Entity &e_projectile, double dt, double now)
{
    // Gravity
    //AspectPhysics &ePhysics = map.ePhysics[entityIndex];
    //playerEntity.ApplyForce({ 0, 5 });

    entityDb->EntityTick(e_projectile, dt, now);

    if (now - e_projectile.spawned_at > 1.0) {
        DespawnEntity(e_projectile.id);
    }

    if (e_projectile.despawned_at) {
        return;
    }

    for (Entity &e_target : entityDb->entities) {
        if (e_target.type == ENTITY_NPC
            && !e_target.despawned_at
            && e_target.Alive()
            && e_target.map_id == e_projectile.map_id)
        {
            assert(e_target.id);
            if (e_target.Dead()) {
                continue;
            }

            Rectangle projectileHitbox = e_projectile.GetSpriteRect();
            Rectangle targetHitbox = e_target.GetSpriteRect();
            if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                e_target.TakeDamage(GetRandomValue(3, 8));
                if (e_target.Alive()) {
                    if (!e_target.dialog_spawned_at) {
                        switch (e_target.spec) {
                            case ENTITY_SPEC_NPC_TOWNFOLK: {
                                //BroadcastEntitySay(victim.id, TextFormat("Ouch! You hit me with\nprojectile #%u!", entity.id));
                                BroadcastEntitySay(e_target.id, e_target.name, "Ouch!");
                                break;
                            }
                            case ENTITY_SPEC_NPC_CHICKEN: {
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
void GameServer::TickResolveEntityWarpCollisions(Tilemap &map, Entity &entity)
{
    if (!entity.on_warp || entity.on_warp_cooldown) {
        return;
    }
    if (entity.type != ENTITY_PLAYER) {
        return;
    }

    ObjectData *obj_data = map.GetObjectData(entity.on_warp_coord.x, entity.on_warp_coord.y);
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
    TickPlayers();

    // TODO: Only do this when the map loads or changes.
    for (Tilemap &map : packs[0].tile_maps) {
        map.UpdateEdges();
    }

    // HACK: Only spawn NPCs in map 1, whatever map that may be (hopefully it's Level_001)
    Tilemap &map_overworld = packs[0].FindByName<Tilemap>(MAP_OVERWORLD);
    Tilemap &map_cave = packs[0].FindByName<Tilemap>(MAP_CAVE);
    TickSpawnTownNPCs(map_overworld.id);
    TickSpawnCaveNPCs(map_cave.id);

    // Tick entites
    for (Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at) {
            continue;
        }
        assert(entity.id);

        // TODO: if (map.sleeping) continue

        switch (entity.type) {
            case ENTITY_NPC:        TickEntityNPC        (entity, SV_TICK_DT, now); break;
            case ENTITY_PLAYER:     TickEntityPlayer     (entity, SV_TICK_DT, now); break;
            case ENTITY_PROJECTILE: TickEntityProjectile (entity, SV_TICK_DT, now); break;
        }

        Tilemap *map = FindMap(entity.map_id);
        if (map) {
            map->ResolveEntityCollisionsEdges(entity);
            map->ResolveEntityCollisionsTriggers(entity);
            TickResolveEntityWarpCollisions(*map, entity);
        }

        bool newlySpawned = entity.spawned_at == now;
        UpdateSprite(entity, SV_TICK_DT, newlySpawned);
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}

void GameServer::SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn)
{
    assert(entityId);
    if (!entityId) return;

    Entity *entity = entityDb->FindEntity(entityId);
    assert(entity);
    if (!entity) return;

    entitySpawn.server_time = lastTickedAt;

    // Entity
    entitySpawn.entity_id = entity->id;
    entitySpawn.type      = entity->type;
    entitySpawn.spec      = entity->spec;
    strncpy(entitySpawn.name,   entity->name.c_str(), SV_MAX_ENTITY_NAME_LEN);
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
    entitySpawn.sprite_id = entity->sprite_id;
}
void GameServer::SendEntitySpawn(int clientIdx, uint32_t entityId)
{
    Entity *entity = entityDb->FindEntity(entityId);
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
    Entity *entity = entityDb->FindEntity(entityId, true);
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
void GameServer::SendEntitySay(int clientIdx, uint32_t entityId, uint16_t dialogId, const std::string &title, const std::string &message)
{
    // TODO: Send only if the client is nearby, or the message is a global event
    Entity *entity = entityDb->FindEntity(entityId);
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

void GameServer::SendTileChunk(int clientIdx, Tilemap &map, uint16_t x, uint16_t y)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_TILE_EVENT)) {
        Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yj_server->CreateMessage(clientIdx, MSG_S_TILE_CHUNK);
        if (msg) {
            TileChunk *chunk = (TileChunk *)yj_server->AllocateBlock(clientIdx, sizeof(TileChunk));

            msg->map_id = map.id;
            msg->x = x;
            msg->y = y;
            msg->w = MIN(map.width, SV_MAX_TILE_CHUNK_WIDTH);
            msg->h = MIN(map.height, SV_MAX_TILE_CHUNK_WIDTH);

            for (uint16_t ty = y; ty < msg->h; ty++) {
                for (uint16_t tx = x; tx < msg->w; tx++) {
                    const uint16_t index = ty * msg->w + tx;
                    map.AtTry(tx, ty, chunk->tile_ids[index]);
                    map.AtTry_Obj(tx, ty, chunk->object_ids[index]);
                }
            }

            yj_server->AttachBlockToMessage(clientIdx, msg, (uint8_t *)chunk, sizeof(TileChunk));
            yj_server->SendMessage(clientIdx, CHANNEL_R_TILE_EVENT, msg);
        }
    }
}
void GameServer::BroadcastTileChunk(Tilemap &map, uint16_t x, uint16_t y)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendTileChunk(clientIdx, map, x, y);
    }
}
void GameServer::SendTileUpdate(int clientIdx, Tilemap &map, uint16_t x, uint16_t y)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_TILE_EVENT)) {
        Msg_S_TileUpdate *msg = (Msg_S_TileUpdate *)yj_server->CreateMessage(clientIdx, MSG_S_TILE_UPDATE);
        if (msg) {
            msg->map_id = map.id;
            msg->x = x;
            msg->y = y;
            uint16_t tile_id{};
            uint16_t object_id{};
            map.AtTry(x, y, tile_id);
            map.AtTry_Obj(x, y, object_id);
            msg->tile_id = (uint16_t)tile_id;
            msg->object_id = (uint16_t)object_id;
            yj_server->SendMessage(clientIdx, CHANNEL_R_TILE_EVENT, msg);
        }
    }
}
void GameServer::BroadcastTileUpdate(Tilemap &map, uint16_t x, uint16_t y)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendTileUpdate(clientIdx, map, x, y);
    }
}
void GameServer::SendTitleShow(int clientIdx, const std::string &text)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_GLOBAL_EVENT)) {
        Msg_S_TitleShow *msg = (Msg_S_TitleShow *)yj_server->CreateMessage(clientIdx, MSG_S_TITLE_SHOW);
        if (msg) {
            strncpy(msg->text, text.c_str(), SV_MAX_TITLE_LEN);
            yj_server->SendMessage(clientIdx, CHANNEL_R_GLOBAL_EVENT, msg);
        }
    }
}

void GameServer::RequestDialog(int clientIdx, Entity &entity, Dialog &dialog)
{
    // Overridable by listeners
    uint16_t dialog_id = dialog.id;
    const std::string_view *msg = &dialog.msg;

    DialogListener listener = dialog_library.FindListenerByKey(dialog.key);
    if (listener) {
        ServerPlayer &player = players[clientIdx];
        uint16_t redirect_id = listener(player.entityId, entity.id, dialog.id);
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
    Entity *entity = entityDb->FindEntity(msg.entityId, ENTITY_NPC);
    if (entity) {
        ServerPlayer &player = players[clientIdx];
        Entity *e_player = entityDb->FindEntity(player.entityId, ENTITY_PLAYER);
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
    Entity *entity = entityDb->FindEntity(msg.entity_id, ENTITY_NPC);
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

    Entity *e_player = entityDb->FindEntity(player.entityId, ENTITY_PLAYER);
    if (msg.map_id != e_player->map_id) {
        // Wrong map, kick player?
        return;
    }

    // TODO: Figure out what tool player is holding
    //player.activeItem

    // TODO: Check if sv_player is allowed to actually interact with this
    // particular tile. E.g. are they even in the same map as it!?
    // Holding the right tool, proximity, etc.
    Tilemap *map = FindMap(msg.map_id);
    if (!map) {
        // Map not loaded, kick player?
        return;
    }

    Tilemap::Coord player_coord{};
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
            //printf("WAY too far\n");
        } else {
            // Player too far away, ignore request
            //printf("too far\n");
        }
        return;
    }

    uint16_t tile_id{};
    if (!map->AtTry(msg.x, msg.y, tile_id)) {
        // Tile at x/y is not a valid tile type.. hmm.. is it void?
        assert(!"not a valid tile");
        return;
    }

    //uint16_t object_id = map->At_Obj(msg.x, msg.y);
    ObjectData *obj_data = map->GetObjectData(msg.x, msg.y);

    if (msg.primary == false && obj_data) {
        if (obj_data->type == "lever") {
            if (obj_data->power_level) {
                map->Set_Obj(msg.x, msg.y, obj_data->tile_def_unpowered, now);
                obj_data->power_level = 0;
            } else {
                map->Set_Obj(msg.x, msg.y, obj_data->tile_def_powered, now);
                obj_data->power_level = 1;
            }
            BroadcastTileUpdate(*map, msg.x, msg.y);
        } else if (obj_data->type == "lootable") {
            SendEntitySay(clientIdx, player.entityId, 0, "Chest", TextFormat("loot_table_id: %u", obj_data->loot_table_id));
        } else if (obj_data->type == "sign") {
            const char *signText = TextFormat("%s\n%s\n%s\n%s",
                obj_data->sign_text[0].c_str(),
                obj_data->sign_text[1].c_str(),
                obj_data->sign_text[2].c_str(),
                obj_data->sign_text[3].c_str()
            );
            SendEntitySay(clientIdx, player.entityId, 0, "Sign", signText);
        } else if (obj_data->type == "warp") {
            Tilemap &map = packs[0].FindById<Tilemap>(obj_data->warp_map_id);
            const char *warpInfo = TextFormat("%s (%u, %u)",
                map.name.c_str(),
                obj_data->warp_dest_x,
                obj_data->warp_dest_y
            );
            SendEntitySay(clientIdx, player.entityId, 0, "Warp", warpInfo);
        }
    } else if (msg.primary) {
        const TileDef &tile_def = map->GetTileDef(tile_id);
        const char *new_tile_def_name = 0;
        if (tile_def.name == "til_water_dark") {
            new_tile_def_name = "til_stone_path";
        } else if (tile_def.name == "til_stone_path") {
            new_tile_def_name = "til_water_dark";
        }

        if (new_tile_def_name) {
            const TileDef &new_tile_def = packs[0].FindByName<TileDef>(new_tile_def_name);
            // TODO(perf): Make some kind of map from string -> tile_def_index in the map?
            // * OR * make the maps all have global tile def ids instead of local tile def ids
            if (new_tile_def.id) {
                map->Set(msg.x, msg.y, new_tile_def.id, now);
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

void GameServer::SerializeSnapshot(Entity &entity, Msg_S_EntitySnapshot &entitySnapshot)
{
    entitySnapshot.server_time = lastTickedAt;

    // Entity
    entitySnapshot.entity_id = entity.id;
    entitySnapshot.type      = entity.type;
    entitySnapshot.map_id    = entity.map_id;
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
        Entity *entity = entityDb->FindEntity(serverPlayer.entityId);
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

#if 0
        // TODO: Send a full chunk resync only when it makes sense (first login, change maps, changes > N, etc.)
        for (int tileChunkIdx = 0; tileChunkIdx < serverPlayer.chunkList.size(); tileChunkIdx++) {
            TileChunkRecord &chunkRec = serverPlayer.chunkList[tileChunkIdx];
            if (chunkRec.lastSentAt < map->chunkLastUpdatedAt) {
                SendTileChunk(clientIdx, *map, chunkRec.coord.x, chunkRec.coord.y);
                chunkRec.lastSentAt = now;
                printf("[game_server] sending chunk to client\n");
            }
        }
#endif

        for (const Tilemap::Coord &coord : map->dirtyTiles) {
            SendTileUpdate(clientIdx, *map, coord.x, coord.y);
        }

        // TODO: Send only the world state that's relevant to this particular client
        const bool fullSnapshot = serverPlayer.joinedAt == now;
        for (Entity &entity : entityDb->entities) {
            if (!entity.id || !entity.type || entity.despawned_at) {
                continue;
            }

            const bool owner = entity.id == serverPlayer.entityId;

            if (fullSnapshot) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        SerializeSpawn(entity.id, *msg);
                        // TODO: MSG_S_ACK_INPUT as unrealible msg? (keep max on receiving end)
                        if (owner) {
                            msg->last_processed_input_cmd = serverPlayer.lastInputSeq;
                        }
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
            }

            if (owner || entity.Active(now)) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                    if (msg) {
                        SerializeSnapshot(entity, *msg);
                        if (owner) {
                            msg->last_processed_input_cmd = serverPlayer.lastInputSeq;
                        }
                        yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
                    }
                }
            }
        }
    }

    for (Tilemap &map : packs[0].tile_maps) {
        map.dirtyTiles.clear();
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
