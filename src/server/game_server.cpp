#include "game_server.h"
#include "../common/shared_lib.h"

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

                const char *dumbBulletMsg = "I'm a bullet, weeeeee!";
                server.BroadcastEntitySay(idBullet, (uint32_t)strlen(dumbBulletMsg), dumbBulletMsg);
            }
        }
    }

    ePlayer.Tick(server.now, SV_TICK_DT);
}

void tick_bot(GameServer &server, uint32_t entityId, double dt)
{
    Entity &entity = server.world->entities[entityId];
    EntityBot &bot = entity.data.bot;
    AiPathNode *aiPathNode = server.world->map.GetPathNode(bot.pathId, bot.pathNodeTarget);

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

    entity.Tick(server.now, SV_TICK_DT);
}

void tick_projectile(GameServer &server, uint32_t entityId, double dt)
{
    Entity &eProjectile = server.world->entities[entityId];
    //eProjectile.ApplyForce({ 0, 5 });
    eProjectile.Tick(server.now, dt);

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
    uint32_t entityId = world->GetPlayerEntityId(clientIdx);

    ServerPlayer &serverPlayer = world->players[clientIdx];
    serverPlayer.needsClockSync = true;
    serverPlayer.joinedAt = now;
    serverPlayer.entityId = entityId;

    static const Color colors[]{
        MAROON,
        LIME,
        SKYBLUE
    };

    Entity &entity = world->entities[serverPlayer.entityId];
    entity.type = Entity_Player;
    entity.color = colors[clientIdx % (sizeof(colors) / sizeof(colors[0]))];
    entity.size = { 32, 64 };
    entity.radius = 10;
    entity.position = { 680, 1390 };
    entity.speed = 100;
    entity.drag = 8.0f;
    entity.data.player.playerId = clientIdx;
    world->SpawnEntity(*this, entityId);
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
                            const char *text = TextFormat("Bob says: TPS is %d", (int)(1.0/SV_TICK_DT));
                            SendEntitySay(clientIdx, msg->entityId, (uint32_t)strlen(text), text);
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
    static uint32_t eid_bot1 = 0;
    if (!eid_bot1) {
        eid_bot1 = world->CreateEntity(Entity_Bot);
        if (eid_bot1) {
            Entity &entity = world->entities[eid_bot1];
            EntityBot &bot = entity.data.bot;
            bot.pathId = 0;

            entity.type = Entity_Bot;
            entity.color = DARKPURPLE;
            entity.size = { 32, 64 };
            entity.radius = 10;
            entity.position = world->map.GetPathNode(bot.pathId, 0)->pos;
            entity.speed = 30;
            entity.drag = 8.0f;
            world->SpawnEntity(*this, eid_bot1);
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

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = world->entities[entityId];
            if (!entity.type) {
                continue;
            }

            ServerPlayer &serverPlayer = world->players[clientIdx];
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

    // TODO(dlb): Calculate actual deltas
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
