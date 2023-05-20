#include "game_server.h"
#include "../common/net/messages/msg_s_entity_spawn.h"
#include "../common/net/messages/msg_s_entity_snapshot.h"

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

void tick_player(GameServer &server, uint32_t entityId, double dt)
{
    Tilemap &map = server.map;
    Entity &entity = map.entities[entityId];

    const InputCmd *inputCmd = 0;
    ServerPlayer &player = server.players[entityId - 1];
    for (int i = 0; i < player.inputQueue.size(); i++) {
        const InputCmd &cmd = player.inputQueue[i];
        if (cmd.seq > player.lastInputSeq) {
            inputCmd = &cmd;
            player.lastInputSeq = inputCmd->seq;
            //printf("Processed command %d\n", (int32_t)server.tick - (int32_t)inputCmd->seq);
            break;
        }
    }

    if (inputCmd) {
        AspectPhysics &physics = map.physics[entityId];
        Vector2 moveForce = inputCmd->GenerateMoveForce(physics.speed);
        entity.ApplyForce(map, entityId, moveForce);

        if (inputCmd->fire) {
            uint32_t idBullet = map.CreateEntity(Entity_Projectile);
            if (idBullet) {
                Entity &bulletEntity = map.entities[idBullet];
                AspectPhysics &bulletPhysics = map.physics[idBullet];

                bulletEntity.position = { entity.position.x, entity.position.y - 32 };
                // Shoot in facing direction
                Vector2 direction = Vector2Scale(inputCmd->facing, 100);
                // Add a bit of random spread
                direction.x += GetRandomValue(-20, 20);
                direction.y += GetRandomValue(-20, 20);
                direction = Vector2Normalize(direction);
                Vector2 bulletVelocity = Vector2Scale(direction, 400); //GetRandomValue(800, 1000));;
                // Random speed
                bulletPhysics.velocity = bulletVelocity;
                bulletPhysics.drag = 0.02f;
                server.SpawnEntity(idBullet);

                // Recoil
                Vector2 recoilForce = Vector2Negate(bulletVelocity);
                entity.ApplyForce(map, entityId, recoilForce);
            }
        }
    }

    entity.Tick(map, entityId, dt);
}

void tick_bot(GameServer &server, uint32_t entityId, double dt)
{
    Tilemap &map = server.map;
    Entity &entity = map.entities[entityId];

    // TODO: tick_pathfind?
    AspectPathfind &pathfind = map.pathfind[entityId];
    AiPathNode *aiPathNode = map.GetPathNode(pathfind.pathId, pathfind.pathNodeTarget);
    if (aiPathNode) {
        Vector2 target = aiPathNode->pos;
        Vector2 toTarget = Vector2Subtract(target, entity.position);
        if (Vector2LengthSqr(toTarget) < 10 * 10) {
            if (pathfind.pathNodeLastArrivedAt != pathfind.pathNodeTarget) {
                // Arrived at a new node
                pathfind.pathNodeLastArrivedAt = pathfind.pathNodeTarget;
                pathfind.pathNodeArrivedAt = server.now;
            }
            if (server.now - pathfind.pathNodeArrivedAt > aiPathNode->waitFor) {
                // Been at node long enough, move on
                pathfind.pathNodeTarget = map.GetNextPathNodeIndex(pathfind.pathId, pathfind.pathNodeTarget);
            }
        } else {
            pathfind.pathNodeLastArrivedAt = 0;
            pathfind.pathNodeArrivedAt = 0;

            AspectPhysics &physics = map.physics[entityId];
            Vector2 moveForce = toTarget;
            moveForce = Vector2Normalize(moveForce);
            moveForce = Vector2Scale(moveForce, physics.speed);
            entity.ApplyForce(map, entityId, moveForce);
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

    entity.Tick(map, entityId, dt);
}

void tick_projectile(GameServer &server, uint32_t entityId, double dt)
{
    Tilemap &map = server.map;
    Entity &entity = map.entities[entityId];

    //entity.ApplyForce({ 0, 5 });
    entity.Tick(map, entityId, dt);

    if (server.now - entity.spawnedAt > 1.0) {
        server.DespawnEntity(entityId);
    }
}

typedef void (*EntityTicker)(GameServer &server, uint32_t entityId, double dt);
static EntityTicker entity_ticker[Entity_Count] = {
    0,
    tick_player,
    tick_bot,
    tick_projectile,
};

void GameServer::OnClientJoin(int clientIdx)
{
    static const Color playerColors[]{
        MAROON,
        LIME,
        SKYBLUE
    };

    uint32_t entityId = GetPlayerEntityId(clientIdx);

    ServerPlayer &serverPlayer = players[clientIdx];
    serverPlayer.needsClockSync = true;
    serverPlayer.joinedAt = now;
    serverPlayer.entityId = entityId;

    Entity &entity = map.entities[entityId];
    AspectCollision &collision = map.collision[entityId];
    AspectLife &life = map.life[entityId];
    AspectPhysics &physics = map.physics[entityId];

    entity.type = Entity_Player;
    entity.position = { 680, 1390 };
    collision.radius = 10;
    life.maxHealth = 100;
    life.health = life.maxHealth;
    physics.speed = 2000;
    physics.drag = 8.0f;

    SpawnEntity(entityId);

    TileChunkRecord mainMap{};
    serverPlayer.chunkList.push(mainMap);
}

void GameServer::OnClientLeave(int clientIdx)
{
    ServerPlayer &serverPlayer = players[clientIdx];
    map.DestroyEntity(serverPlayer.entityId);
    serverPlayer = {};
}

Err GameServer::Start(void)
{
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return RN_NET_INIT_FAILED;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
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
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_R_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_ENTITY_EVENT].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.bandwidthSmoothingFactor = CL_BANDWIDTH_SMOOTHING_FACTOR;

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

void GameServer::SendEntitySpawn(int clientIdx, uint32_t entityId)
{
    // TODO: Send only if the client is nearby, or the message is a global event
    Entity &entity = map.entities[entityId];
    if (!entity.type) {
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

        SendEntitySpawn(clientIdx, entityId);
    }
}

void GameServer::SendEntityDespawn(int clientIdx, uint32_t entityId)
{
    // TODO: Send only if the client is nearby, or the message is a global event
    Entity &entity = map.entities[entityId];
    if (!entity.type) {
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

        SendEntityDespawn(clientIdx, entityId);
    }
}

void GameServer::SendEntitySay(int clientIdx, uint32_t entityId, uint32_t messageLength, const char *message)
{
    // TODO: Send only if the client is nearby, or the message is a global event
    Entity &entity = map.entities[entityId];
    if (!entity.type) {
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

void GameServer::BroadcastEntitySay(uint32_t entityId, uint32_t messageLength, const char *message)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendEntitySay(clientIdx, entityId, messageLength, message);
    }
}

void GameServer::SendTileChunk(int clientIdx, uint32_t x, uint32_t y)
{
    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_TILE_EVENT)) {
        Msg_S_TileChunk *msg = (Msg_S_TileChunk *)yj_server->CreateMessage(clientIdx, MSG_S_TILE_CHUNK);
        if (msg) {
            map.SV_SerializeChunk(*msg, x, y);
            yj_server->SendMessage(clientIdx, CHANNEL_R_TILE_EVENT, msg);
        }
    }
}

void GameServer::BroadcastTileChunk(uint32_t x, uint32_t y)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        SendTileChunk(clientIdx, x, y);
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
                        Entity *entity = map.GetEntity(msg->entityId);
                        if (entity && entity->type == Entity_NPC) {
                            //const char *text = TextFormat("Lily says: TPS is %d", (int)(1.0/SV_TICK_DT));
                            //SendEntitySay(clientIdx, msg->targetId, (uint32_t)strlen(text), text);
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
                        Tile tile{};
                        if (map.AtTry(msg->x, msg->y, tile)) {
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

void GameServer::Tick(void)
{
    // Spawn entities
    static uint32_t eid_bots[10];
    for (int i = 0; i < ARRAY_SIZE(eid_bots); i++) {
        uint32_t entityId = eid_bots[i];
        if (!map.GetEntity(entityId)) {
            entityId = 0;
        }
        if (!entityId && ((int)tick % 100 == i * 10)) {
            eid_bots[i] = map.CreateEntity(Entity_NPC);
            entityId = eid_bots[i];
            if (entityId) {
                Entity &entity = map.entities[entityId];
                AspectCollision &collision = map.collision[entityId];
                AspectLife &life = map.life[entityId];
                AspectPathfind &pathfind = map.pathfind[entityId];
                AspectPhysics &physics = map.physics[entityId];
                AspectSprite &sprite = map.sprite[entityId];

                life.maxHealth = 100;
                life.health = life.maxHealth;
                pathfind.pathId = 0;

                entity.type = Entity_NPC;
                collision.radius = 10;
                AiPathNode *aiPathNode = map.GetPathNode(pathfind.pathId, 0);
                if (aiPathNode) {
                    entity.position = aiPathNode->pos;
                } else {
                    entity.position = { 0, 0 }; // TODO world spawn or something?
                }
                physics.speed = GetRandomValue(1000, 5000);
                physics.drag = 8.0f;
                sprite.spritesheetId = STR_SHT_LILY;
                sprite.animationId = STR_NULL;
                SpawnEntity(entityId);
            }
        }
    }

    // Tick entites
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = map.entities[entityId];
        if (!entity.type || entity.despawnedAt) {
            continue;
        }

        AspectPhysics &physics = map.physics[entityId];
        physics.forceAccum = {};
        entity_ticker[entity.type](*this, entityId, SV_TICK_DT);
        map.ResolveEntityTerrainCollisions(entityId);
        map.ResolveEntityWarpCollisions(entityId, now);
    }

    for (int projectileId = 0; projectileId < SV_MAX_ENTITIES; projectileId++) {
        Entity &projectile = map.entities[projectileId];
        if (projectile.type == Entity_Projectile && !projectile.despawnedAt) {
            for (int targetId = 0; targetId < SV_MAX_ENTITIES && !projectile.despawnedAt; targetId++) {
                Entity &target = map.entities[targetId];
                if (target.type == Entity_NPC && !target.despawnedAt) {
                    AspectLife &life = map.life[targetId];
                    if (life.Dead()) continue;

                    Rectangle projectileHitbox = projectile.GetRect(map, projectileId);
                    Rectangle targetHitbox = target.GetRect(map, targetId);
                    if (CheckCollisionRecs(projectileHitbox, targetHitbox)) {
                        life.TakeDamage(GetRandomValue(3, 8));
                        if (life.Alive()) {
                            const char *msg = "Ouch!";
                            BroadcastEntitySay(targetId, strlen(msg), msg);
                        } else {
                            DespawnEntity(targetId);
                        }
                        DespawnEntity(projectileId);
                        lastCollisionA = projectileHitbox;
                        lastCollisionB = targetHitbox;
                    }
                }
            }
        }
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}

void GameServer::SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn)
{
    entitySpawn.serverTime = lastTickedAt;

    Entity          &entity    = map.entities[entityId];
    AspectCollision &collision = map.collision[entityId];
    AspectPhysics   &physics   = map.physics[entityId];
    AspectLife      &life      = map.life[entityId];

    // Entity
    entitySpawn.id       = entityId;
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

    //SPAWN_PROP(sprite.spritesheetId);
}

void GameServer::SerializeSnapshot(uint32_t entityId, Msg_S_EntitySnapshot &entitySnapshot, uint32_t lastProcessedInputCmd)
{
    entitySnapshot.serverTime = lastTickedAt;

    Entity          &entity    = map.entities[entityId];
    AspectCollision &collision = map.collision[entityId];
    AspectPhysics   &physics   = map.physics[entityId];
    AspectLife      &life      = map.life[entityId];

    // Entity
    entitySnapshot.id       = entityId;
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
    // TODO: Only send this to the player who actually owns this player entity,
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

        for (int tileChunkIdx = 0; tileChunkIdx < serverPlayer.chunkList.size(); tileChunkIdx++) {
            TileChunkRecord &chunkRec = serverPlayer.chunkList[tileChunkIdx];
            if (chunkRec.lastSentAt < map.chunkLastUpdatedAt) {
                SendTileChunk(clientIdx, chunkRec.coord.x, chunkRec.coord.y);
                chunkRec.lastSentAt = now;
                printf("Sending chunk to client\n");
            }
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = map.entities[entityId];
            if (!entity.type) {
                continue;
            }

            if (serverPlayer.joinedAt == now && !entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        SerializeSpawn(entityId, *msg);
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
            } else if (entity.despawnedAt == now) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
                    if (msg) {
                        msg->entityId = entityId;
                        yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
            } else if (!entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                    if (msg) {
                        SerializeSnapshot(entityId, *msg, serverPlayer.lastInputSeq);
                        yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
                    }
                }
            }
        }
    }
}

void GameServer::DestroyDespawnedEntities(void)
{
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = map.entities[entityId];
        if (entity.type && entity.despawnedAt) {
            map.DestroyEntity(entityId);
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

uint32_t GameServer::GetPlayerEntityId(uint32_t clientIdx)
{
    return clientIdx + 1;
}

void GameServer::SpawnEntity(uint32_t entityId)
{
    if (map.SpawnEntity(entityId, now)) {
        BroadcastEntitySpawn(entityId);
    }
}

void GameServer::DespawnEntity(uint32_t entityId)
{
    if (map.DespawnEntity(entityId, now)) {
        BroadcastEntityDespawn(entityId);
    }
}
