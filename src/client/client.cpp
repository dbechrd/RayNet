#include "../common/common.h"

TestAdapter adapter{};

yojimbo::Client &ClientStart()
{
    static yojimbo::Client *client = nullptr;
    if (client) {
        return *client;
    }

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::ClientServerConfig config{};

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, adapter, 100);

    yojimbo::Address serverAddress("127.0.0.1", ServerPort);

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

    client->InsecureConnect(privateKey, clientId, serverAddress);

    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);

    return *client;
}

void ClientUpdate(yojimbo::Client &client)
{
    static double lastSentAt = 0;
    double now = client.GetTime();

    if (now > lastSentAt + 0.2) {
        TestMessage *message = (TestMessage *)client.CreateMessage(TEST_MESSAGE);
        if (message)
        {
            static uint16_t numMessagesSentToServer = 0;
            message->sequence = (uint16_t)numMessagesSentToServer;
            message->hitpoints = 100 - numMessagesSentToServer;
            client.SendMessage(0, message);
            numMessagesSentToServer++;
            lastSentAt = now;
        }
    }

    client.SendPackets();
    client.ReceivePackets();

    if (client.IsDisconnected()) {
        return;
    }

    client.AdvanceTime(now + deltaTime);

    if (client.ConnectionFailed())
        return;

    //yojimbo_sleep(deltaTime);
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
    // yojimbo does this.. why?
    //srand((unsigned int)time(NULL));

    yojimbo::Client &client = ClientStart();

    while (!WindowShouldClose())
    {
        //--------------------
        // Update
        ClientUpdate(client);

        //--------------------
        // Draw
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            WINDOW_WIDTH / 2.0f - texture.width / 2.0f,
            WINDOW_HEIGHT / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

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
    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    ClientStop(client);
    delete &client;
    ShutdownYojimbo();

    return 0;
}