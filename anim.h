/*

The OCR Project - Optical Character Recognition try / SDL Graphic Library development
Copyright (C) 2013 Cokie (cokie.forever@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ANIMH

#define ANIMH

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <smpeg.h>

#define ANIM_MAX_STRING 1000

typedef struct
{
    int nbFrames,
        fps,
        maxReplay;
    char filesExtension[ANIM_MAX_STRING],
         filesAddr[ANIM_MAX_STRING];

    int firstUniFrame,
        isPlaying;
    Uint32 beginTime;

    SDL_Surface **buffer,
                *firstImgSurf;
    SDL_Thread *bufferBuilderThread;
    int bufferSize,
        *fileNumbersList,
        bufferBuilderPosition,
        lastUniFrameRead,
        uniFrameWanted,
        threadTermNow;

    //SMPEG
    SMPEG *smpeg;
    SMPEG_Info smpegInfo;
    SDL_Surface *smpegSurf;

} Animation;

typedef struct
{
    SDL_Rect initPos,
             finalPos;
    int time;

    SDL_Rect firstPos;
    int timeLeft,
        isPlaying,
        beginTime;
} Movement;


#define ANIM_VDISTORSION 0x0001
#define ANIM_HDISTORSION 0x0010
typedef struct
{
    SDL_Surface *originalImg,
                *img;
    int *vDstTab,
        *hDstTab;

    int amplitude,
        duration;
    double frequence;

    int lastTime,
        isPlaying,
        distorsion,
        lifeTime;
} Distorsion;

Animation* Anim_LoadEx(const char *baseName, int fps, int firstImg, int lastImg, int bufferSize);
#define Anim_Load(bn, fps) Anim_LoadEx(bn, fps, 0, -1, fps)
Animation* Anim_LoadMPEG(const char *addr);
void Anim_Free(Animation *anim);
int Anim_BlitEx(Animation *anim, SDL_Rect *srcRect, SDL_Surface *dstSurf, SDL_Rect *dstRect, Uint8 transparency, Uint32 *alphaColor);
#define Anim_Blit(a, sR, dS, dR) Anim_BlitEx(a, sR, dS, dR, SDL_ALPHA_OPAQUE, NULL)
void Anim_Play(Animation *anim);
void Anim_Stop(Animation *anim);
void Anim_Pause(Animation *anim);
SDL_Surface* Anim_GetFirstSurf(Animation *anim);
void Anim_SetMaxReplay(Animation *anim, int maxReplay);
int Anim_IsPlaying(Animation *anim);

Movement Anim_CreateMovement(SDL_Rect initPos, SDL_Rect finalPos, int time);
SDL_Rect Anim_GetPosMovement(Movement mov);
void Anim_PlayMovement(Movement *mov);
void Anim_PauseMovement(Movement *mov);
void Anim_StopMovement(Movement *mov);
int Anim_IsMovementOver(Movement mov);

Distorsion Anim_CreateDistorsion(SDL_Surface *img, int ampl, double freq, int duration, int flags);
SDL_Surface* Anim_GetDistorsionImg(Distorsion *dst);
int Anim_PlayDistorsion(Distorsion *dst);
int Anim_PauseDistorsion(Distorsion *dst);
int Anim_StopDistorsion(Distorsion *dst);
void Anim_FreeDistorsion(Distorsion *dst);
int Anim_ChangeDistorsionImg(Distorsion *dst, SDL_Surface *img);

#endif
