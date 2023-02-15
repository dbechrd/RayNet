#pragma once
#include "yojimbo.h"

const uint64_t ProtocolId = 0x11223344556677ULL;

const int ClientPort = 30000;
const int ServerPort = 40000;
const double deltaTime = 0.01f;

int yj_printf(const char *format, ...);

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
    uint32_t hitpoints;

    TestMessage()
    {
        sequence = 0;
        hitpoints = 0;
    }

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_bits(stream, sequence, 16);
        serialize_uint32(stream, hitpoints);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestMessageType
{
    TEST_MESSAGE,
    NUM_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START(TestMessageFactory, NUM_TEST_MESSAGE_TYPES);
YOJIMBO_DECLARE_MESSAGE_TYPE(TEST_MESSAGE, TestMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class TestAdapter : public yojimbo::Adapter
{
public:

    yojimbo::MessageFactory *CreateMessageFactory(yojimbo::Allocator &allocator)
    {
        return YOJIMBO_NEW(allocator, TestMessageFactory, allocator);
    }
};