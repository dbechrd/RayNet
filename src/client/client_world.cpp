#include "client_world.h"
#include "game_client.h"
#include "../common/collision.h"
#include "../common/entity.h"
#include "../common/entity_db.h"
#include "../common/io.h"
#include "../common/net/messages/msg_s_entity_spawn.h"
#include "../common/net/messages/msg_s_entity_snapshot.h"

GhostSnapshot::GhostSnapshot(Msg_S_EntitySnapshot &msg)
{
    serverTime            = msg.serverTime;
    lastProcessedInputCmd = msg.lastProcessedInputCmd;

    // Entity
    mapId    = msg.mapId;
    position = msg.position;

    // Physics
    speed    = msg.speed;
    velocity = msg.velocity;

    // Life
    maxHealth = msg.maxHealth;
    health    = msg.health;
}

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
    if (localPlayer && localPlayer->mapId) {
        return FindOrLoadMap(localPlayer->mapId);
    }
    return 0;
}

Tilemap *ClientWorld::FindOrLoadMap(uint32_t mapId)
{
    assert(mapId);
    Tilemap *map = 0;

    const auto &mapEntry = mapsById.find(mapId);
    if (mapEntry != mapsById.end()) {
        map = maps[mapEntry->second];
    } else {
        std::string mapName = "unknownMapId";
        switch (mapId) {
            case 1: mapName = LEVEL_001; break;
            case 2: mapName = LEVEL_CAVE; break;
        }
        // TODO: LoadPack map and add to maps/mapdsById

        map = new Tilemap();
        if (map) {
            Err err = map->Load(mapName);
            if (!err) {
                map->id = mapId;
                mapsById[map->id] = maps.size();
                maps.push_back(map);
            } else {
                printf("Failed to load map id %u with code %d\n", mapId, err);
                delete map;
                map = 0;
            }
        } else {
            printf("Failed to allocate space to load map id %u\n", mapId);
        }
    }

    if (map) {
        switch (map->id) {
            case 1: musBackgroundMusic = "AMBIENT_OUTDOORS"; break;
            case 2: musBackgroundMusic = "AMBIENT_CAVE"; break;
            default: musBackgroundMusic = "NONE"; break;
        }
    }

    return map;
}

bool ClientWorld::CopyEntityData(uint32_t entityId, EntityData &data)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to copy entity. Entity id %u not found.\n", entityId);
        return false;
    }

    data.entity    = entityDb->entities[entityIndex];
    data.collision = entityDb->collision[entityIndex];
    data.dialog    = entityDb->dialog[entityIndex];
    data.ghosts    = entityDb->ghosts[entityIndex];
    data.life      = entityDb->life[entityIndex];
    data.pathfind  = entityDb->pathfind[entityIndex];
    data.physics   = entityDb->physics[entityIndex];
    data.sprite    = entityDb->sprite[entityIndex];
    return true;
}

void ClientWorld::ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn)
{
    size_t entityIndex = entityDb->FindEntityIndex(entitySpawn.entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to spawn entity. Entity id %u not found.\n", entitySpawn.entityId);
        return;
    }

    data::Entity          &entity     = entityDb->entities  [entityIndex];
    data::AspectCollision &eCollision = entityDb->collision [entityIndex];
    data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
    data::AspectLife      &eLife      = entityDb->life      [entityIndex];
    data::AspectSprite    &eSprite    = entityDb->sprite    [entityIndex];

    entity.type       = entitySpawn.type;
    entity.mapId      = entitySpawn.mapId;
    entity.position   = entitySpawn.position;
    eCollision.radius = entitySpawn.radius;
    ePhysics.drag     = entitySpawn.drag;
    ePhysics.speed    = entitySpawn.speed;
    ePhysics.velocity = entitySpawn.velocity;
    eLife.maxHealth   = entitySpawn.maxHealth;
    eLife.health      = entitySpawn.health;

    // TODO: Look it up from somewhere based on entity type?
    switch (entity.type) {
        case data::ENTITY_PLAYER: {
            eSprite.sprite = data::SPRITE_CHR_MAGE;
            //eSprite.direction = data::DIR_E;
            break;
        }
        case data::ENTITY_NPC: {
            eSprite.sprite = data::SPRITE_NPC_LILY;
            //eSprite.direction = data::DIR_E;
            break;
        }
        case data::ENTITY_PROJECTILE: {
            eSprite.sprite = data::SPRITE_PRJ_FIREBALL;
            //eSprite.direction = data::DIR_N;
            break;
        };
    }
}

void ClientWorld::ApplyStateInterpolated(EntityInterpolateTuple &data,
    const GhostSnapshot &a, const GhostSnapshot &b, float alpha, float dt)
{
    if (b.mapId != a.mapId) {
        alpha = 1.0f;
    }
    data.entity.mapId = b.mapId;
    data.entity.position.x = LERP(a.position.x, b.position.x, alpha);
    data.entity.position.y = LERP(a.position.y, b.position.y, alpha);

    data.physics.velocity.x = LERP(a.velocity.x, b.velocity.x, alpha);
    data.physics.velocity.y = LERP(a.velocity.y, b.velocity.y, alpha);

    data.life.maxHealth = b.maxHealth;
    data.life.health = b.health;
    data.life.healthSmooth = LERP(data.life.healthSmooth, data.life.health, 1.0f - powf(1.0f - 0.999f, dt));
    //data.life.healthSmooth += ((data.life.healthSmooth < data.life.health) ? 1 : -1) * dt * 20;
}

void ClientWorld::ApplyStateInterpolated(uint32_t entityId,
    const GhostSnapshot &a, const GhostSnapshot &b, float alpha, float dt)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to interpolate entity. Entity id %u not found.\n", entityId);
        return;
    }

    data::Entity        &entity  = entityDb->entities [entityIndex];
    data::AspectPhysics &physics = entityDb->physics  [entityIndex];
    data::AspectLife    &life    = entityDb->life     [entityIndex];
    data::AspectSprite  &sprite  = entityDb->sprite   [entityIndex];

    EntityInterpolateTuple data{ entity, physics, life, sprite };
    ApplyStateInterpolated(data, a, b, alpha, dt);
}

Err ClientWorld::CreateDialog(uint32_t entityId, std::string message, double now)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to create dialog. Could not find entity id %u.\n", entityId);
        return RN_OUT_OF_BOUNDS;
    }

    data::AspectDialog &dialog = entityDb->dialog[entityIndex];
    dialog.spawnedAt = now;
    dialog.title = "Lily";
    dialog.message = message;
    return RN_SUCCESS;
}

void ClientWorld::UpdateEntities(GameClient &client)
{
    hoveredEntityId = 0;
    Tilemap *localPlayerMap = LocalPlayerMap();

    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type) {
            continue;
        }

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        data::AspectPhysics &ePhysics = entityDb->physics[entityIndex];
        AspectGhost &eGhost = entityDb->ghosts[entityIndex];
        data::AspectSprite &eSprite = entityDb->sprite[entityIndex];
        data::AspectDialog &eDialog = entityDb->dialog[entityIndex];

        // Local player
        if (entity.id == localPlayerEntityId) {
            uint32_t latestSnapInputSeq = 0;

            // Apply latest snapshot
            const GhostSnapshot &latestSnapshot = eGhost.newest();
            if (latestSnapshot.serverTime) {
                ApplyStateInterpolated(entity.id, eGhost.newest(), eGhost.newest(), 0, client.frameDt);
                latestSnapInputSeq = latestSnapshot.lastProcessedInputCmd;
            }

#if CL_CLIENT_SIDE_PREDICT
            double cmdAccumDt{};
            Vector2 cmdAccumForce{};
            Tilemap *map = FindOrLoadMap(entity.mapId);
            if (map) {
                // Apply unacked input
                for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
                    InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
                    if (inputCmd.seq > latestSnapInputSeq) {
                        ePhysics.ApplyForce(inputCmd.GenerateMoveForce(ePhysics.speed));
                        entityDb->EntityTick(entity.id, SV_TICK_DT);
                        map->ResolveEntityTerrainCollisions(entity.id);
                    }
                }

                cmdAccumDt = client.now - client.controller.lastInputSampleAt;
                if (cmdAccumDt > 0) {
                    Vector2 posBefore = entity.position;
                    cmdAccumForce = client.controller.cmdAccum.GenerateMoveForce(ePhysics.speed);
                    ePhysics.ApplyForce(cmdAccumForce);
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

            if (!Histogram::paused) {
                static Color colors[] = { BEIGE, BROWN };
                static int colorIdx = 0;
                if (latestSnapInputSeq != histoInput.buffer[histoInput.buffer.size() - 2].value) {
                    colorIdx = ((size_t)colorIdx + 1) % ARRAY_SIZE(colors);
                }
                Histogram::Entry &entryInput = histoInput.buffer.newest();
                entryInput.value = latestSnapInputSeq;
                entryInput.color = colors[colorIdx];
                entryInput.metadata = TextFormat("latestSnapInputSeq: %u", latestSnapInputSeq);

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
                    ePhysics.velocity.x,
                    cmdAccumDt > 0 ? cmdAccumForce.x : 0,
                    cmdAccumDt > 0 ? cmdAccumForce.y : 0,
                    client.ServerNow(),
                    cmdAccumDt
                );
            }
        } else {
            // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
            const double renderAt = client.ServerNow() - SV_TICK_DT;

            size_t snapshotBIdx = 0;
            while (snapshotBIdx < eGhost.size()
                && eGhost[snapshotBIdx].serverTime <= renderAt)
            {
                snapshotBIdx++;
            }

            const GhostSnapshot *snapshotA = 0;
            const GhostSnapshot *snapshotB = 0;

            if (snapshotBIdx <= 0) {
                snapshotA = &eGhost.oldest();
                snapshotB = &eGhost.oldest();
            } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
                snapshotA = &eGhost.newest();
                snapshotB = &eGhost.newest();
            } else {
                snapshotA = &eGhost[snapshotBIdx - 1];
                snapshotB = &eGhost[snapshotBIdx];
            }

            float alpha = 0;
            if (snapshotB != snapshotA) {
                alpha = (renderAt - snapshotA->serverTime) /
                    (snapshotB->serverTime - snapshotA->serverTime);
            }

            ApplyStateInterpolated(entity.id, *snapshotA, *snapshotB, alpha, client.frameDt);

            if (localPlayerMap && entity.mapId == localPlayerMap->id) {
                const Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);
                bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, entityDb->EntityRect(entity.id));
                if (hover) {
                    bool down = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
                    if (down) {
                        client.SendEntityInteract(entity.id);
                    }
                    hoveredEntityId = entity.id;
                }
            }
        }

        bool newlySpawned = entity.spawnedAt == client.now;
        data::UpdateSprite(eSprite, entity.type, ePhysics.velocity, client.frameDt, newlySpawned);

        const double duration = CL_DIALOG_DURATION_MIN + CL_DIALOG_DURATION_PER_CHAR * eDialog.message.size();
        if (eDialog.spawnedAt && client.now - eDialog.spawnedAt > duration) {
            eDialog = {};
        }
    }
}

void ClientWorld::Update(GameClient &gameClient)
{
    UpdateEntities(gameClient);
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
        AspectGhost &ghost = entityDb->ghosts[entityIndex];

        EntityData ghostData{};
        CopyEntityData(entity.id, ghostData);
        // Prevents accidentally overwriting real entities if someone we pass a tuple
        // to decides to look up the entity by id instead of using the tuple's data.
        ghostData.entity.id = 0;

        EntityInterpolateTuple ghostInterpData{ ghostData.entity, ghostData.physics, ghostData.life, ghostData.sprite };
        EntitySpriteTuple ghostSpriteData{ ghostData.entity, ghostData.sprite };
        EntityTickTuple ghostTickData{ ghostData.entity, ghostData.life, ghostData.physics, ghostData.sprite };
        EntityCollisionTuple ghostCollisionData{ ghostData.entity, ghostData.collision, ghostData.physics };

        for (int i = 0; i < ghost.size(); i++) {
            if (!ghost[i].serverTime) {
                continue;
            }
            ApplyStateInterpolated(ghostInterpData, ghost[i], ghost[i], 0, dt);

            const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
            Rectangle ghostRect = entityDb->EntityRect(ghostSpriteData);
            ghostRect = RectShrink(ghostRect, scalePer);
            DrawRectangleRec(ghostRect, Fade(RED, 0.1f));
            DrawRectangleLinesEx(ghostRect, 1, Fade(RED, 0.8f));
        }

        // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
        if (entity.id == localPlayerEntityId) {
#if CL_CLIENT_SIDE_PREDICT
            Tilemap *map = FindOrLoadMap(entity.mapId);
            if (map) {
                uint32_t lastProcessedInputCmd = 0;
                const GhostSnapshot &latestSnapshot = ghost.newest();
                if (latestSnapshot.serverTime) {
                    ApplyStateInterpolated(ghostInterpData, latestSnapshot, latestSnapshot, 0, dt);
                    lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
                }

                for (size_t cmdIndex = 0; cmdIndex < controller.cmdQueue.size(); cmdIndex++) {
                    InputCmd &inputCmd = controller.cmdQueue[cmdIndex];
                    if (inputCmd.seq > lastProcessedInputCmd) {
                        ghostData.physics.ApplyForce(inputCmd.GenerateMoveForce(ghostData.physics.speed));
                        entityDb->EntityTick(ghostTickData, SV_TICK_DT);
                        map->ResolveEntityTerrainCollisions(ghostCollisionData);
                        Rectangle ghostRect = entityDb->EntityRect(ghostSpriteData);
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
                Vector2 cmdAccumForce = controller.cmdAccum.GenerateMoveForce(ghostData.physics.speed);
                ghostData.physics.ApplyForce(cmdAccumForce);
                entityDb->EntityTick(ghostTickData, SV_TICK_DT);
                map->ResolveEntityTerrainCollisions(ghostCollisionData);
                ghostData.entity.position.y = LERP(posBefore.y, ghostData.entity.position.y, cmdAccumDt / SV_TICK_DT);
                ghostData.entity.position.x = LERP(posBefore.x, ghostData.entity.position.x, cmdAccumDt / SV_TICK_DT);
            }
            Rectangle ghostRect = entityDb->EntityRect(ghostSpriteData);
            ghostRect.x = floorf(ghostRect.x);
            ghostRect.y = floorf(ghostRect.y);
            DrawRectangleLinesEx(ghostRect, 1, Fade(BLUE, 0.8f));
#endif
#endif
        }
    }
}

// TODO(dlb): Where should this live? Probably not in ClientWorld?
void ClientWorld::DrawDialog(data::AspectDialog &dialog, Vector2 bottomCenterScreen)
{
    Font &font = fntMedium;

    bottomCenterScreen.x = roundf(bottomCenterScreen.x);
    bottomCenterScreen.y = roundf(bottomCenterScreen.y);

    const float marginBottom = 4.0f;
    const Vector2 bgPad{ 12, 8 };

    //size_t nodeCount = dlb_FancyTextParse(dialog.message.c_str(), dialog.message.size());

    FancyTextTree tree{};
    if (!dlb_FancyTextParse(tree, dialog.message.c_str())) {
        return;
    }

    const Vector2 titleSize = MeasureTextEx(font, dialog.title.c_str(), font.baseSize, 1);

    Vector2 msgOffset{};
    Vector2 msgSize{};
    for (FancyTextNode node : tree.nodes) {
        Vector2 nodeSize = dlb_MeasureFancyTextNode(node, font, msgOffset);
        msgSize.x = MAX(msgSize.x, nodeSize.x);
        msgSize.y = msgOffset.y + nodeSize.y;
    }

    const float bgHeight =
        bgPad.y +
        titleSize.y +
        msgSize.y +
        bgPad.y;

    const float bgTop = bottomCenterScreen.y - marginBottom - bgHeight;
    float cursorY = bgTop + bgPad.y;

    Vector2 titlePos{
        roundf(bottomCenterScreen.x - titleSize.x / 2),
        cursorY
    };
    cursorY += titleSize.y;

    Vector2 msgPos{
        roundf(bottomCenterScreen.x - msgSize.x / 2),
        cursorY
    };
    //msgPos.x = floorf(msgPos.x);
    //msgPos.y = floorf(msgPos.y);

    Rectangle bgRect{
        MIN(titlePos.x, msgPos.x) - bgPad.x,
        bgTop,
        MAX(titleSize.x, msgSize.x) + bgPad.x * 2,
        bgPad.y + titleSize.y + msgSize.y + bgPad.y
    };
    //msgBgRect.x = floorf(msgBgRect.x);
    //msgBgRect.y = floorf(msgBgRect.y);
    //msgBgRect.width = floorf(msgBgRect.width);
    //msgBgRect.height = floorf(msgBgRect.height);

    NPatchInfo nPatch{};
    Texture &nPatchTex = data::pack1.gfxFiles[data::GFX_FILE_DLG_NPATCH].texture;
    nPatch.source = { 0, 0, (float)nPatchTex.width, (float)nPatchTex.height };
    nPatch.left = 16;
    nPatch.top = 16;
    nPatch.right = 16;
    nPatch.bottom = 16;
    nPatch.layout = NPATCH_NINE_PATCH;
    DrawTextureNPatch(nPatchTex, nPatch, bgRect, {}, 0, WHITE);
    //DrawRectangleRounded(msgBgRect, 0.2f, 6, Fade(BLACK, 0.5));
    //DrawRectangleRoundedLines(msgBgRect, 0.2f, 6, 1.0f, RAYWHITE);
    dlb_DrawTextEx(font, dialog.title.c_str(), dialog.title.size(), titlePos, font.baseSize, 1.0f, GOLD);

    Vector2 cursor{};
    for (int i = 0; i < tree.nodes.size(); i++) {
        FancyTextNode &node = tree.nodes[i];
        Color col = RAYWHITE;
        switch (node.type) {
            case FancyTextNode::TEXT:          col = RAYWHITE; break;
            case FancyTextNode::DIALOG_OPTION: col = SKYBLUE;  break;
            case FancyTextNode::HOVER_TIP:     col = YELLOW;   break;
        }
        col = ColorFromHSV((float)i / tree.nodes.size() * 360.0f, 0.8f, 0.8f);
        bool hovered = false;
        dlb_DrawTextEx(font, node.text, node.textLen, msgPos, font.baseSize, 1.0f, col, &cursor, &hovered);

        const bool pressed = hovered && io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (pressed) {
            switch (node.type) {
                case FancyTextNode::DIALOG_OPTION: {
                    printf("Player clicked dialog option %d\n", node.optionId);
                    break;
                }
                case FancyTextNode::HOVER_TIP: {
                    break;
                }
            }
        }
    }
}

void ClientWorld::DrawDialogs(Camera2D &camera)
{
    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawnedAt || entity.mapId != map->id) {
            continue;
        }
        assert(entity.id);

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        data::AspectDialog &dialog = entityDb->dialog[entityIndex];
        if (dialog.spawnedAt) {
            const Vector2 topCenter = entityDb->EntityTopCenter(entity.id);
            const Vector2 topCenterScreen = GetWorldToScreen2D(topCenter, camera);
            DrawDialog(dialog, topCenterScreen);
        }
    }
}

void ClientWorld::Draw(Controller &controller, double now, double dt)
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
    for (data::Entity &entity : entityDb->entities) {
        if (!entity.type || entity.despawnedAt || entity.mapId != map->id) {
            continue;
        }
        assert(entity.id);
        DrawEntitySnapshotShadows(entity.id, controller, now, dt);
        entityDb->DrawEntity(entity.id);
    }
    EndMode2D();

    //--------------------
    // Draw the dialogs
    DrawDialogs(camera2d);

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