#include "client_world.h"
#include "game_client.h"
#include "../common/collision.h"
#include "../common/dlg.h"
#include "../common/entity_db.h"
#include "../common/io.h"

ClientWorld::ClientWorld(void)
{
    camera.zoom = 1.0f;
    zoomTarget = camera.zoom;
}

Entity *ClientWorld::LocalPlayer(void)
{
    Entity *localPlayer = entityDb->FindEntity(localPlayerEntityId, Entity::TYP_PLAYER);
    return localPlayer;
}
uint16_t ClientWorld::LocalPlayerMapId(void)
{
    Entity *localPlayer = LocalPlayer();
    if (localPlayer && localPlayer->map_id) {
        return localPlayer->map_id;
    }
    return 0;
}
Tilemap *ClientWorld::LocalPlayerMap(void)
{
    Entity *localPlayer = LocalPlayer();
    if (localPlayer && localPlayer->map_id) {
        return &FindOrLoadMap(localPlayer->map_id);
    }
    return 0;
}
Tilemap &ClientWorld::FindOrLoadMap(uint16_t map_id)
{
    Tilemap *map = pack_maps.FindByIdTry<Tilemap>(map_id);
    if (map) {
        return *map;
    }

    Tilemap &new_map = pack_maps.tile_maps.emplace_back();
    new_map.id = map_id;
    pack_maps.dat_by_id[DAT_TYP_TILE_MAP][map_id] = pack_maps.tile_maps.size() - 1;
    return new_map;
}

void ClientWorld::ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn)
{
    Entity *entity = entityDb->FindEntity(entitySpawn.entity_id);
    if (!entity) {
        printf("[client_world] Failed to spawn entity. Entity id %u not found.\n", entitySpawn.entity_id);
        return;
    }

    entity->type      = entitySpawn.type;
    entity->spec      = entitySpawn.spec;
    entity->name      = entitySpawn.name;
    entity->map_id    = entitySpawn.map_id;
    entity->position  = entitySpawn.position;
    entity->radius    = entitySpawn.radius;
    entity->drag      = entitySpawn.drag;
    entity->speed     = entitySpawn.speed;
    entity->velocity  = entitySpawn.velocity;
    entity->hp_max    = entitySpawn.hp_max;
    entity->hp        = entitySpawn.hp;
    entity->sprite_id = entitySpawn.sprite_id;
}
void ClientWorld::ApplyStateInterpolated(Entity &entity,
    const GhostSnapshot &a, const GhostSnapshot &b, float alpha, float dt)
{
    if (b.map_id != a.map_id) {
        alpha = 1.0f;
    }
    entity.map_id = b.map_id;
    entity.position.x = LERP(a.position.x, b.position.x, alpha);
    entity.position.y = LERP(a.position.y, b.position.y, alpha);
    entity.on_warp_cooldown = b.on_warp_cooldown;

    entity.velocity.x = LERP(a.velocity.x, b.velocity.x, alpha);
    entity.velocity.y = LERP(a.velocity.y, b.velocity.y, alpha);

    entity.hp_max    = b.hp_max;
    entity.hp        = b.hp;
    entity.hp_smooth = LERP(entity.hp_smooth, entity.hp, 1.0f - powf(1.0f - 0.999f, dt));
    //entity.hp_smooth += ((entity.hp_smooth < entity.hp) ? 1 : -1) * dt * 20;
}
Err ClientWorld::CreateDialog(Entity &entity, uint16_t dialogId, const std::string &title, const std::string &message, double now)
{
    entity.dialog_spawned_at = now;
    entity.dialog_id = dialogId;
    entity.dialog_title = title;
    entity.dialog_message = message;
    return RN_SUCCESS;
}

void ClientWorld::UpdateLocalPlayerHisto(GameClient &client, Entity &entity, HistoData &histoData)
{
    if (Histogram::paused) {
        return;
    }

    // TODO: Move these to histogram class
    static Color colors[] = { BEIGE, BROWN };
    static int colorIdx = 0;
    if (histoData.latestSnapInputSeq != histoInput.buffer[histoInput.buffer.size() - 2].value) {
        colorIdx = ((size_t)colorIdx + 1) % ARRAY_SIZE(colors);
    }
    Histogram::Entry &entryInput = histoInput.buffer.newest();
    entryInput.value = histoData.latestSnapInputSeq;
    entryInput.color = colors[colorIdx];
    entryInput.metadata = TextFormat("latestSnapInputSeq: %u", histoData.latestSnapInputSeq);

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
void ClientWorld::UpdateLocalPlayer(GameClient &client, Entity &entity)
{
    uint8_t latestSnapInputSeq = 0;

    // Apply latest snapshot
    const GhostSnapshot &latestSnapshot = entity.ghost->newest();
    if (latestSnapshot.server_time) {
        ApplyStateInterpolated(entity, entity.ghost->newest(), entity.ghost->newest(), 0, client.frameDt);
        latestSnapInputSeq = latestSnapshot.last_processed_input_cmd;
    }

    double cmdAccumDt{};
    Vector3 cmdAccumForce{};

#if CL_CLIENT_SIDE_PREDICT
    Tilemap &map = FindOrLoadMap(entity.map_id);

    // Apply unacked input
    for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
        InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
        if (paws_greater(inputCmd.seq, latestSnapInputSeq)) {
            entity.ApplyForce(inputCmd.GenerateMoveForce(entity.speed));
            entityDb->EntityTick(entity, SV_TICK_DT, client.now);
            map.ResolveEntityCollisionsEdges(entity);
        }
    }

    cmdAccumDt = client.now - client.controller.lastInputSampleAt;
    if (cmdAccumDt > 0) {
        Vector3 posBefore = entity.position;
        cmdAccumForce = client.controller.cmdAccum.GenerateMoveForce(entity.speed);
        entity.ApplyForce(cmdAccumForce);
        entityDb->EntityTick(entity, SV_TICK_DT, client.now);
        map.ResolveEntityCollisionsEdges(entity);

        entity.position.x = LERP(posBefore.x, entity.position.x, cmdAccumDt / SV_TICK_DT);
        entity.position.y = LERP(posBefore.y, entity.position.y, cmdAccumDt / SV_TICK_DT);
    }
#endif

    // Check for ignored input packets
    uint8_t oldestInput = client.controller.cmdQueue.oldest().seq;
    if (paws_greater(oldestInput, (uint8_t)(latestSnapInputSeq + 1))) {
        printf(" localPlayer: %d inputs dropped\n", oldestInput - latestSnapInputSeq - 1);
    }

    HistoData histoData{};
    histoData.latestSnapInputSeq = latestSnapInputSeq;
    histoData.cmdAccumDt = cmdAccumDt;
    histoData.cmdAccumForce = cmdAccumForce;
    UpdateLocalPlayerHisto(client, entity, histoData);
}
void ClientWorld::UpdateLocalGhost(GameClient &client, Entity &entity, uint16_t player_map_id)
{
    // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
    const double renderAt = client.ServerNow() - SV_TICK_DT;

    size_t snapshotBIdx = 0;
    while (snapshotBIdx < entity.ghost->size()
        && (*entity.ghost)[snapshotBIdx].server_time <= renderAt)
    {
        snapshotBIdx++;
    }

    const GhostSnapshot *snapshotA = 0;
    const GhostSnapshot *snapshotB = 0;

    if (snapshotBIdx <= 0) {
        snapshotA = &entity.ghost->oldest();
        snapshotB = &entity.ghost->oldest();
    } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
        snapshotA = &entity.ghost->newest();
        snapshotB = &entity.ghost->newest();
    } else {
        snapshotA = &(*entity.ghost)[snapshotBIdx - 1];
        snapshotB = &(*entity.ghost)[snapshotBIdx];
    }

    float alpha = 0;
    if (snapshotB != snapshotA) {
        alpha = (renderAt - snapshotA->server_time) / (snapshotB->server_time - snapshotA->server_time);
    }

    ApplyStateInterpolated(entity, *snapshotA, *snapshotB, alpha, client.frameDt);

    if (entity.map_id == player_map_id) {
        const Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        const Rectangle rect = entity.GetSpriteRect();
        bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, rect);
        if (hover) {
            hoveredEntityId = entity.id;
            if (entity.spec == Entity::SPC_NPC_TOWNFOLK) {
                // NOTE: This has to check for ENTITY_TOWNFOLK or entity.attackable, etc.,
                // otherwise you can't shoot da enemies!
                io.PushScope(IO::IO_GameNPC);
                io.CaptureMouse();
                io.PopScope();
            }

            Entity *e_player = entityDb->FindEntity(localPlayerEntityId, Entity::TYP_PLAYER);
            if (e_player) {
                const float dist_x = fabs(e_player->position.x - entity.position.x);
                const float dist_y = fabs(e_player->position.y - entity.position.y);
                if (dist_x <= SV_MAX_ENTITY_INTERACT_DIST && dist_y <= SV_MAX_ENTITY_INTERACT_DIST) {
                    hoveredEntityInRange = true;
                }
            } else {
                assert(!"player entity not found!?");
            }
        }
    }
}
void ClientWorld::UpdateEntities(GameClient &client)
{
    hoveredEntityId = 0;
    hoveredEntityInRange = false;

    const uint16_t player_map_id = LocalPlayerMapId();

    Entity *e_player = entityDb->FindEntity(localPlayerEntityId, Entity::TYP_PLAYER);
    if (!e_player) {
        assert(!"player entity not found!?");
        return;
    }

    for (Entity &entity : entityDb->entities) {
        if (!entity.type) {
            continue;
        }

        // Local player
        if (entity.id == localPlayerEntityId) {
            UpdateLocalPlayer(client, entity);
        } else {
            UpdateLocalGhost(client, entity, player_map_id);
        }

        bool newlySpawned = entity.spawned_at == client.now;
        UpdateSprite(entity, client.frameDt, newlySpawned);

        const double duration = CL_DIALOG_DURATION_MIN + CL_DIALOG_DURATION_PER_CHAR * entity.dialog_message.size();
        if (entity.dialog_spawned_at && client.now - entity.dialog_spawned_at > duration) {
            entity.ClearDialog();
        }
    }

    if (hoveredEntityId && hoveredEntityInRange) {
        Entity *hoveredEntity = entityDb->FindEntity(hoveredEntityId);
        if (hoveredEntity) {
            bool pressed = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
            if (pressed) {
                client.SendEntityInteract(hoveredEntityId);
            }
        }
    }
}
void ClientWorld::UpdateCamera(GameClient &client)
{
    Entity *localPlayer = LocalPlayer();
    if (!localPlayer) {
        return;
    }

    Entity &target = *localPlayer;
    Vector2 targetPos = target.Position2D();

    camera.offset = {
        /*floorf(*/GetRenderWidth ()/2.0f/*)*/,
        /*floorf(*/GetRenderHeight()/2.0f/*)*/
    };

    if (!io.KeyDown(KEY_SPACE)) {
#if CL_CAMERA_LERP
        // https://www.gamedeveloper.com/programming/improved-lerp-smoothing-
        const float halfLife = 8.0f;
        float alpha = 1.0f - exp2f(-halfLife * frameDt);
        camera.target.x = LERP(camera.target.x, targetPos.x, alpha);
        camera.target.y = LERP(camera.target.y, targetPos.y, alpha);

#else
        camera.target.x = targetPos.x;
        camera.target.y = targetPos.y;
#endif
    }

    // Some maps are too small for this to work.. so I just disabled it for now
    //if (map) {
    //Vector2 mapSize = { (float)map->width * TILE_W, map->height * (float)TILE_W };
    //camera.target.x = CLAMP(camera.target.x, camera.offset.x / camera.zoom, mapSize.x - camera.offset.x / camera.zoom);
    //camera.target.y = CLAMP(camera.target.y, camera.offset.y / camera.zoom, mapSize.y - camera.offset.y / camera.zoom);
    //}

    // Snap camera when new entity target, or target entity changes maps
    const bool new_target = target.id != last_target_id;
    const bool new_map = target.map_id != last_target_map_id;

    // Teleport camera to new location
    if (new_target || new_map) {
        camera.target.x = targetPos.x;
        camera.target.y = targetPos.y;
        last_target_id = target.id;
        last_target_map_id = target.map_id;
    }

    if (new_map) {
        warpFadeInStartedAt = client.now;
    }

    // Zoom based on mouse wheel
    float wheel = io.MouseWheelMove();
    if (wheel != 0) {
        // Get the world point that is under the mouse
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        // Zoom increment
        const float zoomIncrement = 0.1f;
        zoomTarget += (wheel * zoomIncrement * zoomTarget);
        zoomTarget = MAX(zoomIncrement, zoomTarget);
    }
    if (fabsf(camera.zoom - zoomTarget) > 0.001f) {
        const float halfLife = 8.0f;
        float alpha = 1.0f - exp2f(-halfLife * client.frameDt);
        camera.zoom = LERP(camera.zoom, zoomTarget, alpha);
    }

    if (io.KeyPressed(KEY_KP_1)) {
        zoomTarget = 1;
    } else if (io.KeyPressed(KEY_KP_2)) {
        zoomTarget = 2;
    }

    camera.target.x = floorf(camera.target.x);
    camera.target.y = floorf(camera.target.y);
    camera.offset.x = floorf(camera.offset.x);
    camera.offset.y = floorf(camera.offset.y);
}
void ClientWorld::Update(GameClient &client)
{
    UpdateTileDefAnimations(client.frameDt);
    Tilemap *map = LocalPlayerMap();
    if (map) {
        map->Update(client.now, false);
    } else {
        assert(!"should this ever happen?");
    }

    io.PushScope(IO::IO_GameNPC);
    UpdateEntities(client);
    io.PopScope();

    UpdateCamera(client);
    spinner.Update();

    // TODO: Make this based on something (at the very least, the # of chickens nearby)
    static double next_cluck_at = 0;
    if (client.now >= next_cluck_at) {
        PlaySound("sfx_chicken_cluck");
        next_cluck_at = client.now + 5 + GetRandomFloatVariance(4);
    }
}

void ClientWorld::DrawHoveredTileIndicator(GameClient &client)
{
    if (client.controller.tile_hovered && spinner.ItemIsTool()) {
        Rectangle tileRect{
            floorf((float)client.controller.tile_x * TILE_W),
            floorf((float)client.controller.tile_y * TILE_W),
            floorf(TILE_W),
            floorf(TILE_W)
        };
        DrawRectangleLinesEx(tileRect, 2.0f, WHITE);
    }
}
void ClientWorld::DrawEntitySnapshotShadows(GameClient &client, Entity &entity, DrawCmdQueue &sortedDraws, Controller &controller)
{
    // TODO: Everything that says "ghostInstance" needs to be an entity_id, but we don't
    // want to modify the actual entity... so perhaps we need a "temp" entity that we can
    // use for drawing shadows? Or some other way to simulate the entity moving without
    // modifying the actual entity.
    struct EntityData {
        Entity      entity {};
        AspectGhost ghost  {};
    };

    AspectGhost &ghost = *entity.ghost;

    EntityData ghostData{};
    ghostData.entity = entity;
    ghostData.ghost = *entity.ghost;
    // Prevents accidentally overwriting real entities if someone we pass a tuple
    // to decides to look up the entity by id instead of using the tuple's data.
    ghostData.entity.id = 0;

    const bool resting = (client.ServerNow() - entity.ghost->newest().server_time) > SV_TICK_DT * 3;
    Color snapColor = resting ? GRAY : RED;

    for (int i = 0; i < ghost.size(); i++) {
        if (!ghost[i].server_time) {
            continue;
        }
        ApplyStateInterpolated(ghostData.entity, ghost[i], ghost[i], 0, client.frameDt);

        //const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
        Rectangle ghostRect = ghostData.entity.GetSpriteRect();
        //ghostRect = RectShrink(ghostRect, scalePer);
        ghostRect.x = floorf(ghostRect.x);
        ghostRect.y = floorf(ghostRect.y);
        ghostRect.width = floorf(ghostRect.width);
        ghostRect.height = floorf(ghostRect.height);
        sortedDraws.push(DrawCmd::RectSolid(ghostRect, Fade(snapColor, 0.1f)));
        sortedDraws.push(DrawCmd::RectOutline(ghostRect, Fade(snapColor, 0.8f), 1));
    }

    // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
    if (entity.id == localPlayerEntityId) {
#if CL_CLIENT_SIDE_PREDICT
        Tilemap &map = FindOrLoadMap(entity.map_id);
        uint8_t lastProcessedInputCmd = 0;
        const GhostSnapshot &latestSnapshot = ghost.newest();
        if (latestSnapshot.server_time) {
            ApplyStateInterpolated(ghostData.entity, latestSnapshot, latestSnapshot, 0, client.frameDt);
            lastProcessedInputCmd = latestSnapshot.last_processed_input_cmd;
        }

        for (size_t cmdIndex = 0; cmdIndex < controller.cmdQueue.size(); cmdIndex++) {
            InputCmd &inputCmd = controller.cmdQueue[cmdIndex];
            if (paws_greater(inputCmd.seq, lastProcessedInputCmd)) {
                ghostData.entity.ApplyForce(inputCmd.GenerateMoveForce(ghostData.entity.speed));
                entityDb->EntityTick(ghostData.entity, SV_TICK_DT, client.now);
                map.ResolveEntityCollisionsEdges(ghostData.entity);
                Rectangle ghostRect = ghostData.entity.GetSpriteRect();
                sortedDraws.push(DrawCmd::RectSolid(ghostRect, Fade(GREEN, 0.1f)));
                sortedDraws.push(DrawCmd::RectOutline(ghostRect, Fade(GREEN, 0.8f), 1));
            }
        }

#if 1
        // We don't really need to draw a blue rect at currently entity position unless
        // the entity is invisible.
        const double cmdAccumDt = client.now - controller.lastInputSampleAt;
        if (cmdAccumDt > 0) {
            Vector3 posBefore = ghostData.entity.position;
            Vector3 cmdAccumForce = controller.cmdAccum.GenerateMoveForce(ghostData.entity.speed);
            ghostData.entity.ApplyForce(cmdAccumForce);
            entityDb->EntityTick(ghostData.entity, SV_TICK_DT, client.now);
            map.ResolveEntityCollisionsEdges(ghostData.entity);
            ghostData.entity.position.y = LERP(posBefore.y, ghostData.entity.position.y, cmdAccumDt / SV_TICK_DT);
            ghostData.entity.position.x = LERP(posBefore.x, ghostData.entity.position.x, cmdAccumDt / SV_TICK_DT);
        }
        Rectangle ghostRect = ghostData.entity.GetSpriteRect();
        ghostRect.x = floorf(ghostRect.x);
        ghostRect.y = floorf(ghostRect.y);
        ghostRect.width = floorf(ghostRect.width);
        ghostRect.height = floorf(ghostRect.height);
        sortedDraws.push(DrawCmd::RectOutline(ghostRect, Fade(BLUE, 0.8f), 1));
#endif
#endif
    }
}
void ClientWorld::DrawEntities(GameClient &client, Tilemap &map, DrawCmdQueue &sortedDraws)
{
    if (showSnapshotShadows) {
        for (Entity &entity : entityDb->entities) {
            if (!entity.type || entity.despawned_at || entity.map_id != map.id) {
                continue;
            }
            assert(entity.id);
            DrawEntitySnapshotShadows(client, entity, sortedDraws, client.controller);
        }
    }

    for (Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at || entity.map_id != map.id) {
            continue;
        }
        assert(entity.id);
        entityDb->DrawEntity(entity, sortedDraws, entity.id == hoveredEntityId);
    }
}
void ClientWorld::DrawHoveredObjectIndicator(GameClient &client, Tilemap &map)
{
    // Object interact indicator (cursor ornament in screen space)
    if (client.controller.tile_hovered) {
        uint16_t object_id = 0;
        map.AtTry(TILE_LAYER_OBJECT, client.controller.tile_x, client.controller.tile_y, object_id);
        if (object_id) {
            const GfxFrame &gfx_frame = GetTileGfxFrame(object_id);
            const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);
            const Rectangle texRect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };

            Vector2 texAspect{ 1.0f, 1.0f };
            if (texRect.width > texRect.height) {
                texAspect.y = texRect.height / texRect.width;
            } else {
                texAspect.x = texRect.width / texRect.height;
            }

            const float ornamentScale = 0.7f + 0.05f * cosf(client.now * 5.0f);
            const Vector2 ornamentPos = Vector2Add(GetMousePosition(), { 4, 4 });
            const Rectangle ornamentRect{
                ornamentPos.x,
                ornamentPos.y,
                texAspect.x * TILE_W * ornamentScale,
                texAspect.y * TILE_W * ornamentScale
            };
            dlb_DrawTexturePro(gfx_file.texture, texRect, ornamentRect, {}, WHITE);
        }
    }
}
void ClientWorld::DrawDialog(GameClient &client, Entity &entity, Vector2 bottomCenterScreen, std::vector<FancyTextTip> &tips)
{
    Font &font = fntMedium;

    bottomCenterScreen.x = floorf(bottomCenterScreen.x);
    bottomCenterScreen.y = floorf(bottomCenterScreen.y);

    const float marginBottom = 4.0f;
    const Vector2 bgPad{ 12, 8 };

    const Vector2 titleSize = dlb_MeasureTextEx(font, CSTRS(entity.dialog_title));

    DialogNodeList msgNodes{};
    char *msgBuf = (char *)entity.dialog_message.c_str();
    Err err = ParseMessage(msgBuf, msgNodes);
    if (err) {
        DialogNode errNode{};
        errNode.type = DIALOG_NODE_TEXT;
        errNode.text = "<???>";
        msgNodes.push_back(errNode);
    }

    Vector2 msgCursor{};
    Vector2 msgSize{};
    for (DialogNode &node : msgNodes) {
        Vector2 nodeSize = dlb_MeasureTextEx(font, CSTRS(node.text), &msgCursor);
        msgSize.x = MAX(msgSize.x, nodeSize.x);
        msgSize.y = msgCursor.y + MIN(nodeSize.y, font.baseSize);
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
    RectConstrainToScreen(bgRect, &constrainedOffset);
    titlePos = Vector2Add(titlePos, constrainedOffset);
    msgPos = Vector2Add(msgPos, constrainedOffset);

    //msgBgRect.x = floorf(msgBgRect.x);
    //msgBgRect.y = floorf(msgBgRect.y);
    //msgBgRect.width = floorf(msgBgRect.width);
    //msgBgRect.height = floorf(msgBgRect.height);

    if (dlb_CheckCollisionPointRec(GetMousePosition(), bgRect)) {
        io.CaptureMouse();
    }

    //printf("%.02f %.02f %.02f %.02f\n", bgRect.x, bgRect.y, bgRect.width, bgRect.height);
    dlb_DrawNPatch(bgRect);
    //DrawRectangleRounded(msgBgRect, 0.2f, 6, Fade(BLACK, 0.5));
    //DrawRectangleRoundedLines(msgBgRect, 0.2f, 6, 1.0f, RAYWHITE);
    dlb_DrawTextEx(font, CSTRS(entity.dialog_title), titlePos, GOLD);

    float separatorY = titlePos.y + titleSize.y + bgPad.y / 2;
    DrawLine(
        bgRect.x + bgPad.x - 2,
        separatorY,
        bgRect.x + bgRect.width - bgPad.x + 2,
        separatorY + 1,
        DIALOG_SEPARATOR_GOLD
    );

    Vector2 nodeCursor{};
    uint16_t nodeIdx = 0;
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
void ClientWorld::DrawDialogTips(std::vector<FancyTextTip> tips)
{
    // Render tooltips last
    for (const FancyTextTip &tip : tips) {
        const char *tipText = TextFormat("%.*s", tip.tipLen, tip.tip);
        Vector2 tipPos = Vector2Add(GetMousePosition(), { 0, 20 });
        tipPos.x = floorf(tipPos.x);
        tipPos.y = floorf(tipPos.y);
        Vector2 tipSize = dlb_MeasureTextEx(*tip.font, CSTRLEN(tipText));

        Rectangle tipRec{
            tipPos.x, tipPos.y,
            tipSize.x, tipSize.y
        };
        tipRec = RectGrow(tipRec, 8);

        Vector2 constrainedOffset{};
        RectConstrainToScreen(tipRec, &constrainedOffset);
        tipPos = Vector2Add(tipPos, constrainedOffset);

        dlb_DrawNPatch(tipRec);
        dlb_DrawTextEx(*tip.font, tip.tip, tip.tipLen, tipPos, RAYWHITE);
    }
}
void ClientWorld::DrawDialogs(GameClient &client)
{
    IO::Scoped scope(IO::IO_GameNPCDialog);

    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    std::vector<FancyTextTip> tips{};

    for (Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawned_at || entity.map_id != map->id) {
            continue;
        }
        assert(entity.id);

        if (entity.dialog_spawned_at) {
            const Vector2 topCenter = entity.TopCenter();
            const Vector2 topCenterScreen = GetWorldToScreen2D(topCenter, camera);
            DrawDialog(client, entity, topCenterScreen, tips);
        }
    }

    DrawDialogTips(tips);
}
void ClientWorld::DrawHUDEntityHoverInfo(void)
{
    if (!hoveredEntityId) {
        return;
    }

    IO::Scoped scope(IO::IO_GameNPC);  // ??? confusing
    if (io.MouseCaptured()) {
        return;
    }

    Entity *entity = entityDb->FindEntity(hoveredEntityId);
    if (!entity) {
        // We were probably hovering an entity while it was despawning?
        assert(!"huh? how can we hover an entity that doesn't exist");
        hoveredEntityId = 0;
        return;
    }

    Entity &e_hovered = *entity;
    if (!e_hovered.hp_max) {
        return;
    }

    const float borderWidth = 1;
    const float pad = 1;
    Vector2 hpBarPad{ pad, pad };
    Vector2 hpBarSize{ 200, 24 };

    Rectangle hpBarBg{
        (float)GetRenderWidth() / 2 - hpBarSize.x / 2 - hpBarPad.x,
        20.0f,
        hpBarSize.x + hpBarPad.x * 2,
        hpBarSize.y + hpBarPad.y * 2
    };

    Rectangle hpBar{
        hpBarBg.x + hpBarPad.x,
        hpBarBg.y + hpBarPad.y,
        hpBarSize.x,
        hpBarSize.y
    };

    DrawRectangleRec(hpBarBg, Fade(BLACK, 0.5));

    if (fabsf(e_hovered.hp - e_hovered.hp_smooth) < 1.0f) {
        float pctHealth = CLAMP((float)e_hovered.hp_smooth / e_hovered.hp_max, 0, 1);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctHealth), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    } else {
        float pctHealth = CLAMP((float)e_hovered.hp / e_hovered.hp_max, 0, 1);
        float pctHealthSmooth = CLAMP((float)e_hovered.hp_smooth / e_hovered.hp_max, 0, 1);
        float pctWhite = MAX(pctHealth, pctHealthSmooth);
        float pctRed = MIN(pctHealth, pctHealthSmooth);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctWhite), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(WHITE, -0.3));
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctRed), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    }

    Vector2 labelSize = dlb_MeasureTextShadowEx(fntMedium, CSTRS(e_hovered.name));
    Vector2 labelPos{
        floorf(hpBarBg.x + hpBarBg.width / 2 - labelSize.x / 2),
        floorf(hpBarBg.y + hpBarBg.height / 2 - labelSize.y / 2)
    };
    dlb_DrawTextShadowEx(fntMedium, e_hovered.name.c_str(), e_hovered.name.size(), labelPos, WHITE);
}
void ClientWorld::DrawHUDSignEditor(void)
{
    IO::Scoped scope(IO::IO_HUDSignEditor);

    static bool editingSign = false;

    if (!editingSign) {
        if (io.KeyPressed(KEY_ONE)) {
            editingSign = true;
        }
    } else {
        io.CaptureKeyboard();
        io.CaptureMouse();

        Entity *player = LocalPlayer();
        assert(player);

        Vector2 playerTopWorld = player->TopCenter();
        Vector2 playerTopScreen = GetWorldToScreen2D(playerTopWorld, camera);

        Vector2 uiSignEditorSize{ 200, 100 };
        Vector2 uiSignEditorPos{
            playerTopScreen.x - uiSignEditorSize.x / 2.0f,
            playerTopScreen.y - uiSignEditorSize.y
        };
        uiSignEditorPos.x = floorf(uiSignEditorPos.x);
        uiSignEditorPos.y = floorf(uiSignEditorPos.y);

        UIStyle uiSignEditorStyle{};
        uiSignEditorStyle.borderColor = BLANK;
        uiSignEditorStyle.size.x = uiSignEditorSize.x;
        UI uiSignEditor{ uiSignEditorPos, uiSignEditorSize, uiSignEditorStyle };

        const Rectangle uiSignedEditorRect{
            uiSignEditorPos.x,
            uiSignEditorPos.y,
            uiSignEditorSize.x,
            uiSignEditorSize.y
        };
        DrawRectangleRec(uiSignedEditorRect, ColorBrightness(BROWN, -0.1f));
        DrawRectangleLinesEx(uiSignedEditorRect, 2.0f, BLACK);

        UIState uiState{};
        uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), uiSignedEditorRect);

        static std::string signText{};

        UIPad margin = uiSignEditorStyle.margin;
        //margin.left -= 4;
        //margin.bottom -= 4;
        uiSignEditor.PushMargin(margin);
        uiSignEditor.PushWidth(uiSignEditorSize.x); // - (uiSignEditorStyle.margin.left + uiSignEditorStyle.margin.right));
        uiSignEditor.PushBgColor(Fade(PINK, 0.2f), UI_CtrlTypeTextbox);
        //uiSignEditor.PushBgColor(BLANK, UI_CtrlTypeTextbox);
        uiSignEditor.Textbox(__COUNTER__, signText, false, 0, LimitStringLength, (void *)128);
        uiSignEditor.Newline();
        uiSignEditor.PopStyle();
        uiSignEditor.PopStyle();
        uiSignEditor.PopStyle();

        if (io.KeyPressed(KEY_ESCAPE)) {
            editingSign = false;
        }
    }
}
void ClientWorld::DrawHUDMenu(void)
{
    IO::Scoped scope(IO::IO_HUDMenu);

    UIStyle uiHUDMenuStyle{};
    static Vector2 uiPosition{};
    static Vector2 uiSize{};
    UI uiHUDMenu{ uiPosition, uiSize, uiHUDMenuStyle };

    // TODO: Add Quit button to the menu at the very least
    uiHUDMenu.Button("Menu");
    uiHUDMenu.Button("Quests");
    uiHUDMenu.Button("Inventory");
    uiHUDMenu.Button("Map");
    uiHUDMenu.Newline();

    const char *holdingItem = spinner.ItemName();
    if (!holdingItem) {
        holdingItem = "-Empty-";
    }
    uiHUDMenu.Text(holdingItem);
    uiHUDMenu.Newline();

    Tilemap *map = LocalPlayerMap();
    if (map) {
        uiHUDMenu.Text(map->bg_music);
    } else {
        uiHUDMenu.Text("-Empty-");
    }
    uiHUDMenu.Newline();
}
void ClientWorld::Draw(GameClient &client)
{
    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    BeginMode2D(camera);

    DrawCmdQueue sortedDraws{};
    map->Draw(camera, sortedDraws);
    DrawHoveredTileIndicator(client);
    DrawEntities(client, *map, sortedDraws);

#if CL_DBG_PIXEL_FIXER
    Vector2 screenSize{
        floorf((float)GetRenderWidth()),
        floorf((float)GetRenderHeight())
    };
    SetShaderValue(shdPixelFixer, shdPixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
    BeginShaderMode(shdPixelFixer);
#endif
    sortedDraws.Draw();
#if CL_DBG_PIXEL_FIXER
    EndShaderMode();
#endif

    EndMode2D();

    DrawHoveredObjectIndicator(client, *map);
    DrawDialogs(client);
    DrawHUDEntityHoverInfo();
    spinner.Draw();
    title.Draw(client.now);
    DrawHUDSignEditor();
    DrawHUDMenu();

    ScreenFX_Fade(warpFadeInStartedAt, CL_WARP_FADE_IN_DURATION, client.now);
}