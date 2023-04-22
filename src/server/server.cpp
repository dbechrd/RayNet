#include "../common/shared_lib.h"
#include "../common/histogram.h"
#include "server_world.h"
#include <stack>
#include <cassert>

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

    ServerWorld *world{};

    void OnClientJoin(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        serverPlayer.needsClockSync = true;
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
        entity.size = { 64, 32 };
        entity.position = { 100, 100 };
        entity.speed = 100;
        entity.drag = 8.0f;
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
    config.channel[CHANNEL_R_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_ENTITY_DESPAWN].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;

    server->yj_server = new yojimbo::Server(
        yojimbo::GetDefaultAllocator(),
        privateKey,
        yojimbo::Address("127.0.0.1", SV_PORT),
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
                }
                server.yj_server->ReleaseMessage(clientIdx, message);
                message = server.yj_server->ReceiveMessage(clientIdx, channelIdx);
            }
        }
    }
}

void tick_player(Server &server, uint32_t entityId, double dt)
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
            uint32_t idBullet = server.world->MakeEntity(Entity_Projectile);
            if (idBullet) {
                Entity &eBullet = server.world->entities[idBullet];
                eBullet.color = ORANGE;
                eBullet.size = { 5, 5 };
                eBullet.position = { ePlayer.position.x, ePlayer.position.y };
                eBullet.velocity = { (float)GetRandomValue(800, 1000), (float)GetRandomValue(0, -200) };
                eBullet.drag = 0.02f;
            }
        }
    }

    ePlayer.Tick(SV_TICK_DT);
}

void tick_bot(Server &server, uint32_t entityId, double dt)
{
    static const Vector2 points[] = {
        { 554, 347 },
        { 598, 408 },
        { 673, 450 },
        { 726, 480 },
        { 767, 535 },
        { 813, 595 },
        { 888, 621 },
        { 952, 598 },
        { 990, 553 },
        { 1011, 490 },
        { 988, 426 },
        { 949, 368 },
        { 905, 316 },
        { 857, 260 },
        { 801, 239 },
        { 757, 267 },
        { 702, 295 },
        { 641, 274 },
        { 591, 258 },
        { 546, 289 }
    };
    static int targetIdx = 0;

    Entity &eBot = server.world->entities[entityId];
    Vector2 target = points[targetIdx];
    if (fabsf(eBot.position.x - points[targetIdx].x) < 10 &&
        fabsf((eBot.position.y - eBot.size.y / 2) - points[targetIdx].y) < 10) {
        targetIdx++;
        targetIdx = (targetIdx + 1) % 20;
    }

    InputCmd aiCmd{};
    if (eBot.position.x < target.x) {
        aiCmd.east = true;
    } else if (eBot.position.x > target.x) {
        aiCmd.west = true;
    }
    if (eBot.position.y - eBot.size.y / 2 < target.y) {
        aiCmd.south = true;
    } else if (eBot.position.y - eBot.size.y / 2 > target.y) {
        aiCmd.north = true;
    }

    Vector2 moveForce = aiCmd.GenerateMoveForce(eBot.speed);
    eBot.ApplyForce(moveForce);

    eBot.Tick(SV_TICK_DT);
}

void tick_projectile(Server &server, uint32_t entityId, double dt)
{
    Entity &eProjectile = server.world->entities[entityId];
    eProjectile.ApplyForce({ 0, 5 });
    eProjectile.Tick(dt);

    if (eProjectile.velocity.x < 100 && eProjectile.velocity.y < 100) {
        server.world->DespawnEntity(entityId);
    } else if (
        eProjectile.position.x < -eProjectile.size.x ||
        eProjectile.position.x > WINDOW_WIDTH ||
        eProjectile.position.y < -eProjectile.size.y ||
        eProjectile.position.y > WINDOW_HEIGHT)
    {
        server.world->DespawnEntity(entityId);
    }
}

typedef void (*EntityTicker)(Server &server, uint32_t entityId, double dt);
static EntityTicker entity_ticker[Entity_Count] = {
    0,
    tick_player,
    tick_bot,
    tick_projectile,
};

void ServerTick(Server &server)
{
    // Tick entites
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = server.world->entities[entityId];
        if (entity.type && !entity.despawnedAt) {
            entity_ticker[entity.type](server, entityId, SV_TICK_DT);
        }
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
        if (!server.yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
            continue;
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (!entity.type) {
                continue;
            }

            ServerPlayer &serverPlayer = server.world->players[clientIdx];
            if (entity.despawnedAt) {
                Msg_S_EntityDespawn *msg = (Msg_S_EntityDespawn *)server.yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_DESPAWN);
                if (msg) {
                    msg->entityId = entityId;
                    server.yj_server->SendMessage(clientIdx, CHANNEL_R_ENTITY_DESPAWN, msg);
                }
            } else {
                Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)server.yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
                if (msg) {
                    entity.Serialize(entityId, msg->entitySnapshot, server.lastTickedAt, serverPlayer.lastInputSeq);
                    server.yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
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

void ServerUpdate(Server &server, double now)
{
    if (!server.yj_server->IsRunning())
        return;

    server.yj_server->AdvanceTime(now);
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

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec)
{
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

struct Cursor {
    int tileDefId;
} cursor{};

struct TileDef {
    int x, y;
} tileDefs[] = {
    // n * (w + pad*2) + pad
    { 0 * (TILE_W + 2) + 1, 0 * (TILE_W + 2) + 1 },  // empty tile
    { 1 * (TILE_W + 2) + 1, 0 * (TILE_W + 2) + 1 },
    { 2 * (TILE_W + 2) + 1, 0 * (TILE_W + 2) + 1 },
    { 3 * (TILE_W + 2) + 1, 0 * (TILE_W + 2) + 1 },
    { 4 * (TILE_W + 2) + 1, 0 * (TILE_W + 2) + 1 },
};

struct Tile {
    int tileDefId;
};

struct Map {
    struct Coord {
        int x, y;
    };

    int width;
    int height;
    Tile *tiles;

    Map(int width, int height) : width(width), height(height)
    {
        tiles = (Tile *)calloc((size_t)width * height, sizeof(*tiles));
    }

    ~Map() {
        free(tiles);
    }

    Tile &At(int x, int y) {
        assert(x < width);
        assert(y < height);
        return tiles[y * width + x];
    }

    Tile *AtTry(int x, int y) {
        if (x >= 0 && y >= 0 && x < width && y < height) {
            return &At(x, y);
        }
        return 0;
    }

    bool WorldToTileIndex(int world_x, int world_y, Coord &coord) {
        int x = world_x / TILE_W;
        int y = world_y / TILE_W;
        if (x >= 0 && y >= 0 && x < width && y < height) {
            coord.x = x;
            coord.y = y;
            return true;
        }
        return false;
    }

    Tile *AtWorld(int world_x, int world_y) {
        Coord coord{};
        if (WorldToTileIndex(world_x, world_y, coord)) {
            return &At(coord.x, coord.y);
        }
        return 0;
    }
};

bool NeedsFill(Map &map, int x, int y, int tileDefFill)
{
    Tile *tile = map.AtTry(x, y);
    return tile && map.At(x, y).tileDefId == tileDefFill;
}

void Scan(Map &map, int lx, int rx, int y, int tileDefFill, std::stack<Map::Coord> &stack)
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

void Fill(Map &map, int x, int y, int tileDefId)
{
    int tileDefFill = map.At(x, y).tileDefId;
    if (tileDefFill == tileDefId) {
        return;
    }

    std::stack<Map::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Map::Coord coord = stack.top();
        stack.pop();

        int lx = coord.x;
        int rx = coord.x;
        while (NeedsFill(map, lx - 1, coord.y, tileDefFill)) {
            map.At(lx - 1, coord.y).tileDefId = tileDefId;
            lx -= 1;
        }
        while (NeedsFill(map, rx, coord.y, tileDefFill)) {
            map.At(rx, coord.y).tileDefId = tileDefId;
            rx += 1;
        }
        Scan(map, lx, rx, coord.y - 1, tileDefFill, stack);
        Scan(map, lx, rx, coord.y + 1, tileDefFill, stack);
    }
}

static Sound sndSoftTick;
static Sound sndHardTick;

bool UIButton(Font font, const char *text, Vector2 &uiCursor)
{
    const Vector2 margin{ 8, 8 };
    const Vector2 pad{ 8, 1 };
    const float cornerRoundness = 0.2f;
    const float cornerSegments = 4;
    const Vector2 lineThick{ 1.0f, 1.0f };

    Vector2 textSize = MeasureTextEx(font, text, font.baseSize, 1.0f);
    Vector2 buttonSize = textSize;
    buttonSize.x += pad.x * 2;
    buttonSize.y += pad.y * 2;

    Rectangle buttonRect = {
        uiCursor.x - lineThick.x,
        uiCursor.y - lineThick.y,
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
    bool hover = false;
    bool down = false;
    bool clicked = false;
    if (dlb_CheckCollisionPointRec(GetMousePosition(), buttonRect)) {
        hover = true;
        color = SKYBLUE;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            down = true;
            color = DARKBLUE;
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            clicked = true;
        }
    }

    if (clicked) {
        PlaySound(sndHardTick);
    } else if (hover && text != prevHover) {
        PlaySound(sndSoftTick);
        prevHover = text;
    }

    float yOffset = (down ? 0 : -lineThick.y * 2);
    DrawRectangleRounded({ uiCursor.x, uiCursor.y + yOffset, buttonSize.x, buttonSize.y }, cornerRoundness, cornerSegments, color);
    DrawTextShadowEx(font, text, { uiCursor.x + pad.x, uiCursor.y + pad.y + yOffset }, font.baseSize, WHITE);

    uiCursor.x += buttonSize.x + margin.x;
    return clicked;
}

void Play(Server &server)
{
    sndSoftTick = LoadSound("resources/soft_tick.wav");
    sndHardTick = LoadSound("resources/hard_tick.wav");

    Texture2D catTexture = LoadTexture("resources/cat.png");
    Vector2 catPos = {
        WINDOW_WIDTH / 2.0f - catTexture.width / 2.0f,
        WINDOW_HEIGHT / 2.0f - catTexture.height / 2.0f
    };

    Texture2D tilesTexture = LoadTexture("resources/tiles32.png");

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
    const char *text = "Listening...";
    Vector2 statusMsgSize = MeasureTextEx(font, text, (float)font.baseSize, 1);
    Vector2 statusMsgPos = {
        WINDOW_WIDTH / 2 - statusMsgSize.x / 2,
        catPos.y + catTexture.height + 4
    };

    Camera3D camera3d{};
    camera3d.position = {0, 0, 0};
    camera3d.target = {0, 0, -1};
    camera3d.up = {0, 1, 0};
    camera3d.fovy = 90;
    camera3d.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera3d, CAMERA_FREE);

    Camera2D camera2d{};
    camera2d.zoom = 1.0f;

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    while (!WindowShouldClose())
    {
        const double now = GetTime();
        frameDt = MIN(now - frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = now;

        server.tickAccum += frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;
        while (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            ServerUpdate(server, now);
        }

        if (IsKeyPressed(KEY_H)) {
            histogram.paused = !histogram.paused;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ZERO)) {
            camera2d.zoom = 1.0f;
        }
        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

#if 0
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            camera2d.target.x -= delta.x;
            camera2d.target.y -= delta.y;
        }
        camera2d.zoom += GetMouseWheelMove() * 0.2f * camera2d.zoom;
#else
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera2d.zoom);

            camera2d.target = Vector2Add(camera2d.target, delta);
        }

        // Zoom based on mouse wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
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
#endif

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(BROWN);

        DrawTexture(catTexture, (int)catPos.x, (int)catPos.y, WHITE);

        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (entity.type) {
                entity.Draw(font, entityId, 1);
            }
        }

        DrawTextShadowEx(font, text, statusMsgPos, (float)font.baseSize, RAYWHITE);

        bool editorPickTileDef = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool editorPlaceTile = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        bool editorPickTile = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        bool editorFillTile = IsKeyPressed(KEY_F);
        bool tileDefHovered = false;

        // Map
        static Map map(128, 128);

        Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera2d);
        Tile *hoveredTile = map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y);

#if CL_DBG_TILE_CULLING
        const int screenMargin = 64;
        Vector2 screenTLWorld = GetScreenToWorld2D({ screenMargin, screenMargin }, camera2d);
        Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth() - screenMargin, (float)GetScreenHeight() - screenMargin }, camera2d);
#else
        Vector2 screenTLWorld = GetScreenToWorld2D({ 0, 0 }, camera2d);
        Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera2d);
#endif

        int yMin = CLAMP(floorf(screenTLWorld.y / TILE_W), 0, map.height);
        int yMax = CLAMP(ceilf(screenBRWorld.y / TILE_W), 0, map.height);
        int xMin = CLAMP(floorf(screenTLWorld.x / TILE_W), 0, map.width);
        int xMax = CLAMP(ceilf(screenBRWorld.x / TILE_W), 0, map.width);

        BeginMode2D(camera2d);
        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                Tile &tile = map.At(x, y);
                TileDef &tileDef = tileDefs[tile.tileDefId];

                Rectangle texRect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
                Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                DrawTextureRec(tilesTexture, texRect, tilePos, WHITE);
            }
        }

        if (hoveredTile) {
            Map::Coord coord{};
            bool validCoord = map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
            assert(validCoord);  // should always be true when hoveredTile != null

            if (editorPlaceTile) {
                hoveredTile->tileDefId = cursor.tileDefId;
            } else if (editorPickTile) {
                cursor.tileDefId = hoveredTile->tileDefId;
            } else if (editorFillTile) {
                Fill(map, coord.x, coord.y, cursor.tileDefId);
            }

            DrawRectangleLines(coord.x * TILE_W, coord.y * TILE_W, TILE_W, TILE_W, WHITE);
        }
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

        // Action Bar
        {
            float x = 300;
            float y = 8;
            float pad = 4;

            Vector2 uiCursor{ 360, 8 };
            if (UIButton(font, "Save", uiCursor)) {
                // TODO: Save something
            }
            if (UIButton(font, "Load", uiCursor)) {
                // TODO: Save something
            }
        }

        // Tile selector
        for (int i = 0; i < ARRAY_SIZE(tileDefs); i++) {
            TileDef &tileDef = tileDefs[i];
            Vector2 screenPos = { 360.0f + i * TILE_W + i * 2, 38.0f };
            Rectangle tileDefRectScreen{ screenPos.x, screenPos.y, TILE_W, TILE_W };
            bool hover = dlb_CheckCollisionPointRec(GetMousePosition(), tileDefRectScreen);
            static int prevHover = -1;
            if (hover) {
                if (editorPickTileDef) {
                    PlaySound(sndHardTick);
                    cursor.tileDefId = i;
                } else if (i != prevHover) {
                    PlaySound(sndSoftTick);
                    prevHover = i;
                }
                tileDefHovered = true;
            }
            Rectangle texRect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
            DrawTextureRec(tilesTexture, texRect, screenPos, WHITE);
            if (i == cursor.tileDefId) {
                DrawRectangleLines(screenPos.x, screenPos.y, TILE_W, TILE_W, YELLOW);
            } else if (hover) {
                DrawRectangleLines(screenPos.x, screenPos.y, TILE_W, TILE_W, WHITE);
            }
        }

        // Cursor tile
        //{
        //    TileDef &tileDef = tileDefs[cursor.tileDefId];
        //    Rectangle texRect{ tileDef.x, tileDef.y, TILE_W, TILE_W };
        //    Vector2 screenPos = GetMousePosition();
        //    //screenPos.x -= tileDef.w / 2;
        //    //screenPos.y -= tileDef.h / 2;
        //    DrawTextureRec(tilesTexture, texRect, screenPos, WHITE);
        //}

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
                DrawTextShadowEx(font, buf, position, (float)font.baseSize, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(font, buf, (float)font.baseSize, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += font.baseSize; \
            }

#define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("time", "%.02f", server.yj_server->GetTime());
            DRAW_TEXT("tick", "%" PRIu64, server.tick);
            DRAW_TEXT("tickAccum", "%.02f", server.tickAccum);
            DRAW_TEXT("cursor", "%d, %d", GetMouseX(), GetMouseY());
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
    }

    UnloadTexture(catTexture);
    UnloadTexture(tilesTexture);
    UnloadFont(font);
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

    //-----------------
    // Bots
    uint32_t eid_bot1 = server->world->MakeEntity(Entity_Bot);
    if (eid_bot1) {
        Entity &bot1 = server->world->entities[eid_bot1];
        bot1.type = Entity_Bot;
        bot1.color = DARKPURPLE;
        bot1.size = { 64, 32 };
        bot1.position = { 200, 200 };
        bot1.speed = 100;
        bot1.drag = 8.0f;
    }
    //-----------------

    Play(*server);

    //--------------------
    // Cleanup
    ServerStop(*server);
    delete server->yj_server;
    delete server;
    server = {};
    ShutdownYojimbo();
    CloseWindow();
    CloseAudioDevice();
    return 0;
}