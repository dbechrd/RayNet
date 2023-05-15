#include "game_server.h"

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
    Entity &ePlayer = server.world->entities[entityId];

    const InputCmd *inputCmd = 0;
    ServerPlayer &player = server.world->players[ePlayer.data.player.playerId];
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
        Vector2 moveForce = inputCmd->GenerateMoveForce(ePlayer.speed);
        ePlayer.ApplyForce(moveForce);

        if (inputCmd->fire) {
            uint32_t idBullet = server.world->CreateEntity(Entity_Projectile);
            if (idBullet) {
                Entity &eBullet = server.world->entities[idBullet];
                eBullet.color = ORANGE;
                eBullet.size = { 5, 5 };
                eBullet.position = { ePlayer.position.x, ePlayer.position.y - 32 };
                // Shoot in facing direction
                Vector2 direction = Vector2Scale(inputCmd->facing, 100);
                // Add a bit of random spread
                direction.x += GetRandomValue(-20, 20);
                direction.y += GetRandomValue(-20, 20);
                direction = Vector2Normalize(direction);
                // Random speed
                eBullet.velocity = Vector2Scale(direction, 400); //GetRandomValue(800, 1000));
                eBullet.drag = 0.02f;
                server.world->SpawnEntity(server, idBullet);
            }
        }
    }

    ePlayer.Tick(dt);
}

void tick_bot(GameServer &server, uint32_t entityId, double dt)
{
    Entity &entity = server.world->entities[entityId];
    EntityBot &bot = entity.data.bot;
    AiPathNode *aiPathNode = server.world->map.GetPathNode(bot.pathId, bot.pathNodeTarget);
    if (aiPathNode) {
        Vector2 target = aiPathNode->pos;
        Vector2 toTarget = Vector2Subtract(target, entity.position);
        if (Vector2LengthSqr(toTarget) < 10 * 10) {
            if (bot.pathNodeLastArrivedAt != bot.pathNodeTarget) {
                // Arrived at a new node
                bot.pathNodeLastArrivedAt = bot.pathNodeTarget;
                bot.pathNodeArrivedAt = server.now;
            }
            if (server.now - bot.pathNodeArrivedAt > aiPathNode->waitFor) {
                // Been at node long enough, move on
                bot.pathNodeTarget = server.world->map.GetNextPathNodeIndex(bot.pathId, bot.pathNodeTarget);
            }
        } else {
            bot.pathNodeLastArrivedAt = 0;
            bot.pathNodeArrivedAt = 0;

            Vector2 moveForce = toTarget;
            moveForce = Vector2Normalize(moveForce);
            moveForce = Vector2Scale(moveForce, entity.speed);
            entity.ApplyForce(moveForce);
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

    entity.Tick(dt);
}

void tick_projectile(GameServer &server, uint32_t entityId, double dt)
{
    Entity &eProjectile = server.world->entities[entityId];
    //eProjectile.ApplyForce({ 0, 5 });
    eProjectile.Tick(dt);

    if (server.now - eProjectile.spawnedAt > 1.0) {
        server.world->DespawnEntity(server, entityId);
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

    uint32_t entityId = world->GetPlayerEntityId(clientIdx);

    ServerPlayer &serverPlayer = world->players[clientIdx];
    serverPlayer.needsClockSync = true;
    serverPlayer.joinedAt = now;
    serverPlayer.entityId = entityId;

    Entity &entity = world->entities[serverPlayer.entityId];
    entity.type = Entity_Player;
    entity.color = playerColors[clientIdx % (sizeof(playerColors) / sizeof(playerColors[0]))];
    entity.size = { 32, 64 };
    entity.radius = 10;
    entity.position = { 680, 1390 };
    entity.speed = 2000;
    entity.drag = 8.0f;

    EntityPlayer &player = entity.data.player;
    player.playerId = clientIdx;
    player.life.maxHealth = 100;
    player.life.health = player.life.maxHealth;

    world->SpawnEntity(*this, entityId);

    TileChunkRecord mainMap{};
    serverPlayer.chunkList.push(mainMap);
}

void GameServer::OnClientLeave(int clientIdx)
{
    ServerPlayer &serverPlayer = world->players[clientIdx];
    Entity &entity = world->entities[serverPlayer.entityId];
    entity = {};
    serverPlayer = {};
    //serverPlayer.entity.color = GRAY;
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
    Entity &entity = world->entities[entityId];
    if (!entity.type) {
        return;
    }

    if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
        Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
        if (msg) {
            entity.Serialize(entityId, msg->entitySpawnEvent, lastTickedAt);
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
    Entity &entity = world->entities[entityId];
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
    Entity &entity = world->entities[entityId];
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
            world->map.SV_SerializeChunk(*msg, x, y);
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
                        Entity *entity = world->GetEntity(msg->entityId);
                        if (entity && entity->type == Entity_Bot) {
                            //const char *text = TextFormat("Lily says: TPS is %d", (int)(1.0/SV_TICK_DT));
                            //SendEntitySay(clientIdx, msg->entityId, (uint32_t)strlen(text), text);
                        }
                        break;
                    }
                    case MSG_C_INPUT_COMMANDS:
                    {
                        Msg_C_InputCommands *msg = (Msg_C_InputCommands *)yjMsg;
                        world->players[clientIdx].inputQueue = msg->cmdQueue;
                        break;
                    }
                    case MSG_C_TILE_INTERACT:
                    {
                        Msg_C_TileInteract *msg = (Msg_C_TileInteract *)yjMsg;
                        Tile tile{};
                        if (world->map.AtTry(msg->x, msg->y, tile)) {
                            //world->map.Set(msg->x, msg->y, 0);
#if 0
                            // TODO: If player is currently holding the magic tile inspector tool
                            uint32_t idLabel = world->CreateEntity(Entity_Projectile);
                            if (idLabel) {
                                Entity &eLabel = world->entities[idLabel];
                                eLabel.color = SKYBLUE;
                                eLabel.size = { 5, 5 };
                                eLabel.position = { (float)msg->x * TILE_W, (float)msg->y * TILE_W };
                                world->SpawnEntity(*this, idLabel);

                                const char *text = TextFormat("Tile type is %u", tile);
                                SendEntitySay(clientIdx, idLabel, (uint32_t)strlen(text), text);
                            }
#endif
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
        if (!world->GetEntity(eid_bots[i])) {
            eid_bots[i] = 0;
        }
        if (!eid_bots[i] && ((int)tick % 100 == i * 10)) {
            eid_bots[i] = world->CreateEntity(Entity_Bot);
            if (eid_bots[i]) {
                Entity *entity = world->GetEntity(eid_bots[i]);
                EntityBot &bot = entity->data.bot;
                bot.life.maxHealth = 100;
                bot.life.health = bot.life.maxHealth;
                bot.pathId = 0;

                entity->type = Entity_Bot;
                entity->color = DARKPURPLE;
                entity->size = { 32, 64 };
                entity->radius = 10;
                AiPathNode *aiPathNode = world->map.GetPathNode(bot.pathId, 0);
                if (aiPathNode) {
                    entity->position = aiPathNode->pos;
                } else {
                    entity->position = { 0, 0 }; // TODO world spawn or something?
                }
                entity->speed = GetRandomValue(1000, 5000);
                entity->drag = 8.0f;
                world->SpawnEntity(*this, eid_bots[i]);
            }
        }
    }

    // Tick entites
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = world->entities[entityId];
        if (!entity.type || entity.despawnedAt) {
            continue;
        }

        // TODO(dlb): Where should this live?
        entity.forceAccum = {};
        entity_ticker[entity.type](*this, entityId, SV_TICK_DT);
        world->map.ResolveEntityTerrainCollisions(entity);
        world->map.ResolveEntityWarpCollisions(entity, now);
    }

    for (int projectileId = 0; projectileId < SV_MAX_ENTITIES; projectileId++) {
        Entity &projectile = world->entities[projectileId];
        if (projectile.type == Entity_Projectile && !projectile.despawnedAt) {
            for (int targetId = 0; targetId < SV_MAX_ENTITIES && !projectile.despawnedAt; targetId++) {
                Entity &target = world->entities[targetId];
                if (target.type == Entity_Bot && !target.despawnedAt) {
                    EntityLife &life = target.data.bot.life;
                    if (life.Dead()) continue;

                    Rectangle projectileHitbox{
                        projectile.position.x - projectile.size.x / 2,
                        projectile.position.y - projectile.size.y,
                        projectile.size.x,
                        projectile.size.y
                    };
                    Rectangle botHitbox{
                        target.position.x - target.size.x / 2,
                        target.position.y - target.size.y,
                        target.size.x,
                        target.size.y
                    };
                    if (CheckCollisionRecs(projectileHitbox, botHitbox)) {
                        life.TakeDamage(GetRandomValue(3, 8));
                        if (life.Alive()) {
                            const char *msg = "Ouch!";
                            BroadcastEntitySay(targetId, strlen(msg), msg);
                        } else {
                            world->DespawnEntity(*this, targetId);
                        }
                        world->DespawnEntity(*this, projectileId);
                        lastCollisionA = projectileHitbox;
                        lastCollisionB = botHitbox;
                    }
                }
            }
        }
    }

    tick++;
    lastTickedAt = yj_server->GetTime();
}

void GameServer::SendClientSnapshots(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = world->players[clientIdx];

        for (int tileChunkIdx = 0; tileChunkIdx < serverPlayer.chunkList.size(); tileChunkIdx++) {
            TileChunkRecord &chunkRec = serverPlayer.chunkList[tileChunkIdx];
            if (chunkRec.lastSentAt < world->map.chunkLastUpdatedAt) {
                SendTileChunk(clientIdx, chunkRec.coord.x, chunkRec.coord.y);
                chunkRec.lastSentAt = now;
                printf("Sending chunk to client\n");
            }
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = world->entities[entityId];
            if (!entity.type) {
                continue;
            }

            if (serverPlayer.joinedAt == now && !entity.despawnedAt) {
                if (yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        entity.Serialize(entityId, msg->entitySpawnEvent, lastTickedAt);
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
                        entity.Serialize(entityId, msg->entitySnapshot, lastTickedAt, serverPlayer.lastInputSeq);
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
        Entity &entity = world->entities[entityId];
        if (entity.type && entity.despawnedAt) {
            world->DestroyEntity(entityId);
        }
    }
}

void GameServer::SendClockSync(void)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = world->players[clientIdx];
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

    delete world;
    world = {};
}
