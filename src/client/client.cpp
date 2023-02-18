#include "../common/common.h"
#include "../common/net.h"
#include <time.h>

void ClientTryConnect(yojimbo::Client &client)
{
    if (!client.IsDisconnected()) {
        return;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::Address serverAddress("127.0.0.1", SERVER_PORT);

    client.InsecureConnect(privateKey, clientId, serverAddress);
}

yojimbo::Client &ClientStart()
{
    static yojimbo::Client *client = nullptr;
    if (client) {
        return *client;
    }

    yojimbo::ClientServerConfig config{};

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, netAdapter, 100);

    ClientTryConnect(*client);
#if 0
    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
    return *client;
}

void ClientUpdate(yojimbo::Client &client)
{
    if (client.IsDisconnected()) {
        return;
    }

    MsgPlayerState *message = (MsgPlayerState *)client.CreateMessage(MSG_PLAYER_STATE);
    if (message)
    {
        message->player = g_world->player;
        client.SendMessage(0, message);
    }

    client.SendPackets();
    client.ReceivePackets();

    if (client.IsDisconnected()) {
        return;
    }

    double now = client.GetTime();
    client.AdvanceTime(now + FIXED_DT);

    if (client.ConnectionFailed())
        return;
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
}

int main(void)
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2, // - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");        // Texture loading

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    const char *text = "Meow MEOW meow MeOw!";
    Vector2 textSize = MeasureTextEx(font, text, (float)FONT_SIZE, 1);

    //--------------------
    // Client
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return 1;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    yojimbo::Client &client = ClientStart();

    //--------------------
    // World
    g_world = (World *)calloc(1, sizeof(*g_world));
    if (!g_world) {
        printf("error: failed to allocate world\n");
        return 1;
    }

    Player &player = g_world->player;
    player.color = RED;
    player.size = { 32, 64 };
    player.position = { 100, 100 };
    player.speed = 5.0f;

    double frameStart = GetTime();
    double frameDt = 0;

    while (!WindowShouldClose())
    {
        {
            double now = GetTime();
            frameDt = now - frameStart;
            frameStart = now;
        }

        //--------------------
        // Update
        ClientUpdate(client);

        if (IsKeyPressed(KEY_V)) {
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
            }
        }

        if (IsKeyPressed(KEY_C)) {
            ClientTryConnect(client);
        }

        Vector2 vDelta{};
        if (IsKeyDown(KEY_A)) {
            vDelta.x -= 1.0f;
        }
        if (IsKeyDown(KEY_D)) {
            vDelta.x += 1.0f;
        }
        if (IsKeyDown(KEY_W)) {
            vDelta.y -= 1.0f;
        }
        if (IsKeyDown(KEY_S)) {
            vDelta.y += 1.0f;
        }
        if (vDelta.x && vDelta.y) {
            float invLength = 1.0f / sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y);
            vDelta.x *= invLength;
            vDelta.y *= invLength;
        }
        player.velocity.x = vDelta.x * player.speed;
        player.velocity.y = vDelta.y * player.speed;
        player.position.x += player.velocity.x;
        player.position.y += player.velocity.y;

        //--------------------
        // Draw
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            WINDOW_WIDTH / 2.0f - texture.width / 2.0f,
            WINDOW_HEIGHT / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        player.Draw();

        Vector2 textPos = {
            WINDOW_WIDTH / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        DrawTextShadowEx(font, text, textPos, (float)FONT_SIZE, RAYWHITE);

        {
            float hud_y = 8.0f;
            char buf[128];
            #define DRAW_TEXT(label, fmt, ...) \
                snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                DrawTextShadowEx(font, buf, { 8, hud_y }, (float)FONT_SIZE, RAYWHITE); \
                hud_y += FONT_SIZE;

            DRAW_TEXT("time", "%f", client.GetTime());

            const char *clientState = "unknown";
            switch (client.GetClientState()) {
                case yojimbo::CLIENT_STATE_ERROR:        clientState = "CLIENT_STATE_ERROR"; break;
                case yojimbo::CLIENT_STATE_DISCONNECTED: clientState = "CLIENT_STATE_DISCONNECTED"; break;
                case yojimbo::CLIENT_STATE_CONNECTING:   clientState = "CLIENT_STATE_CONNECTING"; break;
                case yojimbo::CLIENT_STATE_CONNECTED:    clientState = "CLIENT_STATE_CONNECTED"; break;
            }
            DRAW_TEXT("state", "%s", clientState);

            yojimbo::NetworkInfo netInfo{};
            client.GetNetworkInfo(netInfo);

            DRAW_TEXT("rtt", "%f", netInfo.RTT);
            DRAW_TEXT("%% loss", "%f", netInfo.packetLoss);
            DRAW_TEXT("sent (kbps)", "%f", netInfo.sentBandwidth);
            DRAW_TEXT("recv (kbps)", "%f", netInfo.receivedBandwidth);
            DRAW_TEXT("ack  (kbps)", "%f", netInfo.ackedBandwidth);
            DRAW_TEXT("sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
            DRAW_TEXT("recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
            DRAW_TEXT("ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
        }

        EndDrawing();
    }

    //--------------------
    // Cleanup
    free(g_world);

    ClientStop(client);
    delete &client;
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}