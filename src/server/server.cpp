#include "../common/shared_lib.h"
#include "../common/histogram.h"
#include "server_world.h"
#include <stack>
#include <cassert>

struct Cursor {
    int tileDefId;
} cursor{};

class ServerNetAdapter : public NetAdapter
{
public:
    struct Server *server;

    ServerNetAdapter(Server *server)
    {
        this->server = server;
    }

    void OnServerClientConnected(int clientIdx) override;
    void OnServerClientDisconnected(int clientIdx) override;
};

struct Server {
    ServerNetAdapter adapter{ this };
    yojimbo::Server *yj_server{};

    uint64_t tick{};
    double tickAccum{};
    double lastTickedAt{};
    double now{};

    ServerWorld *world{};

    void OnClientJoin(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        serverPlayer.needsClockSync = true;
        serverPlayer.joinedAt = now;
        serverPlayer.entityId = clientIdx;  // TODO alloc index, or keep all clients in lower indices??

        static const Color colors[]{
            MAROON,
            LIME,
            SKYBLUE
        };

        Entity &entity = world->entities[serverPlayer.entityId];
        entity.type = Entity_Player;
        entity.index = clientIdx;
        entity.color = colors[clientIdx % (sizeof(colors) / sizeof(colors[0]))];
        entity.size = { 32, 64 };
        entity.radius = 10;
        entity.position = { 680, 1390 };
        entity.speed = 100;
        entity.drag = 8.0f;
        world->SpawnEntity(entity.index, now);
    }

    void OnClientLeave(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        Entity &entity = world->entities[serverPlayer.entityId];
        entity = {};
        serverPlayer = {};
        //serverPlayer.entity.color = GRAY;
    }
};

void ServerNetAdapter::OnServerClientConnected(int clientIdx)
{
    server->OnClientJoin(clientIdx);
}

void ServerNetAdapter::OnServerClientDisconnected(int clientIdx)
{
    server->OnClientLeave(clientIdx);
}

int ServerStart(Server *server)
{
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return -1;
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
    config.channel[CHANNEL_R_CLOCK_SYNC     ].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS ].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_ENTITY_EVENT   ].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
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
        return -1;
    }
    server->yj_server = new yojimbo::Server(
        yojimbo::GetDefaultAllocator(),
        privateKey,
        address,
        config,
        server->adapter,
        GetTime()
    );

    // NOTE(dlb): This must be the same size as world->players[] array!
    server->yj_server->Start(SV_MAX_PLAYERS);
    if (!server->yj_server->IsRunning()) {
        printf("yj: Failed to start server\n");
        return -1;
    }
#if 0
    printf("yj: started server on port %d (insecure)\n", SV_PORT);
    char addressString[256];
    server->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: server address is %s\n", addressString);
#endif

    return 0;
}

void ServerProcessMessages(Server &server)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!server.yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
            yojimbo::Message *message = server.yj_server->ReceiveMessage(clientIdx, channelIdx);
            while (message) {
                switch (message->GetType()) {
                    case MSG_C_INPUT_COMMANDS:
                    {
                        Msg_C_InputCommands *msgPlayerInput = (Msg_C_InputCommands *)message;
                        server.world->players[clientIdx].inputQueue = msgPlayerInput->cmdQueue;
                        break;
                    }
                    case MSG_C_ENTITY_INTERACT:
                    {
                        Msg_C_EntityInteract *msgEntityInteract = (Msg_C_EntityInteract *)message;
                        Entity *entity = server.world->GetEntity(msgEntityInteract->entityId);
                        if (entity) {
                            entity->position.x += 50;
                        }
                        break;
                    }
                }
                server.yj_server->ReleaseMessage(clientIdx, message);
                message = server.yj_server->ReceiveMessage(clientIdx, channelIdx);
            }
        }
    }
}

void tick_player(Server &server, uint32_t entityId, double now, double dt)
{
    Entity &ePlayer = server.world->entities[entityId];
    if (!server.yj_server->IsClientConnected(ePlayer.index)) {
        return;
    }

    const InputCmd *inputCmd = 0;
    ServerPlayer &player = server.world->players[ePlayer.index];
    for (int i = 0; i < player.inputQueue.size(); i++) {
        const InputCmd &cmd = player.inputQueue[i];
        if (cmd.seq > player.lastInputSeq) {
            inputCmd = &cmd;
            player.lastInputSeq = inputCmd->seq;
            //printf("Processed command %d\n", (int32_t)server.tick - (int32_t)nextCmd->seq);
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
                server.world->SpawnEntity(idBullet, now);
            }
        }
    }

    ePlayer.Tick(SV_TICK_DT);
}

void tick_bot(Server &server, uint32_t entityId, double now, double dt)
{
    Entity &entity = server.world->entities[entityId];
    EntityBot &bot = entity.data.bot;
    AiPathNode *aiPathNode = server.world->map.GetPathNode(bot.pathId, bot.pathNodeIndexTarget);

    Vector2 target = aiPathNode->pos;
    Vector2 toTarget = Vector2Subtract(target, entity.position);
    if (Vector2LengthSqr(toTarget) < 10*10) {
        if (bot.pathNodeIndexTarget != bot.pathNodeIndexPrev) {
            // Arrived at a new node
            bot.pathNodeIndexPrev = bot.pathNodeIndexTarget;
            bot.pathNodeArrivedAt = now;
        }
        if (now - bot.pathNodeArrivedAt > aiPathNode->waitFor) {
            // Been at node long enough, move on
            bot.pathNodeIndexTarget = server.world->map.GetNextPathNodeIndex(bot.pathId, bot.pathNodeIndexTarget);
        }
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
    if (bot.pathNodeIndexTarget != bot.pathNodeIndexPrev) {
        Vector2 moveForce = toTarget;
        moveForce = Vector2Normalize(moveForce);
        moveForce = Vector2Scale(moveForce, entity.speed);
        entity.ApplyForce(moveForce);
    }
#endif

    entity.Tick(SV_TICK_DT);
}

void tick_projectile(Server &server, uint32_t entityId, double now, double dt)
{
    Entity &eProjectile = server.world->entities[entityId];
    //eProjectile.ApplyForce({ 0, 5 });
    eProjectile.Tick(dt);

    if (now - eProjectile.spawnedAt > 1.0 || Vector2LengthSqr(eProjectile.velocity) < 100*100) {
        server.world->DespawnEntity(entityId, now);
    }
}

typedef void (*EntityTicker)(Server &server, uint32_t entityId, double now, double dt);
static EntityTicker entity_ticker[Entity_Count] = {
    0,
    tick_player,
    tick_bot,
    tick_projectile,
};

void ServerTick(Server &server)
{
    // Spawn entities
    static uint32_t eid_bot1 = 0;
    if (!eid_bot1) {
        eid_bot1 = server.world->CreateEntity(Entity_Bot);
        if (eid_bot1) {
            Entity &entity = server.world->entities[eid_bot1];
            EntityBot &bot = entity.data.bot;
            bot.pathId = 0;

            entity.type = Entity_Bot;
            entity.color = DARKPURPLE;
            entity.size = { 32, 64 };
            entity.radius = 10;
            entity.position = server.world->map.GetPathNode(bot.pathId, 0)->pos;
            entity.speed = 30;
            entity.drag = 8.0f;
            server.world->SpawnEntity(eid_bot1, server.now);
        }
    }

    // Tick entites
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = server.world->entities[entityId];
        if (!entity.type || entity.despawnedAt) {
            continue;
        }

        // TODO(dlb): Where should this live?
        entity.forceAccum = {};
        entity_ticker[entity.type](server, entityId, server.now, SV_TICK_DT);
        server.world->map.ResolveEntityTerrainCollisions(entity);
    }

    server.tick++;
    server.lastTickedAt = server.yj_server->GetTime();
}


void ServerSendClientSnapshots(Server &server)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!server.yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (!entity.type) {
                continue;
            }

            ServerPlayer &serverPlayer = server.world->players[clientIdx];
            if ((serverPlayer.joinedAt == server.now || entity.spawnedAt == server.now) && !entity.despawnedAt) {
                if (server.yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntitySpawn *msg = (Msg_S_EntitySpawn *)server.yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SPAWN);
                    if (msg) {
                        entity.Serialize(entityId, msg->entitySpawnEvent, server.lastTickedAt, serverPlayer.lastInputSeq);
                        server.yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
            } else if (entity.despawnedAt == server.now) {
                if (server.yj_server->CanSendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT)) {
                    Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)server.yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
                    if (msg) {
                        msg->entityId = entityId;
                        server.yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_EVENT, msg);
                    }
                }
            } else if (!entity.despawnedAt) {
                if (server.yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
                    Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)server.yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                    if (msg) {
                        entity.Serialize(entityId, msg->entitySnapshot, server.lastTickedAt, serverPlayer.lastInputSeq);
                        server.yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
                    }
                }
            }
        }
    }
}

void ServerDestroyDespawnedEntities(Server &server)
{
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = server.world->entities[entityId];
        if (entity.type && entity.despawnedAt) {
            server.world->DestroyEntity(entityId);
        }
    }
}

void ServerSendClockSync(Server &server)
{
    for (int clientIdx = 0; clientIdx < SV_MAX_PLAYERS; clientIdx++) {
        if (!server.yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = server.world->players[clientIdx];
        if (serverPlayer.needsClockSync && server.yj_server->CanSendMessage(clientIdx, CHANNEL_R_CLOCK_SYNC)) {
            Msg_S_ClockSync *msg = (Msg_S_ClockSync *)server.yj_server->CreateMessage(clientIdx, MSG_S_CLOCK_SYNC);
            if (msg) {
                msg->serverTime = GetTime();
                server.yj_server->SendMessage(clientIdx, CHANNEL_R_CLOCK_SYNC, msg);
                serverPlayer.needsClockSync = false;
            }
        }
    }
}

void ServerUpdate(Server &server)
{
    if (!server.yj_server->IsRunning())
        return;

    server.yj_server->AdvanceTime(server.now);
    server.yj_server->ReceivePackets();
    ServerProcessMessages(server);

    bool hasDelta = false;
    while (server.tickAccum >= SV_TICK_DT) {
        ServerTick(server);
        server.tickAccum -= SV_TICK_DT;
        hasDelta = true;
    }

    // TODO(dlb): Calculate actual deltas
    if (hasDelta) {
        ServerSendClientSnapshots(server);
        ServerDestroyDespawnedEntities(server);
    }

    ServerSendClockSync(server);
    server.yj_server->SendPackets();
}

void ServerStop(Server &server)
{
    server.yj_server->Stop();

    delete server.world;
    server.world = {};
}

bool NeedsFill(Tilemap &map, int x, int y, int tileDefFill)
{
    Tile tile;
    if (map.AtTry(x, y, tile)) {
        return tile == tileDefFill;
    }
    return false;
}

void Scan(Tilemap &map, int lx, int rx, int y, int tileDefFill, std::stack<Tilemap::Coord> &stack)
{
    bool inSpan = false;
    for (int x = lx; x < rx; x++) {
        if (!NeedsFill(map, x, y, tileDefFill)) {
            inSpan = false;
        } else if (!inSpan) {
            stack.push({ x, y });
            inSpan = true;
        }
    }
}

void Fill(Tilemap &map, int x, int y, int tileDefId)
{
    int tileDefFill = map.At(x, y);
    if (tileDefFill == tileDefId) {
        return;
    }

    std::stack<Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Tilemap::Coord coord = stack.top();
        stack.pop();

        int lx = coord.x;
        int rx = coord.x;
        while (NeedsFill(map, lx - 1, coord.y, tileDefFill)) {
            map.Set(lx - 1, coord.y, tileDefId);
            lx -= 1;
        }
        while (NeedsFill(map, rx, coord.y, tileDefFill)) {
            map.Set(rx, coord.y, tileDefId);
            rx += 1;
        }
        Scan(map, lx, rx, coord.y - 1, tileDefFill, stack);
        Scan(map, lx, rx, coord.y + 1, tileDefFill, stack);
    }
}

static Sound sndSoftTick;
static Sound sndHardTick;

struct UIState {
    bool hover;
    bool down;
    bool clicked;
};

UIState UIButton(Font font, const char *text, Vector2 uiPosition, Vector2 &uiCursor)
{
    Vector2 position = Vector2Add(uiPosition, uiCursor);
    const Vector2 pad{ 8, 1 };
    const float cornerRoundness = 0.2f;
    const float cornerSegments = 4;
    const Vector2 lineThick{ 1.0f, 1.0f };

    Vector2 textSize = MeasureTextEx(font, text, font.baseSize, 1.0f);
    Vector2 buttonSize = textSize;
    buttonSize.x += pad.x * 2;
    buttonSize.y += pad.y * 2;

    Rectangle buttonRect = {
        position.x - lineThick.x,
        position.y - lineThick.y,
        buttonSize.x + lineThick.x * 2,
        buttonSize.y + lineThick.y * 2
    };
    if (lineThick.x || lineThick.y) {
        DrawRectangleRounded(
            buttonRect,
            cornerRoundness, cornerSegments, BLACK
        );
    }

    static const char *prevHover = 0;

    Color color = BLUE;
    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), buttonRect)) {
        state.hover = true;
        color = SKYBLUE;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            color = DARKBLUE;
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state.clicked = true;
        }
    }

    if (state.clicked) {
        PlaySound(sndHardTick);
    } else if (state.hover && text != prevHover) {
        PlaySound(sndSoftTick);
        prevHover = text;
    }

    float yOffset = (state.down ? 0 : -lineThick.y * 2);
    DrawRectangleRounded({ position.x, position.y + yOffset, buttonSize.x, buttonSize.y }, cornerRoundness, cornerSegments, color);
    DrawTextShadowEx(font, text, { position.x + pad.x, position.y + pad.y + yOffset }, font.baseSize, WHITE);

    uiCursor.x += buttonSize.x;
    return state;
}

void Play(Server &server)
{
    sndSoftTick = LoadSound("resources/soft_tick.wav");
    sndHardTick = LoadSound("resources/hard_tick.wav");

    Font fntHackBold20 = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    Err err = server.world->map.Load(LEVEL_001);
    if (err != RN_SUCCESS) {
        printf("Failed to load map with code %d\n", err);
        assert(!"oops");
        // TODO: Display error message on screen for N seconds or
        // until dismissed
    }

    Camera2D &camera2d = server.world->camera2d;
    Tilemap &map = server.world->map;

    /*Camera3D camera3d{};
    camera3d.position = {0, 0, 0};
    camera3d.target = {0, 0, -1};
    camera3d.up = {0, 1, 0};
    camera3d.fovy = 90;
    camera3d.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera3d, CAMERA_FREE);*/

    camera2d.zoom = 1.0f;

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    bool editorActive = false;
    const Vector2 uiMargin{ 8, 8 };
    const Vector2 uiPadding{ 8, 8 };
    const Vector2 uiPosition{ 380, 8 };
    const Rectangle editorRect{ uiPosition.x, uiPosition.y, 800, 80 };

    SetExitKey(0);

    while (!WindowShouldClose())
    {
        server.now = GetTime();
        frameDt = MIN(server.now - frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = server.now;

        server.tickAccum += frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;

        bool escape = IsKeyPressed(KEY_ESCAPE);

        if (IsKeyPressed(KEY_H)) {
            histogram.paused = !histogram.paused;
        }
        if (IsKeyPressed(KEY_V)) {
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
            }
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ZERO)) {
            camera2d.zoom = 1.0f;
        }
        if (IsKeyPressed(KEY_GRAVE)) {
            editorActive = !editorActive;
        }

        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

        if (editorActive) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f / camera2d.zoom);

                camera2d.target = Vector2Add(camera2d.target, delta);
            }

            // Zoom based on mouse wheel
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                // Get the world point that is under the mouse
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);

                // Set the offset to where the mouse is
                camera2d.offset = GetMousePosition();

                // Set the target to match, so that the camera maps the world space point
                // under the cursor to the screen space point under the cursor at any zoom
                camera2d.target = mouseWorldPos;

                // Zoom increment
                const float zoomIncrement = 0.125f;

                camera2d.zoom += (wheel * zoomIncrement * camera2d.zoom);
                if (camera2d.zoom < zoomIncrement) camera2d.zoom = zoomIncrement;
            }
        } else {
            //camera2d.offset = {};
            //camera2d.target = {};
            //camera2d.zoom = 1;
        }

        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera2d);

        while (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            ServerUpdate(server);
        }

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(BROWN);

        BeginMode2D(camera2d);

        // [World] Tilemap
        server.world->map.Draw(camera2d);

        // [World][Editor] Paths

        // Draw path edges
        for (uint32_t pathId = 0; pathId < server.world->map.pathCount; pathId++) {
            AiPath *path = server.world->map.GetPath(pathId);
            for (uint32_t pathNodeIndex = 0; pathNodeIndex < path->pathNodeIndexCount; pathNodeIndex++) {
                uint32_t nextPathNodeIndex = server.world->map.GetNextPathNodeIndex(pathId, pathNodeIndex);
                AiPathNode *pathNode = server.world->map.GetPathNode(pathId, pathNodeIndex);
                AiPathNode *nextPathNode = server.world->map.GetPathNode(pathId, nextPathNodeIndex);
                DrawLine(
                    pathNode->pos.x, pathNode->pos.y,
                    nextPathNode->pos.x, nextPathNode->pos.y,
                    LIGHTGRAY
                );
            }
        }

        static struct PathNodeDrag {
            bool active;
            uint32_t pathId;
            uint32_t pathNodeIndex;
            Vector2 startPosition;
        } aiPathNodeDrag{};

        // Draw path nodes
        const float pathRectRadius = 5;
        for (uint32_t pathId = 0; pathId < server.world->map.pathCount; pathId++) {
            AiPath *aipath = server.world->map.GetPath(pathId);
            for (uint32_t pathNodeIndex = 0; pathNodeIndex < aipath->pathNodeIndexCount; pathNodeIndex++) {
                AiPathNode *aiPathNode = server.world->map.GetPathNode(pathId, pathNodeIndex);

                Rectangle nodeRect{
                    aiPathNode->pos.x - pathRectRadius,
                    aiPathNode->pos.y - pathRectRadius,
                    pathRectRadius * 2,
                    pathRectRadius * 2
                };

                Color color = aiPathNode->waitFor ? BLUE : RED;
                bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, nodeRect);
                if (hover) {
                    color = aiPathNode->waitFor ? SKYBLUE : PINK;
                    bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                    if (down) {
                        color = aiPathNode->waitFor ? DARKBLUE : MAROON;
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            aiPathNodeDrag.active = true;
                            aiPathNodeDrag.pathId = pathId;
                            aiPathNodeDrag.pathNodeIndex = pathNodeIndex;
                            aiPathNodeDrag.startPosition = aiPathNode->pos;
                        } else if (escape) {
                            aiPathNode->pos = aiPathNodeDrag.startPosition;
                            aiPathNodeDrag = {};
                            escape = false;
                        }
                    } else {
                        aiPathNodeDrag = {};
                    }
                }
                DrawRectangleRec(nodeRect, color);
            }
        }

        // NOTE(dlb): I could rewrite the loop above again below to have draw happen
        // after drag update.. but meh. I don't wanna duplicate code, so dragging nodes
        // will have 1 frame of delay. :(
        if (aiPathNodeDrag.active) {
            Vector2 newNodePos = cursorWorldPos;
            Vector2SubtractValue(newNodePos, pathRectRadius);
            AiPath *aiPath = server.world->map.GetPath(aiPathNodeDrag.pathId);
            AiPathNode *aiPathNode = server.world->map.GetPathNode(aiPathNodeDrag.pathId, aiPathNodeDrag.pathNodeIndex);
            aiPathNode->pos = newNodePos;
        }

        const bool editorHovered = dlb_CheckCollisionPointRec(GetMousePosition(), editorRect);

        // [Editor] Tile actions and hover highlight
        if (editorActive && !editorHovered) {
            const bool editorPlaceTile = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            const bool editorPickTile = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
            const bool editorFillTile = IsKeyPressed(KEY_F);

            Tile hoveredTile;
            bool hoveringTile = map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile);
            if (hoveringTile) {
                Tilemap::Coord coord{};
                bool validCoord = map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
                assert(validCoord);  // should always be true when hoveredTile != null

                if (editorPlaceTile) {
                    map.Set(coord.x, coord.y, cursor.tileDefId);
                } else if (editorPickTile) {
                    cursor.tileDefId = hoveredTile;
                } else if (editorFillTile) {
                    Fill(map, coord.x, coord.y, cursor.tileDefId);
                }

                DrawRectangleLinesEx({ (float)coord.x * TILE_W, (float)coord.y * TILE_W, TILE_W, TILE_W }, 2, WHITE);
            }
        }

        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (entity.type) {
                entity.Draw(fntHackBold20, entityId, 1);
#if CL_DBG_COLLIDERS
                // [Debug] Draw colliders
                if (entity.radius) {
                    DrawCircle(entity.position.x, entity.position.y, entity.radius, entity.colliding ? Fade(RED, 0.5) : Fade(GRAY, 0.5));
                }
#endif
#if CL_DBG_FORCE_ACCUM
                // [Debug] Draw force vectors
                if (Vector2LengthSqr(entity.forceAccum)) {
                    DrawLineEx(
                        { entity.position.x, entity.position.y },
                        { entity.position.x + entity.forceAccum.x, entity.position.y + entity.forceAccum.y },
                        2,
                        YELLOW
                    );
                }
#endif
            }
        }

#if CL_DBG_CIRCLE_VS_REC
        {
            Tile tile{};
            if (server.world->map.AtTry(0, 0, tile)) {
                Rectangle tileRect{};
                Vector2 tilePos = { 0, 0 };
                tileRect.x = tilePos.x;
                tileRect.y = tilePos.y;
                tileRect.width = TILE_W;
                tileRect.height = TILE_W;
                Vector2 contact{};
                Vector2 normal{};
                float depth{};
                if (dlb_CheckCollisionCircleRec(cursorWorldPos, 10, tileRect, contact, normal, depth)) {
                    DrawCircle(cursorWorldPos.x, cursorWorldPos.y, 10, Fade(RED, 0.5f));

                    Vector2 resolve = Vector2Scale(normal, depth);
                    DrawLine(
                        contact.x,
                        contact.y,
                        contact.x + resolve.x,
                        contact.y + resolve.y,
                        ORANGE
                    );
                    DrawCircle(contact.x, contact.y, 1, BLUE);
                    DrawCircle(contact.x + resolve.x, contact.y + resolve.y, 1, ORANGE);

                    DrawCircle(cursorWorldPos.x + resolve.x, cursorWorldPos.y + resolve.y, 10, Fade(LIME, 0.5f));
                } else {
                    DrawCircle(cursorWorldPos.x, cursorWorldPos.y, 10, Fade(GRAY, 0.5f));
                }
            }
        }
#endif

        EndMode2D();

#if CL_DBG_TILE_CULLING
        // Screen bounds debug rect for tile culling
        DrawRectangleLinesEx({
            screenMargin,
            screenMargin,
            (float)GetScreenWidth() - screenMargin*2,
            (float)GetScreenHeight() - screenMargin*2,
            }, 1.0f, PINK);
#endif

        // [Editor] Action Bar
        if (editorActive) {
            DrawRectangleRounded(editorRect, 0.2f, 6, { 210, 203, 190, 255 });
            DrawRectangleRoundedLines(editorRect, 0.2f, 6, 2.0f, BLACK);

            Vector2 uiCursor = uiMargin;

            UIState saveButton = UIButton(fntHackBold20, "Save", uiPosition, uiCursor);
            if (saveButton.clicked) {
                if (map.Save(LEVEL_001) != RN_SUCCESS) {
                    // TODO: Display error message on screen for N seconds or
                    // until dismissed
                }
            }

            uiCursor.x += uiPadding.x;

            UIState loadButton = UIButton(fntHackBold20, "Load", uiPosition, uiCursor);
            if (loadButton.clicked) {
                Err err = map.Load(LEVEL_001);
                if (err != RN_SUCCESS) {
                    printf("Failed to load map with code %d\n", err);
                    assert(!"oops");
                    // TODO: Display error message on screen for N seconds or
                    // until dismissed
                }
            }

            uiCursor.x = uiMargin.x;
            uiCursor.y += 30.0f;  // TODO: Calculate MAX(button.height) + pad.y

            // [Editor] Tile selector
            const bool editorPickTileDef = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            DrawRectangle(
                uiPosition.x + uiCursor.x - 2,
                uiPosition.y + uiCursor.y - 2,
                server.world->map.tileDefCount * TILE_W + server.world->map.tileDefCount * 2 + 2,
                TILE_W + 4,
                BLACK
            );

            for (uint32_t i = 0; i < server.world->map.tileDefCount; i++) {
                TileDef &tileDef = server.world->map.tileDefs[i];
                Rectangle texRect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
                Vector2 screenPos = {
                    uiPosition.x + uiCursor.x + i * TILE_W + i * 2,
                    uiPosition.y + uiCursor.y
                };

                Rectangle tileDefRectScreen{ screenPos.x, screenPos.y, TILE_W, TILE_W };
                bool hover = dlb_CheckCollisionPointRec(GetMousePosition(), tileDefRectScreen);
                if (hover) {
                    static int prevTileDefHovered = -1;
                    if (editorPickTileDef) {
                        PlaySound(sndHardTick);
                        cursor.tileDefId = i;
                    } else if (i != prevTileDefHovered) {
                        PlaySound(sndSoftTick);
                    }
                    prevTileDefHovered = i;
                }

                DrawTextureRec(server.world->map.texture, texRect, screenPos, WHITE);
                const int outlinePad = 1;
                if (i == cursor.tileDefId || hover) {
                    DrawRectangleLinesEx(
                        {
                            screenPos.x - outlinePad,
                            screenPos.y - outlinePad,
                            TILE_W + outlinePad * 2,
                            TILE_W + outlinePad * 2
                        },
                        2,
                        i == cursor.tileDefId ? YELLOW : WHITE
                    );
                }
            }
        }

        {
            float hud_x = 8.0f;
            float hud_y = 30.0f;
            char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                if (label) { \
                    snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                } else { \
                    snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
                } \
                Vector2 position{ hud_x, hud_y }; \
                DrawTextShadowEx(fntHackBold20, buf, position, (float)fntHackBold20.baseSize, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntHackBold20, buf, (float)fntHackBold20.baseSize, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += fntHackBold20.baseSize; \
            }

#define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("time", "%.02f", server.yj_server->GetTime());
            DRAW_TEXT("tick", "%" PRIu64, server.tick);
            DRAW_TEXT("tickAccum", "%.02f", server.tickAccum);
            DRAW_TEXT("cursor", "%d, %d (world: %.f, %.f)", GetMouseX(), GetMouseY(), cursorWorldPos.x, cursorWorldPos.y);
            DRAW_TEXT("clients", "%d", server.yj_server->GetNumConnectedClients());

            static bool showClientInfo[yojimbo::MaxClients];
            for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
                if (!server.yj_server->IsClientConnected(clientIdx)) {
                    continue;
                }

                Rectangle clientRowRect{};
                DRAW_TEXT_MEASURE(&clientRowRect,
                    showClientInfo[clientIdx] ? "[-] client" : "[+] client",
                    "%d", clientIdx
                );
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                    && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, clientRowRect))
                {
                    showClientInfo[clientIdx] = !showClientInfo[clientIdx];
                }
                if (showClientInfo[clientIdx]) {
                    hud_x += 16.0f;
                    yojimbo::NetworkInfo netInfo{};
                    server.yj_server->GetNetworkInfo(clientIdx, netInfo);
                    DRAW_TEXT("  rtt", "%f.02", netInfo.RTT);
                    DRAW_TEXT("  % loss", "%.02f", netInfo.packetLoss);
                    DRAW_TEXT("  sent (kbps)", "%.02f", netInfo.sentBandwidth);
                    DRAW_TEXT("  recv (kbps)", "%.02f", netInfo.receivedBandwidth);
                    DRAW_TEXT("  ack  (kbps)", "%.02f", netInfo.ackedBandwidth);
                    DRAW_TEXT("  sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
                    DRAW_TEXT("  recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
                    DRAW_TEXT("  ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
                    hud_x -= 16.0f;
                }
            }
        }

        histogram.Draw(8, 8);
        EndDrawing();
        yojimbo_sleep(0.001);

        // Nobody else handled it, so user probably wants to quit
        if (escape) {
            CloseWindow();
        }
    }

    UnloadFont(fntHackBold20);
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    //SetTraceLogLevel(LOG_WARNING);

    InitAudioDevice();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    //SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Image icon = LoadImage("resources/server.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

#if CL_DBG_ONE_SCREEN
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x, // / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );
#elif CL_DBG_TWO_SCREEN
    const int monitorWidth = GetMonitorWidth(1);
    const int monitorHeight = GetMonitorHeight(1);
    Vector2 monitor2 = GetMonitorPosition(0);
    SetWindowPosition(
        monitor2.x + monitorWidth / 2 - (int)screenSize.x / 2,
        monitor2.y + monitorHeight / 2 - (int)screenSize.y / 2
    );
#endif

    //--------------------
    // Server
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!

    Server *server = new Server;
    if (!server) {
        printf("error: failed to allocate server\n");
        return -1;
    }
    if (ServerStart(server) < 0) {
        printf("error: failed to start server\n");
        return -1;
    }

    //--------------------
    // World
    server->world = new ServerWorld;
    if (!server->world) {
        printf("error: failed to allocate world\n");
        return -1;
    }

    Play(*server);

    //--------------------
    // Cleanup
    ServerStop(*server);
    delete server->yj_server;
    delete server;
    server = {};
    ShutdownYojimbo();
    CloseAudioDevice();
    return 0;
}