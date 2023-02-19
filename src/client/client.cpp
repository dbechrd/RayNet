#include "../common/common_lib.h"
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
    static NetAdapter adapter{};

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, adapter, 100);

    ClientTryConnect(*client);
#if 0
    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
    return *client;
}

void ClientUpdate(yojimbo::Client &client, double frameDt, PlayerInput &playerInput)
{
    const double clientNow = client.GetTime();
    client.AdvanceTime(clientNow + frameDt);
    client.ReceivePackets();

    if (!client.IsConnected()) {
        return;
    }

    {
        const int channelIdx = 0;
        yojimbo::Message *message = client.ReceiveMessage(channelIdx);
        do {
            if (message) {
                switch (message->GetType())
                {
                    case MSG_S_PLAYER_STATE:
                    {
                        Msg_S_PlayerState *msgPlayerState = (Msg_S_PlayerState *)message;
                        g_world->players[msgPlayerState->clientIdx] = msgPlayerState->player;
                        client.ReleaseMessage(message);
                    }
                    break;
                }
            }
            message = client.ReceiveMessage(channelIdx);
        } while (message);
    }

    {
        Msg_C_PlayerInput *message = (Msg_C_PlayerInput *)client.CreateMessage(MSG_C_PLAYER_INPUT);
        if (message)
        {
            message->playerInput = playerInput;
            client.SendMessage(0, message);
            playerInput = {};
        }
    }

    client.SendPackets();
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
}

int main(int argc, char *argv[])
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
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

    double frameStart = GetTime();
    double frameDt = 0;

    // TODO: Input history queue
    PlayerInput playerInputAccum{};

    while (!WindowShouldClose())
    {
        {
            double now = GetTime();
            frameDt = now - frameStart;
            frameStart = now;
        }

        //--------------------
        // Update
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

        playerInputAccum.north |= IsKeyDown(KEY_W);
        playerInputAccum.west  |= IsKeyDown(KEY_A);
        playerInputAccum.south |= IsKeyDown(KEY_S);
        playerInputAccum.east  |= IsKeyDown(KEY_D);

        //--------------------
        // Networking
        ClientUpdate(client, frameDt, playerInputAccum);

        //--------------------
        // Draw
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            WINDOW_WIDTH / 2.0f - texture.width / 2.0f,
            WINDOW_HEIGHT / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        for (int i = 0; i < yojimbo::MaxClients; i++) {
            Player &player = g_world->players[i];
            if (player.color.a) {
                player.Draw();
            }
        }

        Vector2 textPos = {
            WINDOW_WIDTH / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        DrawTextShadowEx(font, text, textPos, (float)FONT_SIZE, RAYWHITE);

        {
            float hud_x = 8.0f;
            float hud_y = 8.0f;
            char buf[128];
            #define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                Vector2 position{ hud_x, hud_y }; \
                DrawTextShadowEx(font, buf, position, (float)FONT_SIZE, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(font, buf, (float)FONT_SIZE, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += FONT_SIZE; \
            }

            #define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT("time", "%f", client.GetTime());

            const char *clientStateStr = "unknown";
            yojimbo::ClientState clientState = client.GetClientState();
            switch (clientState) {
                case yojimbo::CLIENT_STATE_ERROR:        clientStateStr = "CLIENT_STATE_ERROR"; break;
                case yojimbo::CLIENT_STATE_DISCONNECTED: clientStateStr = "CLIENT_STATE_DISCONNECTED"; break;
                case yojimbo::CLIENT_STATE_CONNECTING:   clientStateStr = "CLIENT_STATE_CONNECTING"; break;
                case yojimbo::CLIENT_STATE_CONNECTED:    clientStateStr = "CLIENT_STATE_CONNECTED"; break;
            }

            static bool showNetInfo = false;
            Rectangle netInfoRect{};
            DRAW_TEXT_MEASURE(&netInfoRect,
                showNetInfo ? "[-] state" : "[+] state",
                "%s", clientStateStr
            );
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, netInfoRect))
            {
                showNetInfo = !showNetInfo;
            }
            if (showNetInfo) {
                hud_x += 16.0f;
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
                hud_x -= 16.0f;
            }
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