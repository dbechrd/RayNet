#include "client_world.h"
#include "game_client.h"

Entity *ClientWorld::GetEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
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
        return &entities[entityId];
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return 0;
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

        entity->latestDialog = dialogId;
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

        if (entity->latestDialog != dialogId) {
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
    for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = entities[entityId];
        if (entity.type == Entity_None) {
            continue;
        }
        EntityGhost &ghost = ghosts[entityId];

        // Local player
        if (entityId == client.LocalPlayerEntityId()) {
            uint32_t lastProcessedInputCmd = 0;

            // Apply latest snapshot
            const EntitySnapshot &latestSnapshot = ghost.snapshots.newest();
            if (latestSnapshot.serverTime) {
                entity.ApplyStateInterpolated(ghost.snapshots.newest(), ghost.snapshots.newest(), 0);
                lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
            }

#if CL_CLIENT_SIDE_PREDICT
            // Apply unacked input
            const double cmdAccumDt = client.now - client.controller.lastInputSampleAt;
            for (size_t cmdIndex = 0; cmdIndex < client.controller.cmdQueue.size(); cmdIndex++) {
                InputCmd &inputCmd = client.controller.cmdQueue[cmdIndex];
                if (inputCmd.seq > lastProcessedInputCmd) {
                    entity.ApplyForce(inputCmd.GenerateMoveForce(entity.speed));
                    entity.Tick(client.now, SV_TICK_DT);
                    map.ResolveEntityTerrainCollisions(entity);
                }
            }
            //entity.Tick(&client.controller.cmdAccum, cmdAccumDt);
#endif

            // Check for ignored input packets
            uint32_t oldestInput = client.controller.cmdQueue.oldest().seq;
            if (oldestInput > lastProcessedInputCmd + 1) {
                //printf(" localPlayer: %d inputs dropped\n", oldestInput - lastProcessedInputCmd - 1);
            }
        } else {
            // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
            const double renderAt = client.ServerNow() - SV_TICK_DT;

            size_t snapshotBIdx = 0;
            while (snapshotBIdx < ghost.snapshots.size()
                && ghost.snapshots[snapshotBIdx].serverTime <= renderAt)
            {
                snapshotBIdx++;
            }

            const EntitySnapshot *snapshotA = 0;
            const EntitySnapshot *snapshotB = 0;

            if (snapshotBIdx <= 0) {
                snapshotA = &ghost.snapshots.oldest();
                snapshotB = &ghost.snapshots.oldest();
            } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
                snapshotA = &ghost.snapshots.newest();
                snapshotB = &ghost.snapshots.newest();
            } else {
                snapshotA = &ghost.snapshots[snapshotBIdx - 1];
                snapshotB = &ghost.snapshots[snapshotBIdx];
            }

            float alpha = 0;
            if (snapshotB != snapshotA) {
                alpha = (renderAt - snapshotA->serverTime) /
                    (snapshotB->serverTime - snapshotA->serverTime);
            }

            entity.ApplyStateInterpolated(*snapshotA, *snapshotB, alpha);
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
    Vector2 msgSize = MeasureTextEx(fntHackBold20, dialog.message, FONT_SIZE, 1.0f);
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
    DrawTextEx(fntHackBold20, dialog.message, msgPos, FONT_SIZE, 1.0f, RAYWHITE);
    //DrawTextShadowEx(fntHackBold20, dialog.message, msgPos, FONT_SIZE, RAYWHITE);
}

void ClientWorld::DrawDialogs(void)
{
    for (int i = 0; i < ARRAY_SIZE(dialogs); i++) {
        Dialog &dialog = dialogs[i];
        Entity &entity = entities[dialog.entityId];
        if (entity.type && !entity.despawnedAt) {
            Vector2 entityTopCenter{
                entity.position.x,
                entity.position.y - entity.size.y
            };
            DrawDialog(dialog, entityTopCenter);
        }
    }
}

void ClientWorld::Draw(void)
{
    DrawDialogs();
}