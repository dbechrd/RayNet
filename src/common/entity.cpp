#include "entity.h"

GhostSnapshot::GhostSnapshot(Msg_S_EntitySpawn &msg)
{
    server_time              = msg.server_time;
    last_processed_input_cmd = msg.last_processed_input_cmd;

    // Entity
    map_id   = msg.map_id;
    position = msg.position;
    //on_warp_cooldown = msg.on_warp_cooldown;

    // Life
    hp_max   = msg.hp_max;
    hp       = msg.hp;

    // Physics
    //speed    = msg.speed;
    velocity = msg.velocity;
}

GhostSnapshot::GhostSnapshot(Msg_S_EntitySnapshot &msg)
{
    server_time              = msg.server_time;
    last_processed_input_cmd = msg.last_processed_input_cmd;

    // Entity
    map_id   = msg.map_id;
    position = msg.position;
    on_warp_cooldown = msg.on_warp_cooldown;

    // Life
    hp_max   = msg.hp_max;
    hp       = msg.hp;

    // Physics
    //speed    = msg.speed;
    velocity = msg.velocity;
}

Vector2 Entity::Position2D(void)
{
    Vector2 screenPos{
        floorf(position.x),
        floorf(position.y - position.z)
    };
    return screenPos;
}

const GfxFrame &Entity::GetSpriteFrame() const
{
    const Sprite &sprite = packs[0].FindById<Sprite>(sprite_id);
    const std::string &anim_name = sprite.anims[direction];
    const GfxAnim &anim = packs[0].FindByName<GfxAnim>(anim_name);
    const std::string &frame_name = anim.GetFrame(anim_state.frame);
    const GfxFrame &frame = packs[0].FindByName<GfxFrame>(frame_name);
    return frame;
}
Rectangle Entity::GetSpriteRect() const
{
    const GfxFrame &frame = GetSpriteFrame();
    const Rectangle rect{
        floorf(position.x - (float)(frame.w / 2)),
        floorf(position.y - position.z - (float)frame.h),
        (float)frame.w,
        (float)frame.h
    };
    return rect;
}

Vector2 Entity::TopCenter(void) const
{
    const Rectangle rect = GetSpriteRect();
    const Vector2 topCenter{
        rect.x + rect.width / 2,
        rect.y
    };
    return topCenter;
}

void Entity::ClearDialog(void)
{
    dialog_spawned_at = 0;
    dialog_title = {};
    dialog_message = {};
}

bool Entity::Attack(double now)
{
    if (now - last_attacked_at > attack_cooldown) {
        last_attacked_at = now;
        attack_cooldown = 0.3;  // todo add cooldown to weapon or config or something
        return true;
    }
    return false;
}

void Entity::TakeDamage(int damage)
{
    if (damage >= hp) {
        hp = 0;
    } else {
        hp -= damage;
    }
}

bool Entity::Alive(void)
{
    return hp > 0;
}

bool Entity::Dead(void)
{
    return !Alive();
}

bool Entity::Active(double now)
{
    if (despawned_at) {
        return false;
    }

    return spawned_at == now
        || last_moved_at == now
        || last_attacked_at == now;
}

void Entity::ApplyForce(Vector3 force)
{
    force_accum = Vector3Add(force_accum, force);
}
