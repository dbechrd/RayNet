#pragma once
#include "../common.h"
#include "../entity.h"

struct EntitySnapshot {
    double      serverTime {};
    uint32_t    lastProcessedInputCmd {};
    uint32_t    id         {};
    Entity      entity     {};
    //EntityType  type       {};
    //Vector2     velocity   {};
    //Vector2     position   {};
};

struct EntitySpawnEvent {
    double      serverTime {};
    uint32_t    id         {};
    Entity      entity     {};
    //EntityType  type       {};
    //Color       color      {};
    //Vector2     size       {};
    //float       radius     {};
    //float       drag       {};
    //float       speed      {};
    //Vector2     velocity   {};
    //Vector2     position   {};
};