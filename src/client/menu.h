#pragma once
#include "game_client.h"

struct Menu {
    enum ID {
        MENU_NONE,
        MENU_MAIN,
        MENU_CONNECTING,
        MENU_COUNT,
    };

    ID id{};

    Menu(Menu::ID id) : id(id) {}

    virtual void OnEnter(void) {}
    virtual void OnLeave(void) {}
    virtual void Draw(GameClient &client, bool &back) {}
};

struct MenuMain : public Menu {
    MenuMain(void);
    ~MenuMain(void);

    void Draw(GameClient &client, bool &back) override;

private:
    Texture background{};
};

struct MenuConnecting : public Menu {
    MenuConnecting(void) : Menu(Menu::MENU_CONNECTING) {}

    void OnEnter(void) override;
    void OnLeave(void) override;
    void Draw(GameClient &client, bool &back) override;

private:
    const char *connecting_msgs[4] = {
        "Connecting",
        ". Connecting .",
        ".. Connecting ..",
        "... Connecting ..."
    };

    int    msg_index        {};
    double msg_last_updated {};
    Entity campfire   {};
};

struct MenuSystem {
    Menu::ID active_menu_id{};

    MenuSystem(void);
    void TransitionTo(Menu::ID to_id);
    void Draw(GameClient &client, bool &back);

private:
    Menu *menus_by_id[Menu::MENU_COUNT]{};

    MenuMain       menu_main{};
    MenuConnecting menu_connecting{};
};

extern MenuMain       menu_main;
extern MenuConnecting menu_connecting;