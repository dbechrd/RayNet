#include "f3_menu.h"

void F3Menu_Draw(GameServer &server, Camera2D &camera, Vector2 pos)
{
    IO::Scoped scope(IO::IO_F3Menu);

    Vector2 hudCursor = pos;
    Vector2 histoCursor = hudCursor;
    hudCursor.y += (Histogram::histoHeight + 8) * 1;

    char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) \
    { \
        if (label) { \
            snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
        } else { \
            snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
        } \
        dlb_DrawTextShadowEx(fntSmall, CSTRLEN(buf), hudCursor, RAYWHITE); \
        if (measureRect) { \
            Vector2 measure = dlb_MeasureTextEx(fntSmall, CSTRLEN(buf)); \
            *measureRect = { hudCursor.x,hudCursor.y, measure.x, measure.y }; \
        } \
        hudCursor.y += fntSmall.baseSize; \
    }

#define DRAW_TEXT(label, fmt, ...) \
    DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)


    DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)",
        1.0 / server.frameDt,
        server.frameDt * 1000.0,
        IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off"
    );
    DRAW_TEXT("time", "%.02f", server.yj_server->GetTime());
    DRAW_TEXT("tick", "%" PRIu64, server.tick);
    DRAW_TEXT("tickAccum", "%.02f", server.tickAccum);
    DRAW_TEXT("window", "%d, %d", GetScreenWidth(), GetScreenHeight());
    DRAW_TEXT("render", "%d, %d", GetRenderWidth(), GetRenderHeight());
    DRAW_TEXT("cursorScn", "%d, %d", GetMouseX(), GetMouseY());
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
    DRAW_TEXT("cursorWld", "%.f, %.f", cursorWorldPos.x, cursorWorldPos.y);
    DRAW_TEXT("cursorTil", "%.f, %.f", floorf(cursorWorldPos.x / TILE_W), floorf(cursorWorldPos.y / TILE_W));
    DRAW_TEXT("clients", "%d", server.yj_server->GetNumConnectedClients());

    static bool showClientInfo[yojimbo::MaxClients];
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        Rectangle clientRowRect{};
        DRAW_TEXT_MEASURE(&clientRowRect,
            showClientInfo[clientIdx] ? "[-] client" : "[+] client",
            "%d", clientIdx
        );

        bool hudHover = CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, clientRowRect);
        if (hudHover) {
            io.CaptureMouse();
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                showClientInfo[clientIdx] = !showClientInfo[clientIdx];
            }
        }
        if (showClientInfo[clientIdx]) {
            hudCursor.x += 16.0f;
            yojimbo::NetworkInfo netInfo{};
            server.yj_server->GetNetworkInfo(clientIdx, netInfo);
            DRAW_TEXT("  rtt", "%.02f", netInfo.RTT);
            DRAW_TEXT("  % loss", "%.02f", netInfo.packetLoss);
            DRAW_TEXT("  sent (kbps)", "%.02f", netInfo.sentBandwidth);
            DRAW_TEXT("  recv (kbps)", "%.02f", netInfo.receivedBandwidth);
            DRAW_TEXT("  ack  (kbps)", "%.02f", netInfo.ackedBandwidth);
            DRAW_TEXT("  sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
            DRAW_TEXT("  recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
            DRAW_TEXT("  ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
            hudCursor.x -= 16.0f;
        }
    }

    histoFps.Draw(histoCursor);
    histoCursor.y += Histogram::histoHeight + 8;
    //histoInput.Draw(histoCursor);
    //histoCursor.y += Histogram::histoHeight + 8;
    //histoDx.Draw(histoCursor);
    //histoCursor.y += Histogram::histoHeight + 8;

    histoFps.DrawHover();
    //histoInput.DrawHover();
    //histoDx.DrawHover();
}