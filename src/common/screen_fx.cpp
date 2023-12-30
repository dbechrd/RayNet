#include "screen_fx.h"

void ScreenFX_Fade(double &startedAt, double duration, double now)
{
    if (startedAt) {
        const double fadeInAlpha = (now - startedAt) / duration;
        if (fadeInAlpha < 1.0f) {
            DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), Fade(BLACK, 1.0f - fadeInAlpha));
        } else {
            startedAt = 0;
        }
    }
}