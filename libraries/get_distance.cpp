#include "get_distance.h"

float getDistanceCM(std::function<long()> pulseInFunc) {
    const float SOUND_SPEED_CM_PER_US = 0.0343;
    long duration = pulseInFunc();
    if (duration == 0 || duration >= 30000UL) {
        return -1.0;
    }
    return duration * SOUND_SPEED_CM_PER_US / 2;
} 