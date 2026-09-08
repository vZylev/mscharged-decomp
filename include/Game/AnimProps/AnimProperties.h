#ifndef ANIM_PROPERTIES_H
#define ANIM_PROPERTIES_H

#include "Game/SAnim.h"

struct AnimProperties
{
    const char* enumName;
    const char* animName;
    ePlayMode playMode;
    float blendAmount;
    bool mirror;
    int ballRotationMode;
    int endPhase;
    u8 matchCharacterSpeed;
};

#endif
