#include "player.h"

void Player::Draw(void) {
    DrawRectanglePro(
        { position.x, position.y, size.x, size.y },
        { size.x / 2, size.y },
        0,
        color
    );
}