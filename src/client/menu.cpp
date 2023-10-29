#include "menu.h"

void Menu::TransitionTo(MenuID to_id)
{
    if (to_id == id) {
        return;
    }

    switch (to_id) {
        case MenuID::MENU_MAIN:       main       = {}; break;
        case MenuID::MENU_CONNECTING: connecting = {}; break;
    }
    id = to_id;
}

void Menu::Draw(GameClient &client, bool &back)
{
    switch (id) {
        case MenuID::MENU_MAIN:       DrawMenuMain(client, back);       break;
        case MenuID::MENU_CONNECTING: DrawMenuConnecting(client, back); break;
    }
}

void Menu::DrawMenuMain(GameClient &client, bool &quit)
{
    Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() / 2.0f) };
    uiPosition.y -= 50;
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
    UIState connectButton = uiMenu.Button("Play");
    if (connectButton.released) {
        //rnSoundCatalog.Play(RN_Sound_Lily_Introduction);
        client.TryConnect();
    }
    uiMenu.Newline();
    uiMenu.Button("Options");
    uiMenu.Newline();
    UIState quitButton = uiMenu.Button("Quit");
    if (quitButton.released) {
        quit = true;
    }

    // Draw font atlas for SDF font
    //DrawTexture(fntBig.texture, GetRenderWidth() - fntBig.texture.width, 0, WHITE);
}

void Menu::DrawMenuConnecting(GameClient &client, bool &back)
{
    MenuConnecting &s = connecting;

    if (s.campfire.sprite.empty()) {
        s.campfire.sprite = "sprite_obj_campfire";
    }

    data::UpdateSprite(s.campfire, client.frameDt, !s.msg_last_updated);
    if (!s.msg_last_updated) {
        s.msg_last_updated = client.now;
    } else if (client.now > s.msg_last_updated + 0.5) {
        s.msg_last_updated = client.now;
        s.msg_index = ((size_t)s.msg_index + 1) % ARRAY_SIZE(s.connecting_msgs);
    }

    const data::GfxFrame &campfireFrame = data::GetSpriteFrame(s.campfire);

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
    s.campfire.position.x = campfirePos.x;
    s.campfire.position.y = campfirePos.y;
    s.campfire.position.z = 0;

    data::DrawSprite(s.campfire);
    uiMenu.Space({ 0, (float)uiStyleMenu.font->baseSize / 2 });

    if (client.yj_client->IsConnecting()) {
        uiMenu.Text(s.connecting_msgs[s.msg_index]);
    } else {
        uiMenu.Text("   Loading...");
    }
    uiMenu.Newline();
}