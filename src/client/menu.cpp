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

void MenuMain::Draw(GameClient &client, bool &back)
{
    const GfxFile &gfx_background = pack_assets.FindByName<GfxFile>(background);
    dlb_DrawTexturePro(gfx_background.texture,
        { 0, 0, (float)gfx_background.texture.width, (float)gfx_background.texture.height },
        { 0, 0, g_RenderSize.x, g_RenderSize.y },
        {}, WHITE
    );

    const GfxFile &gfx_scrolls = pack_assets.FindByName<GfxFile>(scrolls);
    dlb_DrawTextureCenteredFull(gfx_scrolls.texture,
        { floorf(g_RenderSize.x / 2.0f), floorf(g_RenderSize.y / 2.0f) },
        WHITE
    );

    Vector2 uiPosition{ floorf(g_RenderSize.x / 2.0f), floorf(g_RenderSize.y / 2.0f) };
    uiPosition.y -= 115;

    Vector2 uiSize{};

    UIStyle uiStyleMenu {};
    uiStyleMenu.margin = {};
    uiStyleMenu.pad = { 16, 4 };
    uiStyleMenu.bgColor[UI_CtrlTypeButton] = BLANK;
    uiStyleMenu.fgColor = RAYWHITE;
    uiStyleMenu.font = &fntBig;
    uiStyleMenu.alignH = TextAlign_Center;

    UI uiMenu{ uiPosition, uiSize, uiStyleMenu };

#if 0
    // Draw weird squares animation
    const Vector2 screenHalfSize{ g_RenderSize.x/2.0f, g_RenderSize.y/2.0f };
    const Vector2 screenCenter{ screenHalfSize.x, screenHalfSize.y };
    for (float scale = 0.0f; scale < 1.1f; scale += 0.1f) {
        const float modScale = fmodf(texMenuBgScale + scale, 1);
        Rectangle menuBgRect{
            screenCenter.x - screenHalfSize.x * modScale,
            screenCenter.y - screenHalfSize.y * modScale,
            g_RenderSize.x * modScale,
            g_RenderSize.y * modScale
        };
        DrawRectangleLinesEx(menuBgRect, 20, BLACK);
    }
#endif
    UIState connectButton = uiMenu.Button(SLOCAL_UI_MENU_PLAY.str());
    if (connectButton.released) {
        //rnSoundCatalog.Play(RN_Sound_Lily_Introduction);
        client.TryConnect();
    }
    uiMenu.Space({ 0, 62 });
    uiMenu.Newline();
    uiMenu.Button(SLOCAL_UI_MENU_OPTIONS.str());
    uiMenu.Space({ 0, 62 });
    uiMenu.Newline();
    UIState quitButton = uiMenu.Button(SLOCAL_UI_MENU_QUIT.str());
    if (quitButton.released) {
        back = true;
    }

    // Draw font atlas for SDF font
    //DrawTexture(fntBig.texture, g_RenderSize.x - fntBig.texture.width, 0, WHITE);
}

void MenuConnecting::OnEnter(void) {
    msg_index        = 0;
    msg_last_updated = 0;
    if (!campfire.sprite_id) {
        campfire.sprite_id = pack_assets.FindByName<Sprite>("sprite_obj_campfire").id;
    }
}

void MenuConnecting::OnLeave(void) {
    ResetSprite(campfire);
}

void MenuConnecting::Draw(GameClient &client, bool &back)
{
    const GfxFile &gfx_background = pack_assets.FindByName<GfxFile>(background);
    dlb_DrawTexturePro(gfx_background.texture,
        { 0, 0, (float)gfx_background.texture.width, (float)gfx_background.texture.height },
        { 0, 0, g_RenderSize.x, g_RenderSize.y },
        {}, WHITE
    );

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

    Vector2 uiPosition{ floorf(g_RenderSize.x / 2.0f), floorf(g_RenderSize.y - uiStyleMenu.font->baseSize * 4) };
    Vector2 uiSize{};
    UI uiMenu{ uiPosition, uiSize, uiStyleMenu };

    const Vector2 cursorScreen = uiMenu.CursorScreen();
    const Vector2 campfirePos = cursorScreen; //Vector2Subtract(cursorScreen, { (float)(campfireFrame.w / 2), 0 });
    campfire.position.x = campfirePos.x;
    campfire.position.y = campfirePos.y;
    campfire.position.z = 0;

    DrawSprite(campfire, 0);
    uiMenu.Space({ 0, (float)uiStyleMenu.font->baseSize / 2 });

    if (client.yj_client->IsConnecting()) {
        uiMenu.Text(connecting_msgs[msg_index]);
    } else {
        uiMenu.Text("   Loading...");
    }
    uiMenu.Newline();
}