#pragma once
#include "game_client.h"

struct MenuMain {
    int unused;
};

struct MenuConnecting {
    const char *connecting_msgs[4] = {
        "Connecting",
        ". Connecting .",
        ".. Connecting ..",
        "... Connecting ..."
    };

    int    msg_index        {};
    double msg_last_updated {};
    data::Entity campfire   {};
};

struct Menu {
    enum MenuID {
        MENU_NONE,
        MENU_MAIN,
        MENU_CONNECTING,
    };

    MenuID id{};

    MenuMain main{};
    MenuConnecting connecting{};

    void TransitionTo(MenuID to_id);
    void Draw(GameClient &client, bool &back);

private:
    void DrawMenuMain(GameClient &client, bool &back);
    void DrawMenuConnecting(GameClient &client, bool &back);
};