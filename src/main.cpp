/*******************************************************************************************
*
*   raylib [textures] example - Texture loading and drawing
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2023 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib/raylib.h"
#include "yojimbo.h"


const uint64_t ProtocolId = 0x11223344556677ULL;

const int ClientPort = 30000;
const int ServerPort = 40000;

inline int GetNumBitsForMessage(uint16_t sequence)
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof(messageBitsArray) / sizeof(int);
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct TestMessage : public yojimbo::Message
{
    uint16_t sequence;

    TestMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_bits(stream, sequence, 16);

        int numBits = GetNumBitsForMessage(sequence);
        int numWords = numBits / 32;
        uint32_t dummy = 0;
        for (int i = 0; i < numWords; ++i)
            serialize_bits(stream, dummy, 32);
        int numRemainderBits = numBits - numWords * 32;
        if (numRemainderBits > 0)
            serialize_bits(stream, dummy, numRemainderBits);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestBlockMessage : public yojimbo::BlockMessage
{
    uint16_t sequence;

    TestBlockMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_bits(stream, sequence, 16);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestSerializeFailOnReadMessage : public yojimbo::Message
{
    template <typename Stream> bool Serialize(Stream & /*stream*/)
    {
        return !Stream::IsReading;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestExhaustStreamAllocatorOnReadMessage : public yojimbo::Message
{
    template <typename Stream> bool Serialize(Stream &stream)
    {
        if (Stream::IsReading)
        {
            const int NumBuffers = 100;

            void *buffers[NumBuffers];

            memset(buffers, 0, sizeof(buffers));

            for (int i = 0; i < NumBuffers; ++i)
            {
                buffers[i] = YOJIMBO_ALLOCATE(stream.GetAllocator(), 1024 * 1024);
            }

            for (int i = 0; i < NumBuffers; ++i)
            {
                YOJIMBO_FREE(stream.GetAllocator(), buffers[i]);
            }
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestMessageType
{
    TEST_MESSAGE,
    TEST_BLOCK_MESSAGE,
    TEST_SERIALIZE_FAIL_ON_READ_MESSAGE,
    TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE,
    NUM_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START(TestMessageFactory, NUM_TEST_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(TEST_MESSAGE, TestMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(TEST_BLOCK_MESSAGE, TestBlockMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(TEST_SERIALIZE_FAIL_ON_READ_MESSAGE, TestSerializeFailOnReadMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE(TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE, TestExhaustStreamAllocatorOnReadMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

enum SingleTestMessageType
{
    SINGLE_TEST_MESSAGE,
    NUM_SINGLE_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START(SingleTestMessageFactory, NUM_SINGLE_TEST_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(SINGLE_TEST_MESSAGE, TestMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

enum SingleBlockTestMessageType
{
    SINGLE_BLOCK_TEST_MESSAGE,
    NUM_SINGLE_BLOCK_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START(SingleBlockTestMessageFactory, NUM_SINGLE_BLOCK_TEST_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(SINGLE_BLOCK_TEST_MESSAGE, TestBlockMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class TestAdapter : public yojimbo::Adapter
{
public:

    yojimbo::MessageFactory *CreateMessageFactory(yojimbo::Allocator &allocator)
    {
        return YOJIMBO_NEW(allocator, TestMessageFactory, allocator);
    }
};

static TestAdapter adapter;
static double serverTime = 100.0;
static double clientTime = 100.0;
const double deltaTime = 0.01f;

yojimbo::Server &ServerStart()
{
    static yojimbo::Server *server = nullptr;
    if (server) {
        return *server;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    yojimbo::ClientServerConfig config;

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", ServerPort), config, adapter, serverTime);

    server->Start(yojimbo::MaxClients);
    if (!server->IsRunning()) {
        printf("Failed to start server\n");
        return *server;
    }

    printf("started server on port %d (insecure)\n", ServerPort);
    char addressString[256];
    server->GetAddress().ToString(addressString, sizeof(addressString));
    printf("server address is %s\n", addressString);

    return *server;
}

void ServerUpdate(yojimbo::Server &server)
{
    if (!server.IsRunning())
        return;

    server.SendPackets();
    server.ReceivePackets();

    const int clientIdx = 0;
    const int channelIdx = 0;
    yojimbo::Message *message = server.ReceiveMessage(clientIdx, channelIdx);
    do {
        if (message) {
            switch (message->GetType())
            {
                case TEST_MESSAGE:
                {
                    TestMessage *testMessage = (TestMessage *)message;
                    printf("recv[seq=%d] TEST_MESSAGE\n", testMessage->sequence);
                    server.ReleaseMessage(clientIdx, message);
                }
                break;
            }
        }
        message = server.ReceiveMessage(clientIdx, channelIdx);
    } while (message);

    serverTime += deltaTime;
    server.AdvanceTime(serverTime);

    yojimbo_sleep(deltaTime);
}

void ServerStop(yojimbo::Server &server)
{
    server.Stop();
}

yojimbo::Client &ClientStart()
{
    static yojimbo::Client *client = nullptr;
    if (client) {
        return *client;
    }

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("client id is %.16" PRIx64 "\n", clientId);

    yojimbo::ClientServerConfig config;

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, adapter, clientTime);

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
    printf("client address is %s\n", addressString);

    return *client;
}

void ClientUpdate(yojimbo::Client &client)
{
    static double lastSentAt = 0;
    if (clientTime > lastSentAt + 1.0) {
        TestMessage *message = (TestMessage *)client.CreateMessage(TEST_MESSAGE);
        if (message)
        {
            static uint16_t numMessagesSentToServer = 0;
            message->sequence = (uint16_t)numMessagesSentToServer;
            client.SendMessage(0, message);
            numMessagesSentToServer++;
            lastSentAt = clientTime;
        }
    }

    client.SendPackets();
    client.ReceivePackets();

    if (client.IsDisconnected()) {
        return;
    }

    clientTime += deltaTime;
    client.AdvanceTime(clientTime);

    if (client.ConnectionFailed())
        return;

    yojimbo_sleep(deltaTime);
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    if (!InitializeYojimbo())
    {
        printf("error: failed to initialize Yojimbo!\n");
        return 1;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    // yojimbo does this.. why?
    //srand((unsigned int)time(NULL));

    yojimbo::Server &server = ServerStart();
    yojimbo::Client &client = ClientStart();

    // Initialization
    //--------------------------------------------------------------------------------------
    const int windowWidth = 1600;
    const int windowHeight = 900;

    InitWindow(windowWidth, windowHeight, "RayNet");
    SetWindowState(FLAG_VSYNC_HINT);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");        // Texture loading
    //---------------------------------------------------------------------------------------

    int fontSize = 32;
    Font font = LoadFontEx("resources/OpenSans-Bold.ttf", fontSize, 0, 0);

    const char *text = "Meow MEOW meow MeOw!";
    Vector2 textSize = MeasureTextEx(font, text, (float)fontSize, 1);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        ServerUpdate(server);
        ClientUpdate(client);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            windowWidth / 2.0f - texture.width / 2.0f,
            windowHeight / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        Vector2 textPos = {
            windowWidth / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        Vector2 textShadowPos = textPos;
        textShadowPos.x += 2;
        textShadowPos.y += 2;

        //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
        DrawTextEx(font, text, textShadowPos, (float)fontSize, 1, BLACK);
        DrawTextEx(font, text, textPos, (float)fontSize, 1, RAYWHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(font);
    UnloadTexture(texture);       // Texture unloading

    CloseWindow();                // Close window and OpenGL context

    ClientStop(client);
    ServerStop(server);

    delete &client;
    delete &server;

    ShutdownYojimbo();
    //--------------------------------------------------------------------------------------

    return 0;
}