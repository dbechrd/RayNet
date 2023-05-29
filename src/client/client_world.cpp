#include "client_world.h"
#include "game_client.h"
#include "../common/entity.h"
#include "../common/net/messages/msg_s_entity_spawn.h"
#include "../common/net/messages/msg_s_entity_snapshot.h"

GhostSnapshot::GhostSnapshot(Msg_S_EntitySnapshot &msg)
{
    serverTime            = msg.serverTime;
    lastProcessedInputCmd = msg.lastProcessedInputCmd;

    // Entity
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
    Entity *localPlayer = FindEntity(localPlayerEntityId);
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
    Tilemap *localPlayerMap = FindEntityMap(localPlayerEntityId);
    return localPlayerMap;
}

Tilemap *ClientWorld::FindOrLoadMap(uint32_t mapId)
{
    const auto &mapEntry = mapsById.find(mapId);
    if (mapEntry != mapsById.end()) {
        return maps[mapEntry->second];
    } else {
        std::string mapName = "unknownMapId";
        switch (mapId) {
            case 1: mapName = LEVEL_001; break;
            case 2: mapName = LEVEL_002; break;
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
Tilemap *ClientWorld::FindEntityMap(uint32_t entityId)
{
    assert(entityId < SV_MAX_ENTITIES);

    const auto &mapIdEntry = entityMapId.find(entityId);
    if (mapIdEntry != entityMapId.end()) {
        size_t mapId = mapIdEntry->second;
        return FindOrLoadMap(mapId);
    }
    return 0;
}
Entity *ClientWorld::FindEntity(uint32_t entityId, bool deadOrAlive)
{
    assert(entityId < SV_MAX_ENTITIES);

    Tilemap *map = FindEntityMap(entityId);
    if (map) {
        Entity *entity = map->FindEntity(entityId);
        assert(entity->type);
        if (entity && entity->type && (deadOrAlive || !entity->despawnedAt)) {
            assert(entity->id == entityId);
            return entity;
        }
    }
    return {};
}

bool ClientWorld::CopyEntityData(uint32_t entityId, EntityData &data)
{
    if (entityId >= SV_MAX_ENTITIES) {
        printf("[client_world] Failed to spawn entity. Entity id %u out of range.\n", entityId);
        return false;
    }

    Tilemap *map = FindEntityMap(entityId);
    if (!map) {
        printf("[client_world] Failed to spawn entity. Could not find entity id %u's map.\n", entityId);
        return false;
    }

    size_t entityIndex = map->FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        printf("[client_world] Failed to spawn entity. Entity id %u not found in map id %u.\n", entityId, map->id);
        return false;
    }

    data.entity    = map->entities[entityIndex];
    data.collision = map->collision[entityIndex];
    data.dialog    = map->dialog[entityIndex];
    data.ghosts    = map->ghosts[entityIndex];
    data.life      = map->life[entityIndex];
    data.pathfind  = map->pathfind[entityIndex];
    data.physics   = map->physics[entityIndex];
    data.sprite    = map->sprite[entityIndex];
    return true;
}

void ClientWorld::ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn)
{
    const uint32_t entityId = entitySpawn.entityId;
    if (entityId >= SV_MAX_ENTITIES) {
        printf("[client_world] Failed to spawn entity. Entity id %u out of range.\n", entityId);
        return;
    }

    Tilemap *map = FindEntityMap(entitySpawn.entityId);
    if (!map) {
        printf("[client_world] Failed to spawn entity. Could not find entity id %u's map.\n", entitySpawn.entityId);
        return;
    }
    assert(map->id == entitySpawn.mapId && "wait.. how? we literally just created it a second ago");

    size_t entityIndex = map->FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        printf("[client_world] Failed to spawn entity. Entity id %u not found in map id %u.\n", entityId, entitySpawn.mapId);
        return;
    }

    Entity          &entity    = map->entities[entityIndex];
    AspectCollision &collision = map->collision[entityIndex];
    AspectPhysics   &physics   = map->physics[entityIndex];
    AspectLife      &life      = map->life[entityIndex];
    data::Sprite    &sprite    = map->sprite[entityIndex];

    entity.type      = entitySpawn.type;
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

void ClientWorld::DespawnEntity(uint32_t entityId)
{
    Tilemap *map = FindEntityMap(entityId);
    if (!map) {
        printf("[client_world] Failed to despawn entity. Could not find entity id %u's map.\n", entityId);
        return;
    }
    map->DestroyEntity(entityId);
    entityMapId.erase(entityId);
}

void ClientWorld::ApplyStateInterpolated(EntityInterpolateTuple &data,
    const GhostSnapshot &a, const GhostSnapshot &b, float alpha)
{
    data.entity.position.x = LERP(a.position.x, b.position.x, alpha);
    data.entity.position.y = LERP(a.position.y, b.position.y, alpha);

    data.physics.velocity.x = LERP(a.velocity.x, b.velocity.x, alpha);
    data.physics.velocity.y = LERP(a.velocity.y, b.velocity.y, alpha);

    // TODO(dlb): Should we lerp max health?
    data.life.maxHealth = a.maxHealth;
    data.life.health = LERP(a.health, b.health, alpha);
}

void ClientWorld::ApplyStateInterpolated(uint32_t entityId,
    const GhostSnapshot &a, const GhostSnapshot &b, float alpha)
{
    if (entityId >= SV_MAX_ENTITIES) {
        printf("[client_world] Failed to interpolate entity. Entity id %u out of range.\n", entityId);
        return;
    }

    Tilemap *map = FindEntityMap(entityId);
    if (!map) {
        printf("[client_world] Failed to interpolate entity. Could not find entity id %u's map.\n", entityId);
        return;
    }

    size_t entityIndex = map->FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        printf("[client_world] Failed to interpolate entity. Entity id %u not found in map id %u.\n", entityId, map->id);
        return;
    }

    Entity        &entity  = map->entities[entityIndex];
    AspectPhysics &physics = map->physics[entityIndex];
    AspectLife    &life    = map->life[entityIndex];

    EntityInterpolateTuple data{entity, physics, life};
    ApplyStateInterpolated(data, a, b, alpha);
}

Err ClientWorld::CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now)
{
    Tilemap *map = FindEntityMap(entityId);;
    if (!map) {
        assert(0);
        printf("[client_world] Failed to create dialog. Could not find entity id %u's map.\n", entityId);
        return RN_OUT_OF_BOUNDS;
    }

    size_t entityIndex = map->FindEntityIndex(entityId);
    if (entityIndex == SV_MAX_ENTITIES) {
        assert(0);
        printf("[client_world] Failed to create dialog. Could not find entity id %u in map id %u.\n", entityId, map->id);
        return RN_OUT_OF_BOUNDS;
    }

    AspectDialog &dialog = map->dialog[entityIndex];
    dialog.spawnedAt = now;
    dialog.messageLength = messageLength;
    dialog.message = (char *)calloc((size_t)messageLength + 1, sizeof(*dialog.message));
    if (!dialog.message) {
        return RN_BAD_ALLOC;
    }
    strncpy(dialog.message, message, messageLength);

    return RN_SUCCESS;
}

void ClientWorld::UpdateEntities(GameClient &client)
{
    hoveredEntityId = 0;

    Tilemap *map = LocalPlayerMap();
    if (!map) {
        assert(0);
        printf("[client_world] Failed to find local player entity id %u's map. cannot update entities\n", localPlayerEntityId);
        return;
    }

    for (uint32_t entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
        Entity &entity = map->entities[entityIndex];
        if (entity.type == Entity_None) {
            continue;
        }

        AspectPhysics &physics = map->physics[entityIndex];
        AspectGhost &ghost = map->ghosts[entityIndex];

        // Local player
        if (entity.id == localPlayerEntityId) {
            uint32_t lastProcessedInputCmd = 0;

            // Apply latest snapshot
            const GhostSnapshot &latestSnapshot = ghost.newest();
            if (latestSnapshot.serverTime) {
                ApplyStateInterpolated(entity.id, ghost.newest(), ghost.newest(), 0);
                lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
            }

#if CL_CLIENT_SIDE_PREDICT
            // Apply unacked input
            for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
                InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
                if (inputCmd.seq > lastProcessedInputCmd) {
                    physics.ApplyForce(inputCmd.GenerateMoveForce(physics.speed));
                    map->EntityTick(entity.id, SV_TICK_DT, client.now);
                    map->ResolveEntityTerrainCollisions(entity.id);
                }
            }

            const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
            if (cmdAccumDt > 0) {
                physics.ApplyForce(client.controller.cmdAccum.GenerateMoveForce(physics.speed));
                map->EntityTick(entity.id, cmdAccumDt, client.now);
                map->ResolveEntityTerrainCollisions(entity.id);
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

            ApplyStateInterpolated(entity.id, *snapshotA, *snapshotB, alpha);

            const Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);
            bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, map->EntityRect(entity.id));
            if (hover) {
                bool down = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
                if (down) {
                    client.SendEntityInteract(entity.id);
                }
                hoveredEntityId = entity.id;
            }
        }
    }
}

void ClientWorld::Update(GameClient &gameClient)
{
    UpdateEntities(gameClient);
}

void ClientWorld::DrawEntitySnapshotShadows(uint32_t entityId, Controller &controller, double now)
{
    Tilemap *map = FindEntityMap(entityId);
    if (!map) return;

    size_t entityIndex = map->FindEntityIndex(entityId);
    Entity &entity = map->entities[entityIndex];

    // TODO: Everything that says "ghostInstance" needs to be an entity_id, but we don't
    // want to modify the actual entity... so perhaps we need a "temp" entity that we can
    // use for drawing shadows? Or some other way to simulate the entity moving without
    // modifying the actual entity.
    if (CL_DBG_SNAPSHOT_SHADOWS) {
        AspectGhost &ghost = map->ghosts[entityIndex];

        EntityData ghostData{};
        CopyEntityData(entity.id, ghostData);
        // Prevents accidentally overwriting real entities if someone we pass a tuple
        // to decides to look up the entity by id instead of using the tuple's data.
        ghostData.entity.id = 0;

        EntityInterpolateTuple ghostInterpData{ ghostData.entity, ghostData.physics, ghostData.life };
        EntitySpriteTuple ghostSpriteData{ ghostData.entity, ghostData.sprite };
        EntityTickTuple ghostTickData{ ghostData.entity, ghostData.dialog, ghostData.physics };
        EntityCollisionTuple ghostCollisionData{ ghostData.entity, ghostData.collision, ghostData.physics };

        for (int i = 0; i < ghost.size(); i++) {
            if (!ghost[i].serverTime) {
                continue;
            }
            ApplyStateInterpolated(ghostInterpData, ghost[i], ghost[i], 0);

            const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
            Rectangle ghostRect = map->EntityRect(ghostSpriteData);
            ghostRect = RectShrink(ghostRect, scalePer);
            DrawRectangleRec(ghostRect, Fade(RED, 0.1f));
            DrawRectangleLinesEx(ghostRect, 1, Fade(RED, 0.8f));
        }

        // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
        if (entity.id == localPlayerEntityId) {
#if CL_CLIENT_SIDE_PREDICT
            uint32_t lastProcessedInputCmd = 0;
            const GhostSnapshot &latestSnapshot = ghost.newest();
            if (latestSnapshot.serverTime) {
                ApplyStateInterpolated(ghostInterpData, latestSnapshot, latestSnapshot, 0);
                lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
            }

            //const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
            for (size_t cmdIndex = 0; cmdIndex < controller.cmdQueue.size(); cmdIndex++) {
                InputCmd &inputCmd = controller.cmdQueue[cmdIndex];
                if (inputCmd.seq > lastProcessedInputCmd) {
                    ghostData.physics.ApplyForce(inputCmd.GenerateMoveForce(ghostData.physics.speed));
                    map->EntityTick(ghostTickData, SV_TICK_DT, now);
                    map->ResolveEntityTerrainCollisions(ghostCollisionData);
                    Rectangle ghostRect = map->EntityRect(ghostSpriteData);
                    DrawRectangleRec(ghostRect, Fade(GREEN, 0.1f));
                    DrawRectangleLinesEx(ghostRect, 1, Fade(GREEN, 0.8f));
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
void ClientWorld::DrawDialog(AspectDialog &dialog, Vector2 topCenter)
{
    const float marginBottom = 4.0f;
    Vector2 bgPad{ 8, 2 };
    Vector2 msgSize = MeasureTextEx(fntHackBold20, dialog.message, fntHackBold20.baseSize, 1.0f);
    Vector2 msgPos{
        topCenter.x - msgSize.x / 2,
        topCenter.y - msgSize.y - bgPad.y * 2 - marginBottom
    };
    Rectangle msgBgRect{
        msgPos.x - bgPad.x,
        msgPos.y - bgPad.y,
        msgSize.x + bgPad.x * 2,
        msgSize.y + bgPad.y * 2
    };
    DrawRectangleRounded(msgBgRect, 0.2f, 6, Fade(BLACK, 0.5));
    //DrawRectangleRoundedLines(msgBgRect, 0.2f, 6, 1.0f, RAYWHITE);
    DrawTextEx(fntHackBold20, dialog.message, msgPos, fntHackBold20.baseSize, 1.0f, RAYWHITE);
    //DrawTextShadowEx(fntHackBold20, dialog.message, msgPos, FONT_SIZE, RAYWHITE);
}

// TODO: Entity should just draw it's own damn dialog
void ClientWorld::DrawDialogs(void)
{
    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    for (int entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
        AspectDialog &dialog = map->dialog[entityIndex];
        if (dialog.spawnedAt) {
            Entity &entity = map->entities[entityIndex];
            assert(entity.id && entity.type);
            if (entity.id && entity.type) {
                const Vector2 topCenter = map->EntityTopCenter(entity.id);
                DrawDialog(dialog, topCenter);
            }
        }
    }
}

void ClientWorld::Draw(Controller &controller, double now)
{
    Tilemap *map = LocalPlayerMap();
    if (!map) {
        return;
    }

    Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);

    //--------------------
    // Draw the map
    BeginMode2D(camera2d);
    map->Draw(camera2d);

    //--------------------
    // Draw the entities
    cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);
    for (uint32_t entityIndex = 0; entityIndex < SV_MAX_ENTITIES; entityIndex++) {
        Entity &entity = map->entities[entityIndex];
        if (!entity.id || !entity.type || entity.despawnedAt) {
            continue;
        }

        DrawEntitySnapshotShadows(entity.id, controller, now);

        map->DrawEntity(entity.id);
    }

    //--------------------
    // Draw the dialogs
    float zoom = camera2d.zoom;
    camera2d.zoom = 1;
    DrawDialogs();
    camera2d.zoom = zoom;

    EndMode2D();

    //--------------------
    // Draw entity info
    if (hoveredEntityId) {
        Entity *entity = FindEntity(hoveredEntityId);
        if (entity) {
            map->DrawEntityHoverInfo(hoveredEntityId);
        } else {
            // We were probably hovering an entity while it was despawning?
            assert(!"huh? how can we hover an entity that doesn't exist");
            hoveredEntityId = 0;
        }
    }
}