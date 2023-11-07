#pragma once
#include "../common/common.h"
#include "../common/input_command.h"
#include "../common/net/net.h"
#include "client_world.h"
#include "menu.h"
#include "todo.h"

struct GameClient {
    NetAdapter adapter;
    yojimbo::Client *yj_client{};
    double clientTimeDeltaVsServer{};  // how far ahead/behind client clock is
    double netTickAccum{};             // when this fills up, a net tick is due
    double lastNetTick{};              // for fixed-step networking updates
    double now{};                      // current time for this frame
    uint64_t frame{};

    double frameStart{};
    double frameDt{};
    double frameDtSmooth = 60;
    double animAccum{};

    Controller controller{};
    ClientWorld *world{};

    /////////////////////////////////////////////////////////////////////////////
    // TODO: None of this stuff belongs in here.. it's not for networking
    double fadeDirection{};
    double fadeDuration{};
    double fadeValue{};

    MenuSystem menu_system{};

    bool showF3Menu = false;
    bool showNetInfo = false;
    bool showTodoList = false;

    const char *hudSpinnerItems[2]{
        "Fireball",
        "Shovel"
    };

    bool hudSpinnerPrev = false;
    bool hudSpinner = false;
    Vector2 hudSpinnerPos{};
    int hudSpinnerIndex = 0;  // which index is currently active
    int hudSpinnerCount = 6;  // how many items in hud spinner

    inline const char *HudSpinnerItemName(void) {
        const char *holdingItem = hudSpinnerIndex < ARRAY_SIZE(hudSpinnerItems) ? hudSpinnerItems[hudSpinnerIndex] : 0;
        return holdingItem;
    }
    TodoList todoList{};
    /////////////////////////////////////////////////////////////////////////////

    GameClient(double now) : now(now), frameStart(now) {};

    inline double ServerNow(void) {
        const double serverNow = now - clientTimeDeltaVsServer;
        return serverNow;
    }

    void Start(void);
    Err TryConnect(void);
    void SendInput(const Controller &controller);
    void SendEntityInteract(uint32_t entityId);
    void SendEntityInteractDialogOption(data::Entity &entity, uint32_t optionId);
    void SendTileInteract(const std::string &map_name, uint32_t x, uint32_t y);
    void ProcessMessages(void);
    void Update(void);
    void Stop(void);
};
