#include "client_world.h"
#include "game_client.h"
#include "../common/collision.h"
#include "../common/dlg.h"
#include "../common/entity_db.h"
#include "../common/io.h"
#include "../common/net/messages/msg_s_entity_spawn.h"
#include "../common/net/messages/msg_s_entity_snapshot.h"

ClientWorld::~ClientWorld(void)
{
    for (Tilemap *map : maps) {
        delete map;
    }
}

data::Entity *ClientWorld::LocalPlayer(void) {
    data::Entity *localPlayer = entityDb->FindEntity(localPlayerEntityId);
    if (localPlayer) {
        assert(localPlayer->type == data::ENTITY_PLAYER);
        if (localPlayer->type == data::ENTITY_PLAYER) {
            return localPlayer;
        }
    }
    return 0;
}

Tilemap *ClientWorld::LocalPlayerMap(void)
{
    data::Entity *localPlayer = LocalPlayer();
    if (localPlayer && localPlayer->map_id) {
        return FindOrLoadMap(localPlayer->map_id);
    }
    return 0;
}

Tilemap *ClientWorld::FindOrLoadMap(uint32_t map_id)
{
    assert(map_id);
    Tilemap *map = 0;

    const auto &mapEntry = mapsById.find(map_id);
    if (mapEntry != mapsById.end()) {
        map = maps[mapEntry->second];
    } else {
        std::string mapName = "unknownMapId";
        switch (map_id) {
            case 1: mapName = LEVEL_001; break;
        }
        // TODO: LoadPack map and add to maps/mapsById

        map = new Tilemap();
        if (map) {
            Err err = map->Load(mapName);
            if (!err) {
                map->id = map_id;
                mapsById[map->id] = maps.size();
                maps.push_back(map);
            } else {
                printf("Failed to load map id %u with code %d\n", map_id, err);
                delete map;
                map = 0;
            }
        } else {
            printf("Failed to allocate space to load map id %u\n", map_id);
        }
    }

    if (map) {
        switch (map->id) {
            case 1: musBackgroundMusic = "mus_ambient_outdoors"; break;
            //case 2: musBackgroundMusic = "mus_ambient_cave"; break;
            default: musBackgroundMusic = "mus_none"; break;
        }
    }

    return map;
}

bool ClientWorld::CopyEntityData(uint32_t entityId, data::EntityData &data)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to copy entity. Entity id %u not found.\n", entityId);
        return false;
    }

    data.entity = entityDb->entities[entityIndex];
    data.ghosts = entityDb->ghosts[entityIndex];
    return true;
}

void ClientWorld::ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn)
{
    size_t entityIndex = entityDb->FindEntityIndex(entitySpawn.entity_id);
    if (!entityIndex) {
        printf("[client_world] Failed to spawn entity. Entity id %u not found.\n", entitySpawn.entity_id);
        return;
    }

    data::Entity &entity = entityDb->entities[entityIndex];

    entity.type     = entitySpawn.type;
    entity.spec     = entitySpawn.spec;
    entity.name     = entitySpawn.name;
    entity.map_id   = entitySpawn.map_id;
    entity.position = entitySpawn.position;
    entity.radius   = entitySpawn.radius;
    entity.drag     = entitySpawn.drag;
    entity.speed    = entitySpawn.speed;
    entity.velocity = entitySpawn.velocity;
    entity.hp_max   = entitySpawn.hp_max;
    entity.hp       = entitySpawn.hp;
    entity.sprite   = entitySpawn.sprite;
}

void ClientWorld::ApplyStateInterpolated(data::Entity &entity,
    const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt)
{
    if (b.map_id != a.map_id) {
        alpha = 1.0f;
    }
    entity.map_id = b.map_id;
    entity.position.x = LERP(a.position.x, b.position.x, alpha);
    entity.position.y = LERP(a.position.y, b.position.y, alpha);

    entity.velocity.x = LERP(a.velocity.x, b.velocity.x, alpha);
    entity.velocity.y = LERP(a.velocity.y, b.velocity.y, alpha);

    entity.hp_max    = b.hp_max;
    entity.hp        = b.hp;
    entity.hp_smooth = LERP(entity.hp_smooth, entity.hp, 1.0f - powf(1.0f - 0.999f, dt));
    //entity.hp_smooth += ((entity.hp_smooth < entity.hp) ? 1 : -1) * dt * 20;
}

void ClientWorld::ApplyStateInterpolated(uint32_t entityId,
    const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt)
{
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity) {
        printf("[client_world] Failed to interpolate entity. Entity id %u not found.\n", entityId);
        return;
    }

    ApplyStateInterpolated(*entity, a, b, alpha, dt);
}

Err ClientWorld::CreateDialog(uint32_t entityId, uint32_t dialogId, std::string message, double now)
{
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity) {
        printf("[client_world] Failed to create dialog. Could not find entity id %u.\n", entityId);
        return RN_OUT_OF_BOUNDS;
    }

    entity->dialog_spawned_at = now;
    entity->dialog_id = dialogId;
    entity->dialog_title = entity->name.size() ? entity->name : "???";
    entity->dialog_message = message;
    return RN_SUCCESS;
}

void ClientWorld::UpdateLocalPlayerHisto(GameClient &client, data::Entity &entity, HistoData &histoData)
{
    if (Histogram::paused) {
        return;
    }

    static Color colors[] = { BEIGE, BROWN };
    static int colorIdx = 0;
    if (histoData.latestSnapInputSeq != histoInput.buffer[histoInput.buffer.size() - 2].value) {
        colorIdx = ((size_t)colorIdx + 1) % ARRAY_SIZE(colors);
    }
    Histogram::Entry &entryInput = histoInput.buffer.newest();
    entryInput.value = histoData.latestSnapInputSeq;
    entryInput.color = colors[colorIdx];
    entryInput.metadata = TextFormat("latestSnapInputSeq: %u", histoData.latestSnapInputSeq);

    static float prevX = 0;
    Histogram::Entry &entryDx = histoDx.buffer.newest();
    entryDx.value = entity.position.x - prevX;
    prevX = entity.position.x;
    entryDx.metadata = TextFormat(
        "plr_x     %.3f\n"
        "plr_vx    %.3f\n"
        "plr_move  %.3f, %.3f\n"
        "svr_now   %.3f\n"
        "cmd_accum %.3f",
        entity.position.x,
        entity.velocity.x,
        histoData.cmdAccumDt > 0 ? histoData.cmdAccumForce.x : 0,
        histoData.cmdAccumDt > 0 ? histoData.cmdAccumForce.y : 0,
        client.ServerNow(),
        histoData.cmdAccumDt
    );
}

void ClientWorld::UpdateLocalPlayer(GameClient &client, data::Entity &entity, data::AspectGhost &ghost)
{
    uint32_t latestSnapInputSeq = 0;

    // Apply latest snapshot
    const data::GhostSnapshot &latestSnapshot = ghost.newest();
    if (latestSnapshot.server_time) {
        ApplyStateInterpolated(entity.id, ghost.newest(), ghost.newest(), 0, client.frameDt);
        latestSnapInputSeq = latestSnapshot.last_processed_input_cmd;
    }

#if CL_CLIENT_SIDE_PREDICT
    double cmdAccumDt{};
    Vector2 cmdAccumForce{};
    Tilemap *map = FindOrLoadMap(entity.map_id);
    if (map) {
        // Apply unacked input
        for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
            InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
            if (inputCmd.seq > latestSnapInputSeq) {
                entity.ApplyForce(inputCmd.GenerateMoveForce(entity.speed));
                entityDb->EntityTick(entity.id, SV_TICK_DT);
                map->ResolveEntityTerrainCollisions(entity.id);
            }
        }

        cmdAccumDt = client.now - client.controller.lastInputSampleAt;
        if (cmdAccumDt > 0) {
            Vector2 posBefore = entity.position;
            cmdAccumForce = client.controller.cmdAccum.GenerateMoveForce(entity.speed);
            entity.ApplyForce(cmdAccumForce);
            entityDb->EntityTick(entity.id, SV_TICK_DT);
            map->ResolveEntityTerrainCollisions(entity.id);
            entity.position.x = LERP(posBefore.x, entity.position.x, cmdAccumDt / SV_TICK_DT);
            entity.position.y = LERP(posBefore.y, entity.position.y, cmdAccumDt / SV_TICK_DT);
        }
    }
#endif

    // Check for ignored input packets
    uint32_t oldestInput = client.controller.cmdQueue.oldest().seq;
    if (oldestInput > latestSnapInputSeq + 1) {
        printf(" localPlayer: %d inputs dropped\n", oldestInput - latestSnapInputSeq - 1);
    }

    HistoData histoData{};
    histoData.latestSnapInputSeq = latestSnapInputSeq;
    histoData.cmdAccumDt = cmdAccumDt;
    histoData.cmdAccumForce = cmdAccumForce;
    UpdateLocalPlayerHisto(client, entity, histoData);
}

void ClientWorld::UpdateLocalGhost(GameClient &client, data::Entity &entity, data::AspectGhost &ghost, Tilemap *localPlayerMap)
{
    // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
    const double renderAt = client.ServerNow() - SV_TICK_DT;

    size_t snapshotBIdx = 0;
    while (snapshotBIdx < ghost.size()
        && ghost[snapshotBIdx].server_time <= renderAt)
    {
        snapshotBIdx++;
    }

    const data::GhostSnapshot *snapshotA = 0;
    const data::GhostSnapshot *snapshotB = 0;

    if (snapshotBIdx <= 0) {
        snapshotA = &ghost.oldest();
        snapshotB = &ghost.oldest();
    } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
        snapshotA = &ghost.newest();
        snapshotB = &ghost.newest();
    } else {
        snapshotA = &ghost[snapshotBIdx - 1];
        snapshotB = &ghost[snapshotBIdx];
    }

    float alpha = 0;
    if (snapshotB != snapshotA) {
        alpha = (renderAt - snapshotA->server_time) / (snapshotB->server_time - snapshotA->server_time);
    }

    ApplyStateInterpolated(entity.id, *snapshotA, *snapshotB, alpha, client.frameDt);

    if (localPlayerMap && entity.map_id == localPlayerMap->id) {
        const Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);
        bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, entityDb->EntityRect(entity.id));
        if (hover) {
            hoveredEntityId = entity.id;
        }
    }
}

void ClientWorld::UpdateEntities(GameClient &client)
{
    hoveredEntityId = 0;
    Tilemap *localPlayerMap = LocalPlayerMap();

    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type) {
            continue;
        }

        uint32_t entityIndex = entityDb->FindEntityIndex(entity.id);
        data::AspectGhost &ghost = entityDb->ghosts[entityIndex];

        // Local player
        if (entity.id == localPlayerEntityId) {
            UpdateLocalPlayer(client, entity, ghost);
        } else {
            UpdateLocalGhost(client, entity, ghost, localPlayerMap);
        }

        bool newlySpawned = entity.spawned_at == client.now;
        data::UpdateSprite(entity, client.frameDt, newlySpawned);

        const double duration = CL_DIALOG_DURATION_MIN + CL_DIALOG_DURATION_PER_CHAR * entity.dialog_message.size();
        if (entity.dialog_spawned_at && client.now - entity.dialog_spawned_at > duration) {
            entity.ClearDialog();
        }
    }

    if (hoveredEntityId) {
        data::Entity *hoveredEntity = entityDb->FindEntity(hoveredEntityId);
        // NOTE: This has to check for ENTITY_TOWNFOLK or entity.attackable, etc.,
        // otherwise you can't shoot da enemies!
        if (hoveredEntity) {
            if (hoveredEntity->spec == data::ENTITY_SPEC_NPC_TOWNFOLK) {
                io.CaptureMouse();
            }

            bool pressed = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
            if (pressed) {
                client.SendEntityInteract(hoveredEntityId);
            }
        }
    }
}

void ClientWorld::Update(GameClient &client)
{
    io.PushScope(IO::IO_GameNPC);
    UpdateEntities(client);
    io.PopScope();

    if ((int)fmod(client.now * 100, 100) == GetRandomValue(0, 9)) {
        data::PlaySound("sfx_chicken_cluck");
    }
}

void ClientWorld::DrawEntitySnapshotShadows(uint32_t entityId, Controller &controller, double now, double dt)
{
    // TODO: Everything that says "ghostInstance" needs to be an entity_id, but we don't
    // want to modify the actual entity... so perhaps we need a "temp" entity that we can
    // use for drawing shadows? Or some other way to simulate the entity moving without
    // modifying the actual entity.
    if (showSnapshotShadows) {
        size_t entityIndex = entityDb->FindEntityIndex(entityId);
        if (!entityIndex) return;

        data::Entity &entity = entityDb->entities[entityIndex];
        data::AspectGhost &ghost = entityDb->ghosts[entityIndex];

        data::EntityData ghostData{};
        CopyEntityData(entity.id, ghostData);
        // Prevents accidentally overwriting real entities if someone we pass a tuple
        // to decides to look up the entity by id instead of using the tuple's data.
        ghostData.entity.id = 0;

        for (int i = 0; i < ghost.size(); i++) {
            if (!ghost[i].server_time) {
                continue;
            }
            ApplyStateInterpolated(ghostData.entity, ghost[i], ghost[i], 0, dt);

            const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
            Rectangle ghostRect = entityDb->EntityRect(ghostData.entity);
            ghostRect = RectShrink(ghostRect, scalePer);
            DrawRectangleRec(ghostRect, Fade(RED, 0.1f));
            DrawRectangleLinesEx(ghostRect, 1, Fade(RED, 0.8f));
        }

        // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
        if (entity.id == localPlayerEntityId) {
#if CL_CLIENT_SIDE_PREDICT
            Tilemap *map = FindOrLoadMap(entity.map_id);
            if (map) {
                uint32_t lastProcessedInputCmd = 0;
                const data::GhostSnapshot &latestSnapshot = ghost.newest();
                if (latestSnapshot.server_time) {
                    ApplyStateInterpolated(ghostData.entity, latestSnapshot, latestSnapshot, 0, dt);
                    lastProcessedInputCmd = latestSnapshot.last_processed_input_cmd;
                }

                for (size_t cmdIndex = 0; cmdIndex < controller.cmdQueue.size(); cmdIndex++) {
                    InputCmd &inputCmd = controller.cmdQueue[cmdIndex];
                    if (inputCmd.seq > lastProcessedInputCmd) {
                        ghostData.entity.ApplyForce(inputCmd.GenerateMoveForce(ghostData.entity.speed));
                        entityDb->EntityTick(ghostData.entity, SV_TICK_DT);
                        map->ResolveEntityTerrainCollisions(ghostData.entity);
                        Rectangle ghostRect = entityDb->EntityRect(ghostData.entity);
                        DrawRectangleRec(ghostRect, Fade(GREEN, 0.1f));
                        DrawRectangleLinesEx(ghostRect, 1, Fade(GREEN, 0.8f));
                    }
                }
            }

#if 1
            // We don't really need to draw a blue rect at currently entity position unless
            // the entity is invisible.
             const double cmdAccumDt = now - controller.lastInputSampleAt;
            if (cmdAccumDt > 0) {
                Vector2 posBefore = ghostData.entity.position;
                Vector2 cmdAccumForce = controller.cmdAccum.GenerateMoveForce(ghostData.entity.speed);
                ghostData.entity.ApplyForce(cmdAccumForce);
                entityDb->EntityTick(ghostData.entity, SV_TICK_DT);
                map->ResolveEntityTerrainCollisions(ghostData.entity);
                ghostData.entity.position.y = LERP(posBefore.y, ghostData.entity.position.y, cmdAccumDt / SV_TICK_DT);
                ghostData.entity.position.x = LERP(posBefore.x, ghostData.entity.position.x, cmdAccumDt / SV_TICK_DT);
            }
            Rectangle ghostRect = entityDb->EntityRect(ghostData.entity);
            ghostRect.x = floorf(ghostRect.x);
            ghostRect.y = floorf(ghostRect.y);
            DrawRectangleLinesEx(ghostRect, 1, Fade(BLUE, 0.8f));
#endif
#endif
        }
    }
}

void ClientWorld::DrawDialogTips(std::vector<FancyTextTip> tips)
{
    // Render tooltips last
    for (const FancyTextTip &tip : tips) {
        const char *tipText = TextFormat("%.*s", tip.tipLen, tip.tip);
        Vector2 tipPos = Vector2Add(GetMousePosition(), { 0, 20 });
        tipPos.x = floorf(tipPos.x);
        tipPos.y = floorf(tipPos.y);
        Vector2 tipSize = MeasureTextEx(*tip.font, tipText, tip.font->baseSize, 1);

        Rectangle tipRec{
            tipPos.x, tipPos.y,
            tipSize.x, tipSize.y
        };
        tipRec = RectGrow(tipRec, 8);

        Vector2 constrainedOffset{};
        tipRec = RectConstrainToScreen(tipRec, &constrainedOffset);
        tipPos = Vector2Add(tipPos, constrainedOffset);

        dlb_DrawNPatch(tipRec);
        dlb_DrawTextEx(*tip.font, tip.tip, tip.tipLen, tipPos, RAYWHITE);
    }
}

// TODO(dlb): Where should this live? Probably not in ClientWorld?
void ClientWorld::DrawDialog(GameClient &client, data::Entity &entity, Vector2 bottomCenterScreen, std::vector<FancyTextTip> &tips)
{
    Font &font = fntMedium;

    bottomCenterScreen.x = floorf(bottomCenterScreen.x);
    bottomCenterScreen.y = floorf(bottomCenterScreen.y);

    const float marginBottom = 4.0f;
    const Vector2 bgPad{ 12, 8 };

    const Vector2 titleSize = MeasureTextEx(font, entity.dialog_title.c_str(), font.baseSize, 1);

    DialogNodeList msgNodes{};
    char *msgBuf = (char *)entity.dialog_message.c_str();
    Err err = ParseMessage(msgBuf, msgNodes);
    if (err) {
        return;
    }

    Vector2 msgCursor{};
    Vector2 msgSize{};
    for (DialogNode &node : msgNodes) {
        Vector2 nodeSize = dlb_MeasureTextEx(font, node.text.data(), node.text.size(), &msgCursor);
        msgSize.x = MAX(msgSize.x, nodeSize.x);
        msgSize.y = msgCursor.y + nodeSize.y;
    }

    const float bgHeight =
        bgPad.y +
        titleSize.y +
        bgPad.y +
        msgSize.y +
        bgPad.y;

    const float bgTop = bottomCenterScreen.y - marginBottom - bgHeight;
    float cursorY = bgTop + bgPad.y;

    Vector2 titlePos{
        floorf(bottomCenterScreen.x - titleSize.x / 2),
        cursorY
    };
    //titlePos.x = floorf(titlePos.x);
    //titlePos.y = floorf(titlePos.y);

    cursorY += titleSize.y;
    cursorY += bgPad.y;

    Vector2 msgPos{
        floorf(bottomCenterScreen.x - msgSize.x / 2),
        cursorY
    };
    //msgPos.x = floorf(msgPos.x);
    //msgPos.y = floorf(msgPos.y);

    Rectangle bgRect{
        MIN(titlePos.x, msgPos.x) - bgPad.x,
        bgTop,
        MAX(titleSize.x, msgSize.x) + bgPad.x * 2,
        bgHeight
    };

    Vector2 constrainedOffset{};
    bgRect = RectConstrainToScreen(bgRect, &constrainedOffset);
    titlePos = Vector2Add(titlePos, constrainedOffset);
    msgPos = Vector2Add(msgPos, constrainedOffset);

    //msgBgRect.x = floorf(msgBgRect.x);
    //msgBgRect.y = floorf(msgBgRect.y);
    //msgBgRect.width = floorf(msgBgRect.width);
    //msgBgRect.height = floorf(msgBgRect.height);

    if (dlb_CheckCollisionPointRec(GetMousePosition(), bgRect)) {
        io.CaptureMouse();
    }

    printf("%.02f %.02f %.02f %.02f\n", bgRect.x, bgRect.y, bgRect.width, bgRect.height);
    dlb_DrawNPatch(bgRect);
    //DrawRectangleRounded(msgBgRect, 0.2f, 6, Fade(BLACK, 0.5));
    //DrawRectangleRoundedLines(msgBgRect, 0.2f, 6, 1.0f, RAYWHITE);
    DrawTextEx(font, entity.dialog_title.c_str(), titlePos, font.baseSize, 1.0f, GOLD);

    float separatorY = titlePos.y + titleSize.y + bgPad.y / 2;
    DrawLine(
        bgRect.x + bgPad.x - 2,
        separatorY,
        bgRect.x + bgRect.width - bgPad.x + 2,
        separatorY + 1,
        DIALOG_SEPARATOR_GOLD
    );

    Vector2 nodeCursor{};
    uint32_t nodeIdx = 0;
    for (DialogNode &node : msgNodes) {
        Color col = RAYWHITE;
        bool hoverable = false;
        switch (node.type) {
            case DIALOG_NODE_TEXT:      col = RAYWHITE;         break;
            case DIALOG_NODE_LINK:      col = SKYBLUE;          hoverable = true; break;
            case DIALOG_NODE_HOVER_TIP: col = FANCY_TIP_YELLOW; hoverable = true; break;
        }
        //col = ColorFromHSV((float)i / tree.nodes.size() * 360.0f, 0.8f, 0.8f);
        bool hovered = false;
        bool *hoveredPtr = hoverable ? &hovered : 0;
        dlb_DrawTextEx(font, node.text.data(), node.text.size(), msgPos, col, &nodeCursor, hoveredPtr);

        switch (node.type) {
            case DIALOG_NODE_LINK: {
                const bool pressed = hovered && io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
                if (pressed) {
                    client.SendEntityInteractDialogOption(entity, nodeIdx);
                }
                break;
            }
            case DIALOG_NODE_HOVER_TIP: {
                if (hovered) {
                    FancyTextTip tip{};
                    tip.font = &font;
                    tip.tip = node.data.data();
                    tip.tipLen = node.data.size();
                    tips.push_back(tip);
                }
                break;
            }
        }
        nodeIdx++;
    }
}

void ClientWorld::DrawDialogs(GameClient &client, Camera2D &camera)
{
    io.PushScope(IO::IO_GameNPCDialog);

    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    std::vector<FancyTextTip> tips{};

    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at || entity.map_id != map->id) {
            continue;
        }
        assert(entity.id);

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        if (entity.dialog_spawned_at) {
            const Vector2 topCenter = entityDb->EntityTopCenter(entity.id);
            const Vector2 topCenterScreen = GetWorldToScreen2D(topCenter, camera);
            DrawDialog(client, entity, topCenterScreen, tips);
        }
    }

    DrawDialogTips(tips);

    io.PopScope();
}

void ClientWorld::Draw(GameClient &client)
{
    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);

    //--------------------
    // Draw the map
    BeginMode2D(camera2d);

    //BeginShaderMode(shdPixelFixer);
    map->Draw(camera2d);
    //EndShaderMode();

    //--------------------
    // Draw the entities
    cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);

    struct EntityDrawCmd {
        uint32_t entity_id {};
        float    y_pos     {};

        EntityDrawCmd(uint32_t entity_id, float y_pos) : entity_id(entity_id), y_pos(y_pos) {}
        bool operator<(const EntityDrawCmd& rhs) const {
            return y_pos > rhs.y_pos;  // backwards on purpose cus y is inverted
        }
    };
    std::priority_queue<EntityDrawCmd> sortedEntityIds{};

    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at || entity.map_id != map->id) {
            continue;
        }
        assert(entity.id);
        sortedEntityIds.emplace(entity.id, entity.position.y);
    }

    while (!sortedEntityIds.empty()) {
        const EntityDrawCmd &drawCmd = sortedEntityIds.top();
        DrawEntitySnapshotShadows(drawCmd.entity_id, client.controller, client.now, client.frameDt);
        entityDb->DrawEntity(drawCmd.entity_id);
        sortedEntityIds.pop();
    }

    EndMode2D();

    //--------------------
    // Draw the dialogs
    DrawDialogs(client, camera2d);

    //--------------------
    // Draw entity info
    if (hoveredEntityId) {
        data::Entity *entity = entityDb->FindEntity(hoveredEntityId);
        if (entity) {
            entityDb->DrawEntityHoverInfo(hoveredEntityId);
        } else {
            // We were probably hovering an entity while it was despawning?
            assert(!"huh? how can we hover an entity that doesn't exist");
            hoveredEntityId = 0;
        }
    }
}