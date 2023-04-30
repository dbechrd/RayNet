#pragma once
#include "../common/shared_lib.h"

struct ClientWorld;

struct Controller {
    int nextSeq{};  // next input command sequence number to use

    InputCmd cmdAccum{};         // accumulate input until we're ready to sample
    double sampleInputAccum{};   // when this fills up, we are due to sample again
    double lastInputSampleAt{};  // time we last sampled accumulator
    double lastCommandSentAt{};  // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples
};

struct GameClient {
    NetAdapter adapter;
    yojimbo::Client *yj_client{};
    double clientTimeDeltaVsServer{};  // how far ahead/behind client clock is
    double netTickAccum{};             // when this fills up, a net tick is due
    double lastNetTick{};              // for fixed-step networking updates
    double now{};                      // current time for this frame

    Controller controller{};
    ClientWorld *world{};

    inline double ServerNow(void) {
        const double serverNow = now - clientTimeDeltaVsServer;
        return serverNow;
    }

    uint32_t LocalPlayerEntityId(void) {
        if (yj_client->IsConnected()) {
            // TODO(dlb)[cleanup]: Yikes.. the server should send us our entityId when we connect
            return yj_client->GetClientIndex() + 1;
        }
        return 0;
    }

    Entity *LocalPlayer(void) {
        if (yj_client->IsConnected()) {
            Entity *localPlayer = world->GetEntity(LocalPlayerEntityId());
            if (localPlayer && localPlayer->type == Entity_Player) {
                return localPlayer;
            }
        }
        return 0;
    }
};
