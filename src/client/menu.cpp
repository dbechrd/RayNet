#include "menu.h"

MenuSystem::MenuSystem(void)
{
    menus_by_id[Menu::MENU_MAIN] = &menu_main;
    menus_by_id[Menu::MENU_CONNECTING] = &menu_connecting;
}

void MenuSystem::TransitionTo(Menu::ID to_id)
{
    if (to_id == active_menu_id) {
        return;
    }

    Menu *menu = menus_by_id[active_menu_id];
    if (menu) {
        menu->OnLeave();
    }

    active_menu_id = to_id;

    menu = menus_by_id[active_menu_id];
    if (menu) {
        menu->OnEnter();
    }
}

void MenuSystem::Draw(GameClient &client, bool &back) {
    Menu *menu = menus_by_id[active_menu_id];
    if (menu) {
        menu->Draw(client, back);
    }
}

MenuMain::MenuMain(void) : Menu(Menu::MENU_MAIN)
{
    background = LoadTexture("resources/graphics/title.png");
}

MenuMain::~MenuMain(void)
{
    UnloadTexture(background);
}

void MenuMain::Draw(GameClient &client, bool &back)
{
    DrawTexture(background,
        floorf(GetRenderWidth() / 2.0f - background.width / 2.0f),
        floorf(GetRenderHeight() / 2.0f - background.height / 2.0f),
        WHITE
    );

    Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() / 2.0f) };
    uiPosition.y -= 115;
    UIStyle uiStyleMenu {};
    uiStyleMenu.margin = {};
    uiStyleMenu.pad = { 16, 4 };
    uiStyleMenu.bgColor[UI_CtrlTypeButton] = BLANK;
    uiStyleMenu.fgColor = RAYWHITE;
    uiStyleMenu.font = &fntBig;
    uiStyleMenu.alignH = TextAlign_Center;
    UI uiMenu{ uiPosition, uiStyleMenu };

#if 0
    // Draw weird squares animation
    const Vector2 screenHalfSize{ GetRenderWidth()/2.0f, GetRenderHeight()/2.0f };
    const Vector2 screenCenter{ screenHalfSize.x, screenHalfSize.y };
    for (float scale = 0.0f; scale < 1.1f; scale += 0.1f) {
        const float modScale = fmodf(texMenuBgScale + scale, 1);
        Rectangle menuBgRect{
            screenCenter.x - screenHalfSize.x * modScale,
            screenCenter.y - screenHalfSize.y * modScale,
            GetRenderWidth() * modScale,
            GetRenderHeight() * modScale
        };
        DrawRectangleLinesEx(menuBgRect, 20, BLACK);
    }
#endif
    UIState connectButton = uiMenu.Button(CSTRS(SLOCAL_UI_MENU_PLAY.str()));
    if (connectButton.released) {
        //rnSoundCatalog.Play(RN_Sound_Lily_Introduction);
        client.TryConnect();
    }
    uiMenu.Space({ 0, 62 });
    uiMenu.Newline();
    uiMenu.Button(CSTRS(SLOCAL_UI_MENU_OPTIONS.str()));
    uiMenu.Space({ 0, 62 });
    uiMenu.Newline();
    UIState quitButton = uiMenu.Button(CSTRS(SLOCAL_UI_MENU_QUIT.str()));
    if (quitButton.released) {
        back = true;
    }

    // Draw font atlas for SDF font
    //DrawTexture(fntBig.texture, GetRenderWidth() - fntBig.texture.width, 0, WHITE);
}

void MenuConnecting::OnEnter(void) {
    msg_index        = 0;
    msg_last_updated = 0;
    if (!campfire.sprite_id) {
        campfire.sprite_id = packs[0].FindByName<Sprite>("sprite_obj_campfire").id;
    }
}

void MenuConnecting::OnLeave(void) {
    ResetSprite(campfire);
}

void MenuConnecting::Draw(GameClient &client, bool &back)
{
    UpdateSprite(campfire, client.frameDt, !msg_last_updated);
    if (!msg_last_updated) {
        msg_last_updated = client.now;
    } else if (client.now > msg_last_updated + 0.5) {
        msg_last_updated = client.now;
        msg_index = ((size_t)msg_index + 1) % ARRAY_SIZE(connecting_msgs);
    }

    const GfxFrame &campfireFrame = campfire.GetSpriteFrame();

    UIStyle uiStyleMenu {};
    uiStyleMenu.margin = {};
    uiStyleMenu.pad = { 16, 4 };
    uiStyleMenu.bgColor[UI_CtrlTypeButton] = BLANK;
    uiStyleMenu.fgColor = RAYWHITE;
    uiStyleMenu.font = &fntBig;
    uiStyleMenu.alignH = TextAlign_Center;

    Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() - uiStyleMenu.font->baseSize * 4) };
    UI uiMenu{ uiPosition, uiStyleMenu };

    const Vector2 cursorScreen = uiMenu.CursorScreen();
    const Vector2 campfirePos = cursorScreen; //Vector2Subtract(cursorScreen, { (float)(campfireFrame.w / 2), 0 });
    campfire.position.x = campfirePos.x;
    campfire.position.y = campfirePos.y;
    campfire.position.z = 0;

    DrawSprite(campfire, 0);
    uiMenu.Space({ 0, (float)uiStyleMenu.font->baseSize / 2 });

    if (client.yj_client->IsConnecting()) {
        uiMenu.Text(CSTRLEN(connecting_msgs[msg_index]));
    } else {
        uiMenu.Text(CSTR("   Loading..."));
    }
    uiMenu.Newline();
}