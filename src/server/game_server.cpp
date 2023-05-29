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

    Tilemap &map = *maps[0];
    uint32_t entityId = CreateEntity(map, Entity_Player);

    ServerPlayer &serverPlayer = players[clientIdx];
    serverPlayer.needsClockSync = true;
    serverPlayer.joinedAt = now;
    serverPlayer.entityId = entityId;

    size_t entityIndex = map.FindEntityIndex(entityId);

    Entity &entity = map.entities[entityIndex];
    AspectCollision &collision = map.collision[entityIndex];
    AspectLife &life = map.life[entityIndex];
    AspectPhysics &physics = map.physics[entityIndex];
    data::Sprite &sprite = map.sprite[entityIndex];

    entity.type = Entity_Player;
    entity.position = { 680, 1390 };
    collision.radius = 10;
    life.maxHealth = 100;
    life.health = life.maxHealth;
    physics.speed = 2000;
    physics.drag = 8.0f;
    sprite.anims[0] = data::GFX_ANIM_CHR_MAGE_N;

    SpawnEntity(entityId);

    TileChunkRecord mainMap{};
    serverPlayer.chunkList.push(mainMap);
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

Err GameServer::LoadMap(std::string filename)
{
    Err err = RN_SUCCESS;
    Tilemap *map = new Tilemap();
    do {
        if (!map) {
            err = RN_BAD_ALLOC;
            break;
        }

        err = map->Load(filename);
        if (err) {
            printf("Failed to load map %s with code %d\n", filename.c_str(), err);
            break;
        }

        map->id = nextMapId++;
        map->chunkLastUpdatedAt = now;
        mapsById[map->id] = maps.size();
        maps.push_back(map);
    } while (0);
    return err;
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
    //yojimbo::Address address("127.0.0.1", SV_PORT);
    //yojimbo::Address address("::1", SV_PORT);

    // Any interface
    //yojimbo::Address address("0.0.0.0", SV_PORT);
    //yojimbo::Address address("::", SV_PORT);

    //yojimbo::Address address("192.168.0.143", SV_PORT);
    yojimbo::Address address("68.9.219.64", SV_PORT);
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
Tilemap *GameServer::FindEntityMap(uint32_t entityId)
{
    const auto &mapIdEntry = entityMapId.find(entityId);
    if (mapIdEntry != entityMapId.end()) {
        size_t mapId = mapIdEntry->second;
        return FindMap(mapId);
    }
    return 0;
}
Entity *GameServer::FindEntity(uint32_t entityId)
{
    Tilemap *map = FindEntityMap(entityId);
    if (map) {
        Entity *entity = map->FindEntity(entityId);
        if (entity) {
            assert(entity->id == entityId);
            return entity;
        }
    }
    return {};
}

uint32_t GameServer::CreateEntity(Tilemap &map, EntityType type)
{
    uint32_t entityId = nextEntityId++;
    if (map.CreateEntity(entityId, type)) {
        entityMapId[entityId] = map.id;
        return entityId;
    } else {
        // TODO: EntityTypeStr
        assert(0);
        printf("[game_server] Failed to spawn entity type %u in map id %u\n", type, map.id);
    }
    return 0;
}
void GameServer::SpawnEntity(uint32_t entityId)
{
    Tilemap *map = FindEntityMap(entityId);
    if (map) {
        if (map->SpawnEntity(entityId, now)) {
            BroadcastEntitySpawn(*map, entityId);
        } else {
            assert(0);
            printf("[game_server] Failed to spawn entity id %u in map id %u\n", entityId, map->id);
        }
    } else {
        assert(0);
        printf("[game_server] Failed to spawn entity id %u, could not find map\n", entityId);
    }
}
void GameServer::DespawnEntity(uint32_t entityId)
{
    Tilemap *map = FindEntityMap(entityId);
    if (map) {
        if (map->DespawnEntity(entityId, now)) {
            printf("[game_server] DespawnEntity id %u\n", entityId);
            BroadcastEntityDespawn(*map, entityId);
        } else {
            assert(0);
            printf("[game_server] Failed to despawn entity id %u in map id %u\n", entityId, map->id);
        }
    } else {
        assert(0);
        printf("[game_server] Failed to despawn entity id %u, could not find map\n", entityId);
    }
}
void GameServer::DestroyDespawnedEntities(void)
{
    for (Tilemap *mapPtr : maps) {
        Tilemap &map = *mapPtr;
        for (int entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
            Entity &entity = map.entities[entityIndex];
            if (entity.type && entity.despawnedAt) {
                map.DestroyEntity(entity.id);
                entityMapId.erase(entity.id);
            }
        }
    }
}

void GameServer::SerializeSpawn(Tilemap &map, size_t entityIndex, Msg_S_EntitySpawn &entitySpawn)
{
    assert(entityIndex);

    Entity          &entity    = map.entities[entityIndex];
    AspectCollision &collision = map.collision[entityIndex];
    AspectPhysics   &physics   = map.physics[entityIndex];
    AspectLife      &life      = map.life[entityIndex];

    entitySpawn.serverTime = lastTickedAt;

    // Entity
    entitySpawn.mapId    = map.id;
    entitySpawn.entityId = entity.id;
    entitySpawn.type     = entity.type;
    entitySpawn.position = entity.position;

    // Collision
    entitySpawn.radius = collision.radius;

    // Physics
    entitySpawn.drag     = physics.drag;
    entitySpawn.speed    = physics.speed;
    entitySpawn.velocity = physics.velocity;

    // Life
    entitySpawn.maxHealth = life.maxHealth;
    entitySpawn.health    = life.health;

    //SPAWN_PROP(bulletSprite.spritesheetId);
}
void GameServer::SendEntitySpawn(int clientIdx, Tilemap &map, uint32_t entityId)
{
    size_t entityIndex = map.FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        assert(0);
        printf("[game_server] entity id %u has invalid index in map id %u. cannot send spawn msg.\n", entityId, map.id);
        return;
    }

    Entity entity = map.entities[entityIndex];
    if (!entity.id || !entity.type) {
        assert(!entity.id && !entity.type);
        assert(0);
        printf("[game_server] entity has invalid id %u or type %d. cannot send spawn msg.\n", entityId, entity.type);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
        if (msg) {
            SerializeSpawn(map, entityIndex, *msg);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntitySpawn(Tilemap &map, uint32_t entityId)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntitySpawn(clientIdx, map, entityId);
    }
}

void GameServer::SendEntityDespawn(int clientIdx, Tilemap &map, uint32_t entityId)
{
    size_t entityIndex = map.FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        assert(0);
        printf("[game_server] entity id %u has invalid index in map id %u. cannot send despawn msg.\n", entityId, map.id);
        return;
    }

    // TODO: Send only if the client is nearby, or the message is a global event
    Entity &entity = map.entities[entityIndex];
    if (!entity.id || !entity.type) {
        assert(!entity.id && !entity.type);
        assert(0);
        printf("[game_server] entity has invalid id %u or type %d. cannot send despawn msg.\n", entityId, entity.type);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
        if (msg) {
            msg->entityId = entityId;
            printf("[game_server][client %d] ENTITY_DESPAWN id %u\n", clientIdx, msg->entityId);
            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntityDespawn(Tilemap &map, uint32_t entityId)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntityDespawn(clientIdx, map, entityId);
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

void GameServer::SendEntitySay(int clientIdx, Tilemap &map, uint32_t entityId, uint32_t messageLength, const char *message)
{
    size_t entityIndex = map.FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        assert(0);
        printf("[game_server] entity id %u has invalid index in map id %u. cannot send entity say msg.\n", entityId, map.id);
        return;
    }

    // TODO: Send only if the client is nearby, or the message is a global event
    Entity &entity = map.entities[entityIndex];
    if (!entity.id || !entity.type) {
        assert(!entity.id && !entity.type);
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntitySay *msg = (Msg_S_EntitySay *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SAY);
        if (msg) {
            msg->entityId = entityId;
            msg->messageLength = messageLength;
            strncpy(msg->message, message, MAX(SV_MAX_ENTITY_SAY_MSG_LEN, messageLength));

            yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
        }
    }
}
void GameServer::BroadcastEntitySay(Tilemap &map, uint32_t entityId, uint32_t messageLength, const char *message)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntitySay(clientIdx, map, entityId, messageLength, message);
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
                        Entity *entity = FindEntity(msg->entityId);
                        if (entity && entity->type == Entity_NPC) {
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

uint32_t GameServer::SpawnProjectile(Tilemap &map, Vector2 position, Vector2 direction)
{
    uint32_t bulletId = CreateEntity(map, Entity_Projectile);
    if (bulletId) {
        size_t bulletIndex = map.FindEntityIndex(bulletId);
        Entity &bulletEntity = map.entities[bulletIndex];
        AspectPhysics &bulletPhysics = map.physics[bulletIndex];
        data::Sprite &bulletSprite = map.sprite[bulletIndex];

        // [Entity] position
        bulletEntity.position = position;

        // [Physics] shoot in facing bulletDirection
        Vector2 bulletDirection = Vector2Scale(direction, 100);
        // [Physics] add a bit of random spread
        bulletDirection.x += GetRandomValue(-20, 20);
        bulletDirection.y += GetRandomValue(-20, 20);
        bulletDirection = Vector2Normalize(bulletDirection);
        Vector2 bulletVelocity = Vector2Scale(bulletDirection, 200); //GetRandomValue(800, 1000));;
        // [Physics] random speed
        bulletPhysics.velocity = bulletVelocity;
        bulletPhysics.drag = 0.02f;

        // [Sprite] animation
        bulletSprite.anims[0] = data::GFX_ANIM_PRJ_BULLET;
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
            Tilemap *map = FindEntityMap(player.entityId);
            if (map) {
                size_t playerIndex = map->FindEntityIndex(player.entityId);
                if (playerIndex < SV_MAX_ENTITIES) {
                    AspectPhysics &playerPhysics = map->physics[playerIndex];
                    Vector2 moveForce = inputCmd->GenerateMoveForce(playerPhysics.speed);
                    playerPhysics.ApplyForce(moveForce);

                    if (inputCmd->fire) {
                        Entity &playerEntity = map->entities[playerIndex];
                        Vector2 projSpawnLoc{ playerEntity.position.x, playerEntity.position.y - 24 };
                        uint32_t bulletId = SpawnProjectile(*map, projSpawnLoc, inputCmd->facing);
                        if (bulletId) {
                            size_t bulletIndex = map->FindEntityIndex(bulletId);
                            const AspectPhysics &bulletPhysics = map->physics[bulletIndex];

                            // Recoil
                            Vector2 recoilForce = Vector2Negate(bulletPhysics.velocity);
                            playerPhysics.ApplyForce(recoilForce);

                            SpawnEntity(bulletId);
                        }
                    }
                } else {
                    assert(0);
                    printf("[game_server] Player entityId %u out of range. Cannot process inputCmd.\n", player.entityId);
                }
            } else {
                assert(0);
                printf("[game_server] Player entityId %u does not belong to a map!?\n", player.entityId);
            }
        }
    }
}
void GameServer::TickSpawnBots(Tilemap &map)
{
    static uint32_t eid_bots[10];
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        uint32_t entityId = eid_bots[i];

        // Make sure entity still exists
        if (entityId) {
            Entity *entity = map.FindEntity(entityId);
            if (!entity) {
                entityId = 0;
            }
        }

        if (!entityId && ((int)tick % 100 == i * 10)) {
            entityId = CreateEntity(map, Entity_NPC);
            if (entityId) {
                size_t entityIndex = map.FindEntityIndex(entityId);
                Entity &entity = map.entities[entityIndex];
                AspectCollision &collision = map.collision[entityIndex];
                AspectLife &life = map.life[entityIndex];
                AspectPathfind &pathfind = map.pathfind[entityIndex];
                AspectPhysics &physics = map.physics[entityIndex];
                data::Sprite &sprite = map.sprite[entityIndex];

                entity.type = Entity_NPC;

                collision.radius = 10;

                life.maxHealth = 100;
                life.health = life.maxHealth;

                pathfind.pathId = 0;
                AiPathNode *aiPathNode = map.GetPathNode(pathfind.pathId, 0);
                if (aiPathNode) {
                    entity.position = aiPathNode->pos;
                } else {
                    entity.position = { 0, 0 }; // TODO world spawn or something?
                }

                physics.speed = GetRandomValue(300, 600);
                physics.drag = 8.0f;

                sprite.anims[0] = data::GFX_ANIM_NPC_LILY_N;

                SpawnEntity(entityId);
                eid_bots[i] = entityId;
            }
        }
    }
}
void GameServer::TickEntityBot(Tilemap &map, uint32_t entityIndex, double dt)
{
    Entity &entity = map.entities[entityIndex];

    // TODO: tick_pathfind?
    AspectPathfind &pathfind = map.pathfind[entityIndex];
    AiPathNode *aiPathNode = map.GetPathNode(pathfind.pathId, pathfind.pathNodeTarget);
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
                pathfind.pathNodeTarget = map.GetNextPathNodeIndex(pathfind.pathId, pathfind.pathNodeTarget);
            }
        } else {
            pathfind.pathNodeLastArrivedAt = 0;
            pathfind.pathNodeArrivedAt = 0;

            AspectPhysics &physics = map.physics[entityIndex];
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

    map.EntityTick(entity.id, dt, now);
}
void GameServer::TickEntityPlayer(Tilemap &map, uint32_t entityIndex, double dt)
{
    Entity &entity = map.entities[entityIndex];
    map.EntityTick(entity.id, dt, now);
}
void GameServer::TickEntityProjectile(Tilemap &map, uint32_t entityIndex, double dt)
{
    Entity &entity = map.entities[entityIndex];

    // Gravity
    //AspectPhysics &physics = map.physics[entityIndex];
    //playerEntity.ApplyForce({ 0, 5 });

    map.EntityTick(entity.id, dt, now);

    if (now - entity.spawnedAt > 1.0) {
        DespawnEntity(entity.id);
    }
}
void GameServer::Tick(void)
{
    // HACK: Only spawn NPCs in map 0, whatever map that may be (hopefully it's Level_001)
    TickSpawnBots(*maps[0]);

    for (Tilemap *mapPtr : maps) {
        Tilemap &map = *mapPtr;

        // TODO: if (map.sleeping) continue

        // Tick entites
        for (int entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
            Entity &entity = map.entities[entityIndex];
            if (!entity.type || entity.despawnedAt) {
                continue;
            }

            switch (entity.type) {
                case Entity_Player:     TickEntityPlayer    (map, entityIndex, SV_TICK_DT); break;
                case Entity_NPC:        TickEntityBot       (map, entityIndex, SV_TICK_DT); break;
                case Entity_Projectile: TickEntityProjectile(map, entityIndex, SV_TICK_DT); break;
            }
            map.ResolveEntityTerrainCollisions(entity.id);
            map.ResolveEntityWarpCollisions(entity.id, now);

            data::Sprite &sprite = map.sprite[entityIndex];
            data::UpdateSprite(sprite, SV_TICK_DT);
        }

        for (int projectileId = 0; projectileId < SV_MAX_ENTITIES; projectileId++) {
            Entity &projectile = map.entities[projectileId];
            if (projectile.type == Entity_Projectile && !projectile.despawnedAt) {
                for (int targetIndex = 0; targetIndex < SV_MAX_ENTITIES && !projectile.despawnedAt; targetIndex++) {
                    Entity &target = map.entities[targetIndex];
                    if (target.type == Entity_NPC && !target.despawnedAt) {
                        AspectLife &life = map.life[targetIndex];
                        if (life.Dead()) continue;

                        Rectangle projectileHitbox = map.EntityRect(projectileId);
                        Rectangle targetHitbox = map.EntityRect(targetIndex);
                        if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                            life.TakeDamage(GetRandomValue(3, 8));
                            if (life.Alive()) {
                                const char *msg = "Ouch!";
                                BroadcastEntitySay(map, target.id, strlen(msg), msg);
                            } else {
                                DespawnEntity(target.id);
                            }
                            DespawnEntity(projectile.id);
                            lastCollisionA = projectileHitbox;
                            lastCollisionB = targetHitbox;
                        }
                    }
                }
            }
        }
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}
void GameServer::SerializeSnapshot(Tilemap &map, size_t entityIndex,
    Msg_S_EntitySnapshot &entitySnapshot, uint32_t lastProcessedInputCmd)
{
    assert(entityIndex);

    Entity          &entity    = map.entities[entityIndex];
    AspectCollision &collision = map.collision[entityIndex];
    AspectPhysics   &physics   = map.physics[entityIndex];
    AspectLife      &life      = map.life[entityIndex];

    entitySnapshot.serverTime = lastTickedAt;

    // Entity
    entitySnapshot.mapId    = map.id;
    entitySnapshot.entityId = entity.id;
    entitySnapshot.type     = entity.type;
    entitySnapshot.position = entity.position;

    // Collision
    //entitySnapshot.radius = collision.radius;

    // Physics
    //entitySnapshot.drag     = bulletPhysics.drag;
    entitySnapshot.speed    = physics.speed;
    entitySnapshot.velocity = physics.velocity;

    // Life
    entitySnapshot.maxHealth = life.maxHealth;
    entitySnapshot.health    = life.health;

    // Only for Entity_Player
    // TODO: Only send this to the player who actually owns this player playerEntity,
    //       otherwise we're leaking info about other players' connections.
    // clientIdx
    entitySnapshot.lastProcessedInputCmd = lastProcessedInputCmd;
}
void GameServer::SendClientSnapshots(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = players[clientIdx];
        Entity *entity = FindEntity(serverPlayer.entityId);
        if (!entity) {
            assert(0);
            printf("[game_server] could not find client id %d's entity id %u. cannot send snapshots\n", clientIdx, serverPlayer.entityId);
            continue;
        }

        Tilemap *map = FindEntityMap(serverPlayer.entityId);
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
        for (int entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
            Entity &entity = map->entities[entityIndex];
            if (!entity.id || !entity.type) {
                assert(!entity.id && !entity.type);  // why has one but not other!?
                continue;
            }

            if (serverPlayer.joinedAt == now && !entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        SerializeSpawn(*map, entityIndex, *msg);
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
                        SerializeSnapshot(*map, entityIndex, *msg, serverPlayer.lastInputSeq);
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
}
