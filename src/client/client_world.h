#pragma once
#include "../common/common.h"
#include "../common/data.h"
#include "../common/input_command.h"

struct Msg_S_EntitySpawn;
struct Msg_S_EntitySnapshot;
struct GameClient;

struct Controller {
    int nextSeq{};  // next input command sequence number to use

    InputCmd cmdAccum{};         // accumulate input until we're ready to sample
    double sampleInputAccum{};   // when this fills up, we are due to sample again
    double lastInputSampleAt{};  // time we last sampled accumulator
    double lastCommandSentAt{};  // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples

    // not synced, just tracked client-side
    bool tile_hovered{};
    uint32_t tile_x{};
    uint32_t tile_y{};
};

struct Spinner {
    const char *items[2]{
        "Fireball",
        "Shovel"
    };

    bool activePrev = false;
    bool active = false;
    Vector2 position{};
    int index = 0;  // which index is currently active
    int count = 6;  // how many items in hud spinner

    inline const char *ItemName(void) {
        const char *holdingItem = index < ARRAY_SIZE(items) ? items[index] : 0;
        return holdingItem;
    }

    // Return true if the held item interacts with tiles on primary click (used to show hover rect)
    inline bool ItemIsTool(void) {
        const char *holdingItem = ItemName();
        return holdingItem == "Shovel";
    }

    void Update(void)
    {
        IO::Scoped scope(IO::IO_HUDSpinner);

        activePrev = active;
        active = io.KeyDown(KEY_TAB);
        if (active) {
            io.CaptureMouse();
        }
    }

    void Draw(void)
    {
        if (!active) {
            return;
        }


        IO::Scope scope(IO::IO_HUDSpinner);

        const float innerRadius = 80;
        const float outerRadius = innerRadius * 2;
        const float centerRadius = innerRadius + (outerRadius - innerRadius) / 2;

        Vector2 mousePos = GetMousePosition();
        if (!activePrev) {
            position = mousePos; //{ GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };
            CircleConstrainToScreen(position, outerRadius);
        }

        Vector2 toMouse = Vector2Subtract(mousePos, position);
        float toMouseLen2 = Vector2LengthSqr(toMouse);

        if (toMouseLen2 >= innerRadius*innerRadius) {
            // Position on circle, normalized to 0.0 - 1.0 range, clockwise from 12 o'clock
            float pieAlpha = 1.0f - (atan2f(toMouse.x, toMouse.y) / PI + 1.0f) / 2.0f;
            index = pieAlpha * count;
            index = CLAMP(index, 0, count);
        }

#if 1
        // TODO(cleanup): Debug code to increase/decrease # of pie entries
        float wheelMove = io.MouseWheelMove();
        if (wheelMove) {
            int delta = (int)CLAMP(wheelMove, -1, 1) * -1;
            count += delta;
            count = CLAMP(count, 1, 20);
        }
#endif

        const float pieSliceDeg = 360.0f / count;
        const float angleStart = pieSliceDeg * index - 90;
        const float angleEnd = angleStart + pieSliceDeg;
        const Color color = ColorFromHSV(angleStart, 0.7f, 0.7f);

        //DrawCircleV(position, outerRadius, Fade(LIGHTGRAY, 0.6f));
        DrawRing(position, innerRadius, outerRadius, 0, 360, 32, Fade(LIGHTGRAY, 0.6f));

        //DrawCircleSector(position, outerRadius, angleStart, angleEnd, 32, Fade(SKYBLUE, 0.6f));
        DrawRing(position, innerRadius, outerRadius, angleStart, angleEnd, 32, Fade(color, 0.8f));

        for (int i = 0; i < count; i++) {
            // Find middle of pie slice as a percentage of total pie circumference
            const float iconPieAlpha = (float)i / count - 0.25f + (1.0f / count * 0.5f);
            const Vector2 iconCenter = {
                position.x + centerRadius * cosf(2 * PI * iconPieAlpha),
                position.y + centerRadius * sinf(2 * PI * iconPieAlpha)
            };

            const char *menuText = i < ARRAY_SIZE(items) ? items[i] : "-Empty-";

            //DrawCircle(iconCenter.x, iconCenter.y, 2, BLACK);
            Vector2 textSize = MeasureTextEx(fntMedium, menuText, fntMedium.baseSize, 1.0f);
            Vector2 textPos{
                iconCenter.x - textSize.x / 2.0f,
                iconCenter.y - textSize.y / 2.0f
            };
            //DrawTextShadowEx(fntMedium, TextFormat("%d %.2f", i, iconPieAlpha), iconCenter, RED);
            DrawTextShadowEx(fntMedium, menuText, textPos, WHITE);
        }

#if 0
        // TODO(cleanup): Draw debug text on pie menu
        DrawTextShadowEx(fntMedium,
            TextFormat("%.2f (%d of %d)", pieAlpha, index + 1, count),
            center, WHITE
        );
#endif
    }
};

struct ClientWorld {
    Camera2D camera{};

    uint32_t last_target_id{};
    uint32_t last_target_map_id{};
    float zoomTarget{};

    double warpFadeInStartedAt{};

    double titleShownAt{};
    std::string titleText{};

    bool showSnapshotShadows{};

    uint32_t localPlayerEntityId{};
    uint32_t hoveredEntityId{};   // mouse is over entity
    bool hoveredEntityInRange{};  // player is close enough to interact

    Spinner spinner{};

    // For histogram
    float prevX = 0;

    ClientWorld(void);

    data::Entity *LocalPlayer(void);
    uint32_t LocalPlayerMapId(void);
    data::Tilemap *LocalPlayerMap(void);
    data::Tilemap *FindOrLoadMap(uint32_t map_id);

    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);
    void ApplyStateInterpolated(data::Entity &entity, const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt);
    Err CreateDialog(data::Entity &entity, uint32_t dialogId, const std::string &title, const std::string &message, double now);

    void Update(GameClient &client);
    void Draw(GameClient &client);

private:
    struct HistoData {
        uint32_t latestSnapInputSeq;
        double cmdAccumDt;
        Vector3 cmdAccumForce;
    };

    void UpdateMap(data::Tilemap &map);
    void UpdateLocalPlayerHisto(GameClient &client, data::Entity &entity, HistoData &histoData);
    void UpdateLocalPlayer(GameClient &client, data::Entity &entity, data::AspectGhost &ghost);
    void UpdateLocalGhost(GameClient &client, data::Entity &entity, data::AspectGhost &ghost, uint32_t player_map_id);
    void UpdateEntities(GameClient &client);
    void UpdateCamera(GameClient &client);
    void UpdateHUDSpinner(void);

    void DrawHoveredTileIndicator(GameClient &client);
    void DrawEntitySnapshotShadows(GameClient &client, data::Entity &entity, Controller &controller);
    void DrawEntities(GameClient &client, data::Tilemap &map, data::DrawCmdQueue &sortedDraws);
    void DrawHoveredObjectIndicator(GameClient &client, data::Tilemap &map);
    void DrawDialog(GameClient &client, data::Entity &entity, Vector2 bottomCenterScreen, std::vector<FancyTextTip> &tips);
    void DrawDialogTips(std::vector<FancyTextTip> tips);
    void DrawDialogs(GameClient &client);
    void DrawHUDEntityHoverInfo(void);
    void DrawHUDSpinner(void);
    void DrawHUDTitle(GameClient &client);
    void DrawHUDSignEditor(void);
    void DrawHUDMenu(void);
};
