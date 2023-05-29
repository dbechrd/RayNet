#pragma once
#include "../common/common.h"
#include "../common/input_command.h"
#include "../common/net/net.h"
#include "client_world.h"
#include "todo.h"

struct GameClient {
    NetAdapter adapter;
    yojimbo::Client *yj_client{};
    double clientTimeDeltaVsServer{};  // how far ahead/behind client clock is
    double netTickAccum{};             // when this fills up, a net tick is due
    double lastNetTick{};              // for fixed-step networking updates
    double now{};                      // current time for this frame

    Histogram fpsHistogram{};
    double frameStart{};
    double frameDt = 0;
    double frameDtSmooth = 60;
    double animAccum = 0;

    bool showF3Menu = false;
    bool showNetInfo = false;
    bool showTodoList = false;

    Controller controller{};
    ClientWorld *world{};
    TodoList todoList{};

    GameClient(double now) : now(now), frameStart(now) {};

    inline double ServerNow(void) {
        const double serverNow = now - clientTimeDeltaVsServer;
        return serverNow;
    }

    Err TryConnect(void);
    void Start(void);
    void SendInput(const Controller &controller);
    void SendEntityInteract(uint32_t entityId);
    void SendTileInteract(uint32_t x, uint32_t y);
    void ProcessMessages(void);
    void Update(void);
    void Stop(void);
};
