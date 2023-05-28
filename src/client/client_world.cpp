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

Entity *ClientWorld::GetEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = map.entities[entityId];
        if (entity.type && !entity.despawnedAt) {
            return &entity;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return 0;
}

Entity *ClientWorld::GetEntityDeadOrAlive(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        return &map.entities[entityId];
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return 0;
}

void ClientWorld::ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn)
{
    uint32_t entityId = entitySpawn.entityId;
    if (entityId >= SV_MAX_ENTITIES) {
        printf("[client_world] WARN: Failed to spawn entity. ID %u out of range.\n", entityId);
        return;
    }

    Entity          &entity    = map.entities[entityId];
    AspectCollision &collision = map.collision[entityId];
    AspectPhysics   &physics   = map.physics[entityId];
    AspectLife      &life      = map.life[entityId];
    data::Sprite    &sprite    = map.sprite[entityId];

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

void ClientWorld::ApplyStateInterpolated(uint32_t entityId, const GhostSnapshot &a, const GhostSnapshot &b, float alpha)
{
    Entity        &entity  = map.entities[entityId];
    AspectPhysics &physics = map.physics[entityId];
    AspectLife    &life    = map.life[entityId];

    entity.position.x = LERP(a.position.x, b.position.x, alpha);
    entity.position.y = LERP(a.position.y, b.position.y, alpha);

    physics.velocity.x = LERP(a.velocity.x, b.velocity.x, alpha);
    physics.velocity.y = LERP(a.velocity.y, b.velocity.y, alpha);

    // TODO(dlb): Should we lerp max health?
    life.maxHealth = a.maxHealth;
    life.health = LERP(a.health, b.health, alpha);
}

Err ClientWorld::CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now)
{
    Entity *entity = GetEntity(entityId);
    if (!entity) {
        return RN_SUCCESS;  // might be an error.. but might be success.. hmm
    }

    //Dialog dialog{ entityId, messageLength, message, now };
    //dialogs.push_back(dialog);
    int dialogId = 1;
    Dialog *dialog = 0;
    for (; dialogId < ARRAY_SIZE(dialogs); dialogId++) {
        if (!dialogs[dialogId].entityId) {
            dialog = &dialogs[dialogId];
            break;
        }
    }

    if (dialog) {
        dialog->entityId = entityId;
        dialog->spawnedAt = now;
        dialog->messageLength = messageLength;
        dialog->message = (char *)calloc((size_t)messageLength + 1, sizeof(*dialog->message));
        if (!dialog->message) {
            return RN_BAD_ALLOC;
        }
        strncpy(dialog->message, message, messageLength);

        AspectDialog &dialog = map.dialog[entityId];
        dialog.latestDialog = dialogId;
    } else {
        return RN_BAD_ALLOC;
    }

    return RN_SUCCESS;
}

Err ClientWorld::DestroyDialog(uint32_t dialogId)
{
    if (dialogId < CL_MAX_DIALOGS) {
        free(dialogs[dialogId].message);
        dialogs[dialogId] = {};
        return RN_SUCCESS;
    }
    return RN_BAD_ID;
}

void ClientWorld::RemoveExpiredDialogs(GameClient &gameClient)
{
    for (int dialogId = 0; dialogId < ARRAY_SIZE(dialogs); dialogId++) {
        Dialog &dialog = dialogs[dialogId];
        Entity *entity = GetEntity(dialog.entityId);
        if (!entity) {
            continue;
        }

        AspectDialog &aspectDialog = map.dialog[dialog.entityId];
        if (aspectDialog.latestDialog != dialogId) {
            DestroyDialog(dialogId);
        }

        const double duration = CL_DIALOG_DURATION_MIN + CL_DIALOG_DURATION_PER_CHAR * dialog.messageLength;
        if (gameClient.now - dialog.spawnedAt > duration) {
            DestroyDialog(dialogId);
        }
    }
}

void ClientWorld::UpdateEntities(GameClient &client)
{
    client.hoveredEntityId = 0;
    for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = map.entities[entityId];
        if (entity.type == Entity_None) {
            continue;
        }

        AspectPhysics &physics = map.physics[entityId];
        Ghost &ghost = ghosts[entityId];

        // Local player
        if (entityId == client.LocalPlayerEntityId()) {
            uint32_t lastProcessedInputCmd = 0;

            // Apply latest snapshot
            const GhostSnapshot &latestSnapshot = ghost.newest();
            if (latestSnapshot.serverTime) {
                ApplyStateInterpolated(entityId, ghost.newest(), ghost.newest(), 0);
                lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
            }

#if CL_CLIENT_SIDE_PREDICT
            // Apply unacked input
            for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
                InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
                if (inputCmd.seq > lastProcessedInputCmd) {
                    physics.ApplyForce(inputCmd.GenerateMoveForce(physics.speed));
                    map.EntityTick(entityId, SV_TICK_DT);
                    map.ResolveEntityTerrainCollisions(entityId);
                }
            }

            const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
            if (cmdAccumDt > 0) {
                physics.ApplyForce(client.controller.cmdAccum.GenerateMoveForce(physics.speed));
                map.EntityTick(entityId, cmdAccumDt);
                map.ResolveEntityTerrainCollisions(entityId);
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

            ApplyStateInterpolated(entityId, *snapshotA, *snapshotB, alpha);

            const Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);
            bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, map.EntityRect(entityId));
            if (hover) {
                bool down = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);
                if (down) {
                    client.SendEntityInteract(entityId);
                }
                client.hoveredEntityId = entityId;
            }
        }
    }
}

void ClientWorld::Update(GameClient &gameClient)
{
    UpdateEntities(gameClient);
    RemoveExpiredDialogs(gameClient);
}

// TODO(dlb): Where should this live? Probably not in ClientWorld?
void ClientWorld::DrawDialog(Dialog &dialog, Vector2 bottomCenter)
{
    const float marginBottom = 4.0f;
    Vector2 bgPad{ 8, 2 };
    Vector2 msgSize = MeasureTextEx(fntHackBold20, dialog.message, fntHackBold20.baseSize, 1.0f);
    Vector2 msgPos{
        bottomCenter.x - msgSize.x / 2,
        bottomCenter.y - msgSize.y - bgPad.y * 2 - marginBottom
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

void ClientWorld::DrawDialogs(void)
{
    for (int i = 0; i < ARRAY_SIZE(dialogs); i++) {
        Dialog &dialog = dialogs[i];
        Entity &entity = map.entities[dialog.entityId];
        if (entity.type && !entity.despawnedAt) {
            const Vector2 topCenter = map.EntityTopCenter(dialog.entityId);
            DrawDialog(dialog, topCenter);
        }
    }
}

void ClientWorld::Draw(void)
{
    DrawDialogs();
}