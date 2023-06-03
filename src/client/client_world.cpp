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

Entity *ClientWorld::LocalPlayer(void) {
    Entity *localPlayer = entityDb->FindEntity(localPlayerEntityId);
    if (localPlayer) {
        assert(localPlayer->type == Entity_Player);
        if (localPlayer->type == Entity_Player) {
            return localPlayer;
        }
    }
    return 0;
}

Tilemap *ClientWorld::LocalPlayerMap(void)
{
    Entity *localPlayer = LocalPlayer();
    if (localPlayer) {
        return FindOrLoadMap(localPlayer->mapId);
    }
    return 0;
}

Tilemap *ClientWorld::FindOrLoadMap(uint32_t mapId)
{
    assert(mapId);

    const auto &mapEntry = mapsById.find(mapId);
    if (mapEntry != mapsById.end()) {
        return maps[mapEntry->second];
    } else {
        std::string mapName = "unknownMapId";
        switch (mapId) {
            case 1: mapName = LEVEL_001; break;
            case 2: mapName = LEVEL_CAVE; break;
        }
        // TODO: Load map and add to maps/mapdsById

        Tilemap *map = new Tilemap();
        if (map) {
            Err err = map->Load(mapName);
            if (!err) {
                map->id = mapId;
                mapsById[map->id] = maps.size();
                maps.push_back(map);
                return map;
            } else {
                printf("Failed to load map id %u with code %d\n", mapId, err);
                delete map;
            }
        } else {
            printf("Failed to allocate space to load map id %u\n", mapId);
        }
    }
    return 0;
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

    Entity          &entity    = entityDb->entities[entityIndex];
    AspectCollision &collision = entityDb->collision[entityIndex];
    AspectPhysics   &physics   = entityDb->physics[entityIndex];
    AspectLife      &life      = entityDb->life[entityIndex];
    data::Sprite    &sprite    = entityDb->sprite[entityIndex];

    entity.type      = entitySpawn.type;
    entity.mapId     = entitySpawn.mapId;
    entity.position  = entitySpawn.position;
    collision.radius = entitySpawn.radius;
    physics.drag     = entitySpawn.drag;
    physics.speed    = entitySpawn.speed;
    physics.velocity = entitySpawn.velocity;
    life.maxHealth   = entitySpawn.maxHealth;
    life.health      = entitySpawn.health;

    // TODO: Look it up from somewhere based on entity type?
    switch (entity.type) {
        case Entity_NPC: {
            sprite.anims[0] = data::GFX_ANIM_NPC_LILY_N;
            break;
        }
        case Entity_Player: {
            sprite.anims[0] = data::GFX_ANIM_CHR_MAGE_N;
            break;
        }
        case Entity_Projectile: {
            sprite.anims[0] = data::GFX_ANIM_PRJ_BULLET;
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

    Entity        &entity  = entityDb->entities[entityIndex];
    AspectPhysics &physics = entityDb->physics[entityIndex];
    AspectLife    &life    = entityDb->life[entityIndex];

    EntityInterpolateTuple data{entity, physics, life};
    ApplyStateInterpolated(data, a, b, alpha, dt);
}

Err ClientWorld::CreateDialog(uint32_t entityId, std::string message, double now)
{
    size_t entityIndex = entityDb->FindEntityIndex(entityId);
    if (!entityIndex) {
        printf("[client_world] Failed to create dialog. Could not find entity id %u.\n", entityId);
        return RN_OUT_OF_BOUNDS;
    }

    AspectDialog &dialog = entityDb->dialog[entityIndex];
    dialog.spawnedAt = now;
    dialog.message = message;
    return RN_SUCCESS;
}

void ClientWorld::UpdateEntities(GameClient &client)
{
    hoveredEntityId = 0;
    Tilemap *localPlayerMap = LocalPlayerMap();

    for (Entity &entity : entityDb->entities) {
        if (entity.type == Entity_None) {
            continue;
        }

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        AspectPhysics &physics = entityDb->physics[entityIndex];
        AspectGhost &ghost = entityDb->ghosts[entityIndex];
        AspectDialog &dialog = entityDb->dialog[entityIndex];

        // Local player
        if (entity.id == localPlayerEntityId) {
            uint32_t lastProcessedInputCmd = 0;

            // Apply latest snapshot
            const GhostSnapshot &latestSnapshot = ghost.newest();
            if (latestSnapshot.serverTime) {
                ApplyStateInterpolated(entity.id, ghost.newest(), ghost.newest(), 0, client.frameDt);
                lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
            }

#if CL_CLIENT_SIDE_PREDICT
            // TODO: This probably should just be FindMap and shouldn't force
            // every map to load and start ticking? I'm not sure..
            Tilemap *map = FindOrLoadMap(entity.mapId);
            if (map) {
                // Apply unacked input
                for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
                    InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
                    if (inputCmd.seq > lastProcessedInputCmd) {
                        physics.ApplyForce(inputCmd.GenerateMoveForce(physics.speed));
                        entityDb->EntityTick(entity.id, SV_TICK_DT, client.now);
                        map->ResolveEntityTerrainCollisions(entity.id);
                    }
                }

                const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
                if (cmdAccumDt > 0) {
                    physics.ApplyForce(client.controller.cmdAccum.GenerateMoveForce(physics.speed));
                    entityDb->EntityTick(entity.id, cmdAccumDt, client.now);
                    map->ResolveEntityTerrainCollisions(entity.id);
                }
            }
#endif

            // Check for ignored input packets
            uint32_t oldestInput = client.controller.cmdQueue.oldest().seq;
            if (oldestInput > lastProcessedInputCmd + 1) {
                printf(" localPlayer: %d inputs dropped\n", oldestInput - lastProcessedInputCmd - 1);
            }
        } else {
            // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
            const double renderAt = client.ServerNow() - SV_TICK_DT;

            size_t snapshotBIdx = 0;
            while (snapshotBIdx < ghost.size()
                && ghost[snapshotBIdx].serverTime <= renderAt)
            {
                snapshotBIdx++;
            }

            const GhostSnapshot *snapshotA = 0;
            const GhostSnapshot *snapshotB = 0;

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

        const double duration = CL_DIALOG_DURATION_MIN + CL_DIALOG_DURATION_PER_CHAR * dialog.message.size();
        if (dialog.spawnedAt && client.now - dialog.spawnedAt > duration) {
            dialog = {};
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

        Entity &entity = entityDb->entities[entityIndex];
        AspectGhost &ghost = entityDb->ghosts[entityIndex];

        EntityData ghostData{};
        CopyEntityData(entity.id, ghostData);
        // Prevents accidentally overwriting real entities if someone we pass a tuple
        // to decides to look up the entity by id instead of using the tuple's data.
        ghostData.entity.id = 0;

        EntityInterpolateTuple ghostInterpData{ ghostData.entity, ghostData.physics, ghostData.life };
        EntitySpriteTuple ghostSpriteData{ ghostData.entity, ghostData.sprite };
        EntityTickTuple ghostTickData{ ghostData.entity, ghostData.life, ghostData.physics };
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

                //const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
                for (size_t cmdIndex = 0; cmdIndex < controller.cmdQueue.size(); cmdIndex++) {
                    InputCmd &inputCmd = controller.cmdQueue[cmdIndex];
                    if (inputCmd.seq > lastProcessedInputCmd) {
                        ghostData.physics.ApplyForce(inputCmd.GenerateMoveForce(ghostData.physics.speed));
                        entityDb->EntityTick(ghostTickData, SV_TICK_DT, now);
                        map->ResolveEntityTerrainCollisions(ghostCollisionData);
                        Rectangle ghostRect = entityDb->EntityRect(ghostSpriteData);
                        DrawRectangleRec(ghostRect, Fade(GREEN, 0.1f));
                        DrawRectangleLinesEx(ghostRect, 1, Fade(GREEN, 0.8f));
                    }
                }
            }
#endif

#if 0
            // We don't really need to draw a blue rect at currently entity position unless
            // the entity is invisible.
            const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
            if (cmdAccumDt > 0) {
                ghostInstance.ApplyForce(client.controller.cmdAccum.GenerateMoveForce(ghostInstance.speed));
                ghostInstance.Tick(cmdAccumDt);
                map->ResolveEntityTerrainCollisions(ghostInstance);
                DrawRectangleRec(ghostInstance.GetRect(), Fade(BLUE, 0.2f));
                printf("%.3f\n", cmdAccumDt);
            }
#endif
        }
    }
}

// TODO(dlb): Where should this live? Probably not in ClientWorld?
void ClientWorld::DrawDialog(AspectDialog &dialog, Vector2 topCenterScreen)
{
    const float marginBottom = 4.0f;
    Vector2 bgPad{ 12, 8 };
    Vector2 msgSize = MeasureTextEx(fntHackBold20, dialog.message.c_str(), fntHackBold20.baseSize, 1.0f);
    Vector2 msgPos{
        topCenterScreen.x - msgSize.x / 2,
        topCenterScreen.y - msgSize.y - bgPad.y * 2 - marginBottom
    };
    //msgPos.x = floorf(msgPos.x);
    //msgPos.y = floorf(msgPos.y);

    Rectangle msgBgRect{
        msgPos.x - bgPad.x,
        msgPos.y - bgPad.y,
        msgSize.x + bgPad.x * 2,
        msgSize.y + bgPad.y * 2
    };
    //msgBgRect.x = floorf(msgBgRect.x);
    //msgBgRect.y = floorf(msgBgRect.y);
    //msgBgRect.width = floorf(msgBgRect.width);
    //msgBgRect.height = floorf(msgBgRect.height);

    NPatchInfo nPatch{};
    nPatch.source = { 0, 0, (float)texNPatch.width, (float)texNPatch.height };
    nPatch.left = 16;
    nPatch.top = 16;
    nPatch.right = 16;
    nPatch.bottom = 16;
    nPatch.layout = NPATCH_NINE_PATCH;
    DrawTextureNPatch(texNPatch, nPatch, msgBgRect, {}, 0, WHITE);
    //DrawRectangleRounded(msgBgRect, 0.2f, 6, Fade(BLACK, 0.5));
    //DrawRectangleRoundedLines(msgBgRect, 0.2f, 6, 1.0f, RAYWHITE);
    DrawTextEx(fntHackBold20, dialog.message.c_str(), msgPos, fntHackBold20.baseSize, 1.0f, RAYWHITE);
    //DrawTextShadowEx(fntHackBold20, dialog.message, msgPos, FONT_SIZE, RAYWHITE);
}

// TODO: Entity should just draw it's own damn dialog
void ClientWorld::DrawDialogs(Camera2D &camera)
{
    for (Entity &entity : entityDb->entities) {
        if (!entity.type) {
            continue;
        }
        assert(entity.id);

        size_t entityIndex = entityDb->FindEntityIndex(entity.id);
        AspectDialog &dialog = entityDb->dialog[entityIndex];
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
    for (Entity &entity : entityDb->entities) {
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
        Entity *entity = entityDb->FindEntity(hoveredEntityId);
        if (entity) {
            entityDb->DrawEntityHoverInfo(hoveredEntityId);
        } else {
            // We were probably hovering an entity while it was despawning?
            assert(!"huh? how can we hover an entity that doesn't exist");
            hoveredEntityId = 0;
        }
    }
}