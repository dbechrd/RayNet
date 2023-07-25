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

    uint32_t entityId = SpawnEntity(data::ENTITY_PLAYER);
    if (entityId) {
        ServerPlayer &serverPlayer = players[clientIdx];
        serverPlayer.needsClockSync = true;
        serverPlayer.joinedAt = now;
        serverPlayer.entityId = entityId;

        size_t entityIndex = entityDb->FindEntityIndex(entityId);

        data::Entity &entity = entityDb->entities[entityIndex];
        data::AspectCollision &eCollision = entityDb->collision [entityIndex];
        data::AspectLife      &eLife      = entityDb->life      [entityIndex];
        data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
        data::AspectSprite    &eSprite    = entityDb->sprite    [entityIndex];

        entity.type = data::ENTITY_PLAYER;
        entity.mapId = maps[0]->id;
        const Vector2 caveEntrance{ 1650, 435 };
        const Vector2 townCenter{ 830, 1180 };
        entity.position = townCenter;
        eCollision.radius = 10;
        eLife.maxHealth = 100;
        eLife.health = eLife.maxHealth;
        ePhysics.speed = 1000;
        ePhysics.drag = 1.0f;
        eSprite.sprite = data::SPRITE_CHR_MAGE;
        //eSprite.direction = data::DIR_E;  // what's it do if it defaults to North?

        TileChunkRecord mainMap{};
        serverPlayer.chunkList.push(mainMap);

        BroadcastEntitySpawn(entityId);
    } else {
        assert(!"world full? hmm.. need to disconnect client somehow");
    }
}
void GameServer::OnClientLeave(int clientIdx)
{
    // TODO: Huh? What dis mean?
    if (!maps.size()) return;

    // TODO: Which map is this player currently on? Need to despawn them from that map.
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

uint32_t GameServer::SpawnEntity(data::EntityType type)
{
    uint32_t entityId = nextEntityId++;
    if (entityDb->SpawnEntity(entityId, type, now)) {
        return entityId;
    }
    return 0;
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
        if (entity.type && entity.despawnedAt) {
            assert(entity.id);
            entityDb->DestroyEntity(entity.id);
        }
    }
}

void GameServer::SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn)
{
    assert(entityId);
    if (!entityId) return;

    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    assert(entityIndex);
    if (!entityIndex) return;

    data::Entity          &entity     = entityDb->entities  [entityIndex];
    data::AspectCollision &eCollision = entityDb->collision [entityIndex];
    data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
    data::AspectLife      &eLife      = entityDb->life      [entityIndex];

    entitySpawn.serverTime = lastTickedAt;

    // Entity
    entitySpawn.entityId = entity.id;
    entitySpawn.type     = entity.type;
    entitySpawn.mapId    = entity.mapId;
    entitySpawn.position = entity.position;

    // Collision
    entitySpawn.radius = eCollision.radius;

    // Physics
    entitySpawn.drag     = ePhysics.drag;
    entitySpawn.speed    = ePhysics.speed;
    entitySpawn.velocity = ePhysics.velocity;

    // Life
    entitySpawn.maxHealth = eLife.maxHealth;
    entitySpawn.health    = eLife.health;

    //SPAWN_PROP(bulletSprite.spritesheetId);
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

void GameServer::SendEntitySay(int clientIdx, uint32_t entityId, std::string message)
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
            msg->entityId = entityId;
            msg->message = message.substr(0, SV_MAX_ENTITY_SAY_MSG_LEN);
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

        SendEntitySay(clientIdx, entityId, message);
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

                        // TODO: Check if player is allowed to actually interact with this
                        // particular entity. E.g. are they even in the same map as them!?
                        // Proximity, etc.
                        data::Entity *entity = entityDb->FindEntity(msg->entityId);
                        if (entity && entity->type == data::ENTITY_NPC) {
                            //const char *text = TextFormat("Lily says: TPS is %d", (int)(1.0/SV_TICK_DT));
                            //SendEntitySay(clientIdx, msg->targetIndex, (uint32_t)strlen(text), text);
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

                        // TODO: Check if player is allowed to actually interact with this
                        // particular tile. E.g. are they even in the same map as it!?
                        // Holding the right tool, proximity, etc.
                        Tilemap *map = FindMap(msg->mapId);
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

uint32_t GameServer::SpawnProjectile(uint32_t mapId, Vector2 position, Vector2 direction)
{
    uint32_t bulletId = SpawnEntity(data::ENTITY_PROJECTILE);
    if (bulletId) {
        size_t entityIndex = entityDb->FindEntityIndex(bulletId);
        data::Entity &entity = entityDb->entities[entityIndex];
        data::AspectPhysics &ePhysics = entityDb->physics [entityIndex];
        data::AspectSprite  &eSprite  = entityDb->sprite  [entityIndex];

        // [Entity] position
        entity.position = position;
        entity.mapId = mapId;

        // [Physics] shoot in facing direction
        Vector2 dir = Vector2Scale(direction, 100);
        // [Physics] add a bit of random spread
        dir.x += GetRandomValue(-20, 20);
        dir.y += GetRandomValue(-20, 20);
        dir = Vector2Normalize(dir);
        Vector2 velocity = Vector2Scale(dir, 200); //GetRandomValue(800, 1000));;
        // [Physics] random speed
        ePhysics.velocity = velocity;
        ePhysics.drag = 0.02f;

        eSprite.sprite = data::SPRITE_PRJ_FIREBALL;
        //eSprite.direction = data::DIR_E;

        BroadcastEntitySpawn(bulletId);
        return bulletId;
    }
    return 0;
}
void GameServer::UpdateServerPlayers(void)
{
    for (ServerPlayer &player : players) {
        const InputCmd *inputCmd = 0;
        for (int i = 0; i < player.inputQueue.size(); i++) {
            const InputCmd &cmd = player.inputQueue[i];
            if (cmd.seq > player.lastInputSeq) {
                inputCmd = &cmd;
                player.lastInputSeq = inputCmd->seq;
                //printf("Processed command %d\n", (int32_t)tick - (int32_t)inputCmd->seq);
                break;
            }
        }

        if (inputCmd) {
            size_t playerIndex = entityDb->FindEntityIndex(player.entityId);
            if (playerIndex) {
                data::AspectCombat &playerCombat   = entityDb->combat  [playerIndex];
                data::AspectPhysics &playerPhysics = entityDb->physics [playerIndex];

                Vector2 moveForce = inputCmd->GenerateMoveForce(playerPhysics.speed);
                playerPhysics.ApplyForce(moveForce);

                if (inputCmd->fire) {
                    data::Entity &playerEntity = entityDb->entities[playerIndex];
                    if (now - playerCombat.lastAttackedAt > playerCombat.attackCooldown) {
                        Vector2 projSpawnLoc{ playerEntity.position.x, playerEntity.position.y - 24 };
                        uint32_t bulletId = SpawnProjectile(playerEntity.mapId, projSpawnLoc, inputCmd->facing);
                        if (bulletId) {
                            size_t bulletIndex = entityDb->FindEntityIndex(bulletId);
                            const data::AspectPhysics &bulletPhysics = entityDb->physics[bulletIndex];

                            // Recoil
                            Vector2 recoilForce = Vector2Negate(bulletPhysics.velocity);
                            playerPhysics.ApplyForce(recoilForce);
                        }
                        playerCombat.lastAttackedAt = now;
                        playerCombat.attackCooldown = 0.3;
                    }
                }
            } else {
                assert(0);
                printf("[game_server] Player entityId %u out of range. Cannot process inputCmd.\n", player.entityId);
            }
        }
    }
}
void GameServer::TickSpawnTownNPCs(uint32_t mapId)
{
    static uint32_t eid_bots[1];
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        uint32_t entityId = eid_bots[i];

        // Make sure entity still exists
        if (entityId) {
            data::Entity *entity = entityDb->FindEntity(entityId);
            if (!entity) {
                entityId = 0;
            }
        }

        if (!entityId && ((int)tick % 100 == i * 10)) {
            entityId = SpawnEntity(data::ENTITY_NPC);
            if (entityId) {
                size_t entityIndex = entityDb->FindEntityIndex(entityId);
                data::Entity &entity = entityDb->entities[entityIndex];
                data::AspectCollision &eCollision = entityDb->collision [entityIndex];
                data::AspectLife      &eLife      = entityDb->life      [entityIndex];
                data::AspectPathfind  &ePathfind  = entityDb->pathfind  [entityIndex];
                data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
                data::AspectSprite    &eSprite    = entityDb->sprite    [entityIndex];

                entity.type = data::ENTITY_NPC;
                entity.mapId = mapId;
                entity.position = { 0, 0 };

                eCollision.radius = 10;

                eLife.maxHealth = 100;
                eLife.health = eLife.maxHealth;

                Tilemap *map = FindMap(mapId);
                if (map) {
                    ePathfind.active = false;
                    ePathfind.pathId = 0;
                    AiPathNode *aiPathNode = map->GetPathNode(ePathfind.pathId, 0);
                    if (aiPathNode) {
                        entity.position = aiPathNode->pos;
                    }
                }

                ePhysics.speed = GetRandomValue(300, 600);
                ePhysics.drag = 1.0f;

                eSprite.sprite = data::SPRITE_NPC_LILY;
                //eSprite.direction = data::DIR_E;

                BroadcastEntitySpawn(entityId);
                eid_bots[i] = entityId;
            }
        }
    }
}
void GameServer::TickSpawnCaveNPCs(uint32_t mapId)
{
    static uint32_t eid_bots[1];
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        uint32_t entityId = eid_bots[i];

        // Make sure entity still exists
        if (entityId) {
            data::Entity *entity = entityDb->FindEntity(entityId);
            if (!entity) {
                entityId = 0;
            }
        }

        if (!entityId && ((int)tick % 100 == i * 10)) {
            entityId = SpawnEntity(data::ENTITY_NPC);
            if (entityId) {
                size_t entityIndex = entityDb->FindEntityIndex(entityId);
                data::Entity &entity = entityDb->entities[entityIndex];
                data::AspectCollision &collision = entityDb->collision [entityIndex];
                data::AspectLife      &life      = entityDb->life      [entityIndex];
                data::AspectPathfind  &pathfind  = entityDb->pathfind  [entityIndex];
                data::AspectPhysics   &physics   = entityDb->physics   [entityIndex];
                data::AspectSprite    &sprite    = entityDb->sprite    [entityIndex];

                entity.type = data::ENTITY_NPC;
                entity.mapId = mapId;
                entity.position = { 1620, 450 };

                collision.radius = 10;

                life.maxHealth = 100;
                life.health = life.maxHealth;

                physics.speed = GetRandomValue(300, 600);
                physics.drag = 8.0f;

                sprite.sprite = data::SPRITE_NPC_LILY;
                //eSprite.direction = data::DIR_E;

                BroadcastEntitySpawn(entityId);
                eid_bots[i] = entityId;
            }
        }
    }
}
void GameServer::TickEntityBot(uint32_t entityId, double dt)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) return;

    data::Entity &entity = entityDb->entities[entityIndex];

    Tilemap *map = FindMap(entity.mapId);
    if (!map) return;

    // TODO: tick_pathfind?
    data::AspectPathfind &pathfind = entityDb->pathfind[entityIndex];
    if (pathfind.active) {
        AiPathNode *aiPathNode = map->GetPathNode(pathfind.pathId, pathfind.pathNodeTarget);
        if (aiPathNode) {
            Vector2 target = aiPathNode->pos;
            Vector2 toTarget = Vector2Subtract(target, entity.position);
            if (Vector2LengthSqr(toTarget) < 10 * 10) {
                if (pathfind.pathNodeLastArrivedAt != pathfind.pathNodeTarget) {
                    // Arrived at a new node
                    pathfind.pathNodeLastArrivedAt = pathfind.pathNodeTarget;
                    pathfind.pathNodeArrivedAt = now;
                }
                if (now - pathfind.pathNodeArrivedAt > aiPathNode->waitFor) {
                    // Been at node long enough, move on
                    pathfind.pathNodeTarget = map->GetNextPathNodeIndex(pathfind.pathId, pathfind.pathNodeTarget);
                }
            } else {
                pathfind.pathNodeLastArrivedAt = 0;
                pathfind.pathNodeArrivedAt = 0;

                data::AspectPhysics &physics = entityDb->physics[entityIndex];
                Vector2 moveForce = toTarget;
                moveForce = Vector2Normalize(moveForce);
                moveForce = Vector2Scale(moveForce, physics.speed);
                physics.ApplyForce(moveForce);
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

            Vector2 moveForce = aiCmd.GenerateMoveForce(eBot.speed);
    #else

    #endif
        }
    } else {
        printf("");
    }

    entityDb->EntityTick(entityId, dt);
}
void GameServer::TickEntityPlayer(uint32_t entityId, double dt)
{
    entityDb->EntityTick(entityId, dt);
}
void GameServer::TickEntityProjectile(uint32_t entityId, double dt)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    assert(entityIndex);

    // Gravity
    //AspectPhysics &ePhysics = map.ePhysics[entityIndex];
    //playerEntity.ApplyForce({ 0, 5 });

    entityDb->EntityTick(entityId, dt);

    data::Entity &entity = entityDb->entities[entityIndex];
    if (entity.despawnedAt) {
        return;
    }

    if (now - entity.spawnedAt > 1.0) {
        DespawnEntity(entity.id);
    }

    if (!entity.despawnedAt) {
        for (data::Entity &target : entityDb->entities) {
            if (target.type == data::ENTITY_NPC && !target.despawnedAt && target.mapId == entity.mapId) {
                assert(target.id);
                size_t targetIndex = entityDb->FindEntityIndex(target.id);
                data::AspectLife &life = entityDb->life[targetIndex];
                if (life.Dead()) {
                    continue;
                }

                Rectangle projectileHitbox = entityDb->EntityRect(entity.id);
                Rectangle targetHitbox = entityDb->EntityRect(target.id);
                if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                    life.TakeDamage(GetRandomValue(3, 8));
                    if (life.Alive()) {
                        BroadcastEntitySay(target.id, TextFormat("Ouch! You hit me with\nprojectile #%u!", entity.id));
                    } else {
                        DespawnEntity(target.id);
                    }
                    DespawnEntity(entity.id);
                    lastCollisionA = projectileHitbox;
                    lastCollisionB = targetHitbox;
                }
            }
        }
    }
}
void GameServer::WarpEntity(Tilemap &map, uint32_t entityId, data::AspectWarp &warp)
{
    assert(entityId);

    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) return;

    data::Entity &entity = entityDb->entities[entityIndex];
    data::AspectCollision &collision = entityDb->collision[entityIndex];

    if (warp.destMap.size()) {
        // TODO: We need to move our entity to the new map
        Tilemap *map = FindOrLoadMap(warp.destMap);
        if (map) {
            // TODO: Move entity to other map?
            entity.mapId = map->id;
            entity.position = warp.destPos;
            data::AspectPhysics &physics = entityDb->physics[entityIndex];
            physics.forceAccum = {};
            physics.velocity = {};
        } else {
            assert(!"UH-OH");
        }
    } else {
        // TODO: This needs to ask GameServer to load the new map and
        // we also need to move our entity to the new map

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
        // game server (e.g. mapId or mapTemplateId).
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

    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) return;

    data::Entity &entity = entityDb->entities[entityIndex];
    if (entity.type != data::ENTITY_PLAYER) {
        return;
    }

    data::AspectCollision &collision = entityDb->collision[entityIndex];
    if (!collision.radius) {
        return;
    }

    bool onWarp = false;
    for (data::Entity &warpEntity : entityDb->entities) {
        if (!warpEntity.id || warpEntity.despawnedAt || warpEntity.mapId != entity.mapId) {
            continue;
        }

        data::AspectWarp &warp = entityDb->warp[entityIndex];
        onWarp = dlb_CheckCollisionCircleRec(entity.position, collision.radius, warp.collider, 0);
        if (onWarp) {
            // De-bounce warps. Only allow warping after entity has moved off any warps
            // they may have just used to get here.
            if (!collision.onWarp) {
                WarpEntity(map, entityId, warp);
            }
            break;
        }
    }
    collision.onWarp = onWarp;
}
void GameServer::Tick(void)
{
    // HACK: Only spawn NPCs in map 1, whatever map that may be (hopefully it's Level_001)
    TickSpawnTownNPCs(1);
    TickSpawnCaveNPCs(2);

    // Tick entites
    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawnedAt) {
            continue;
        }
        assert(entity.id);

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        assert(entityIndex);

        // TODO: if (map.sleeping) continue

        switch (entity.type) {
            case data::ENTITY_NPC:        TickEntityBot        (entity.id, SV_TICK_DT); break;
            case data::ENTITY_PLAYER:     TickEntityPlayer     (entity.id, SV_TICK_DT); break;
            case data::ENTITY_PROJECTILE: TickEntityProjectile (entity.id, SV_TICK_DT); break;
        }

        Tilemap *map = FindMap(entity.mapId);
        if (map) {
            map->ResolveEntityTerrainCollisions(entity.id);
            TickResolveEntityWarpCollisions(*map, entity.id, now);
        }

        data::AspectSprite &eSprite = entityDb->sprite[entityIndex];
        data::AspectPhysics &ePhysics = entityDb->physics[entityIndex];

        bool newlySpawned = entity.spawnedAt == now;
        data::UpdateSprite(eSprite, entity.type, ePhysics.velocity, SV_TICK_DT, newlySpawned);
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}
void GameServer::SerializeSnapshot(uint32_t entityId, Msg_S_EntitySnapshot &entitySnapshot)
{
    assert(entityId);
    if (!entityId) return;

    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    assert(entityIndex);
    if (!entityIndex) return;

    data::Entity          &entity     = entityDb->entities  [entityIndex];
    data::AspectCollision &eCollision = entityDb->collision [entityIndex];
    data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
    data::AspectLife      &eLife      = entityDb->life      [entityIndex];

    entitySnapshot.serverTime = lastTickedAt;

    // Entity
    entitySnapshot.entityId = entity.id;
    entitySnapshot.type     = entity.type;
    entitySnapshot.mapId    = entity.mapId;
    entitySnapshot.position = entity.position;

    // Collision
    //entitySnapshot.radius = eCollision.radius;

    // Physics
    //entitySnapshot.drag     = ePhysics.drag;
    entitySnapshot.speed    = ePhysics.speed;
    entitySnapshot.velocity = ePhysics.velocity;

    // Life
    entitySnapshot.maxHealth = eLife.maxHealth;
    entitySnapshot.health    = eLife.health;
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

        Tilemap *map = FindMap(entity->mapId);
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

            if (serverPlayer.joinedAt == now && !entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        SerializeSpawn(entity.id, *msg);
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
           /* } else if (entity.despawnedAt == now) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
                    if (msg) {
                        msg->entityId = entity.id;
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }*/
            } else if (!entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                    if (msg) {
                        SerializeSnapshot(entity.id, *msg);
                        // TODO: We only need this for the player entity, not EVERY entity. Make it suck less.
                        msg->lastProcessedInputCmd = serverPlayer.lastInputSeq;
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
