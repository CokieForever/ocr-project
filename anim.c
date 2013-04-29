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

#include "anim.h"

static int AnimationBufferBuilder(void* anim0);
static void MPEGAnimCallback(SDL_Surface* dst, int x, int y, unsigned int w, unsigned int h);
static int BlitMPEGAnim(Animation *anim, SDL_Rect *srcRect, SDL_Surface *dstSurf, SDL_Rect *dstRect);
static int BlitRGBA(SDL_Surface *srcSurf, SDL_Rect *srcRect0, SDL_Surface *dstSurf, SDL_Rect *dstRect0);
static Uint32 GetPixel(SDL_Surface *surface, int x, int y);
static void PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

static int Anim_initialized = 0;

Animation* Anim_LoadEx(const char *baseName, int fps, int firstImg, int lastImg, int bufferSize)
{
    char fullName[ANIM_MAX_STRING],
         *p;
    Animation *anim;
    int done = 0, i, ok,
        nbFiles = 0;
    FILE *file;
    SDL_RWops *rw;

    anim = malloc(sizeof(Animation));
    memset(anim, 0, sizeof(Animation));

    strncpy(anim->filesAddr, baseName, ANIM_MAX_STRING-1);
    if ( (p = strrchr(anim->filesAddr, '.')) )
    {
        strcpy(anim->filesExtension, p);
        *p = '\0';
    }

    for(i=firstImg ; !done ; i++)
    {
        snprintf(fullName, ANIM_MAX_STRING-1, "%s%04d%s", anim->filesAddr, i, anim->filesExtension);
        file = fopen(fullName, "rb");
        if ( (!file && lastImg < 0) || (lastImg >= 0 && i > lastImg) )
            done = 1;
        else if (file)
        {
            nbFiles++;
            fclose(file);
        }
    }

    anim->nbFrames = 0;
    anim->fileNumbersList = malloc(nbFiles * sizeof(int));
    done = 0;
    for(i=firstImg ; !done ; i++)
    {
        snprintf(fullName, ANIM_MAX_STRING-1, "%s%04d%s", anim->filesAddr, i, anim->filesExtension);
        rw = SDL_RWFromFile(fullName, "rb");
        ok = rw && (IMG_isBMP(rw) || IMG_isPNG(rw) || IMG_isJPG(rw));
        if ( (!ok && lastImg < 0) || (lastImg >= 0 && i > lastImg) )
            done = 1;
        else if (ok)
        {
            anim->nbFrames++;
            anim->fileNumbersList[anim->nbFrames-1] = i;
        }

        if (rw)
            SDL_RWclose(rw);
    }

    if (anim->nbFrames < 2)
    {
        free(anim->fileNumbersList);
        free(anim);
        return NULL;
    }

    if (bufferSize == 1)
        bufferSize = 0;
    anim->bufferSize = bufferSize >= 0 && bufferSize <= anim->nbFrames ? bufferSize : anim->nbFrames;
    if (anim->bufferSize > 0)
    {
        anim->buffer = malloc(sizeof(SDL_Surface*) * anim->bufferSize);
        memset(anim->buffer, 0, sizeof(SDL_Surface*) * anim->bufferSize);
    }
    else anim->buffer = NULL;

    anim->firstUniFrame = 0;
    anim->maxReplay = -1;
    anim->isPlaying = 0;
    anim->beginTime = 0;
    anim->fps = fps > 0 ? fps : 25;

    snprintf(fullName, ANIM_MAX_STRING-1, "%s%04d%s", anim->filesAddr, anim->fileNumbersList[0], anim->filesExtension);
    anim->firstImgSurf = IMG_Load(fullName);

    anim->threadTermNow = 0;
    anim->lastUniFrameRead = -1;
    if (anim->buffer && anim->bufferSize > 0)
        anim->bufferBuilderThread = SDL_CreateThread(AnimationBufferBuilder, (void*)anim);
    else anim->bufferBuilderThread = NULL;

    anim->smpeg = NULL;
    return anim;
}

static void MPEGAnimCallback(SDL_Surface* dst, int x, int y, unsigned int w, unsigned int h)
{
    return;
}
Animation* Anim_LoadMPEG(const char *addr)
{
    Animation *anim = malloc(sizeof(Animation));
    memset(anim, 0, sizeof(Animation));

    anim->smpeg = SMPEG_new(addr, &(anim->smpegInfo), 1);
    if (SMPEG_error(anim->smpeg))
    {
        free(anim);
        return NULL;
    }

    SMPEG_enableaudio(anim->smpeg, 0);
    SMPEG_loop(anim->smpeg, 1);

    anim->smpegSurf = SDL_CreateRGBSurface(SDL_HWSURFACE, anim->smpegInfo.width, anim->smpegInfo.height, 32, 0,0,0,0);
    SMPEG_setdisplay(anim->smpeg, anim->smpegSurf, NULL, MPEGAnimCallback);

    SMPEG_renderFrame(anim->smpeg, 0);
    anim->firstImgSurf = SDL_ConvertSurface(anim->smpegSurf, anim->smpegSurf->format, SDL_HWSURFACE);

    return anim;
}


void Anim_Free(Animation *anim)
{
    if (!anim)
        return;

    if (anim->bufferBuilderThread)
    {
        anim->threadTermNow = 1;
        SDL_WaitThread(anim->bufferBuilderThread, NULL);
        anim->threadTermNow = 0;
    }
    if (anim->fileNumbersList)
        free(anim->fileNumbersList);
    if (anim->buffer)
        free(anim->buffer);
    if (anim->firstImgSurf)
        SDL_FreeSurface(anim->firstImgSurf);
    if (anim->smpegSurf)
        SDL_FreeSurface(anim->smpegSurf);
    if (anim->smpeg)
        SMPEG_delete(anim->smpeg);

    free(anim);
}

int Anim_BlitEx(Animation *anim, SDL_Rect *srcRect, SDL_Surface *dstSurf, SDL_Rect *dstRect, Uint8 transparency, Uint32 *alphaColor)
{
    int currentFrame, r,
        currentBufferFrame,
        currentUniFrame;

    if (!anim)
        return 0;

    if (anim->smpeg)
        return BlitMPEGAnim(anim, srcRect, dstSurf, dstRect);

    if (!anim->fileNumbersList)
        return 0;

    currentUniFrame = anim->isPlaying ? ((SDL_GetTicks() - anim->beginTime) * anim->fps / 1000 + anim->firstUniFrame) : anim->firstUniFrame;
    if (anim->maxReplay >= 0 && currentUniFrame >= (anim->maxReplay+1) * anim->nbFrames)
    {
        Anim_Pause(anim);
        currentUniFrame = (anim->maxReplay+1) * anim->nbFrames -1;
        anim->firstUniFrame = currentUniFrame;
    }
    currentFrame = currentUniFrame % anim->nbFrames;
    currentBufferFrame = anim->bufferSize > 0 ? currentUniFrame % anim->bufferSize : 0;

    if (!anim->buffer || anim->bufferSize <= 0 || !anim->bufferBuilderThread || (!anim->buffer[currentBufferFrame] && currentUniFrame < anim->bufferBuilderPosition))
    {
        char fullName[ANIM_MAX_STRING];
        SDL_Surface *surf;

        snprintf(fullName, ANIM_MAX_STRING-1, "%s%04d%s", anim->filesAddr, currentFrame, anim->filesExtension);
        surf = IMG_Load(fullName);

        SDL_SetAlpha(surf, SDL_SRCALPHA, transparency);
        if (alphaColor)
            SDL_SetColorKey(surf, SDL_SRCCOLORKEY, *alphaColor);
        r = BlitRGBA(surf, srcRect, dstSurf, dstRect);

        SDL_FreeSurface(surf);
    }
    else
    {
        if (currentUniFrame >= anim->bufferBuilderPosition )
        {
            Anim_Pause(anim);
            if (currentUniFrame - anim->bufferBuilderPosition > 1)
                anim->uniFrameWanted = currentUniFrame;
            while (currentUniFrame >= anim->bufferBuilderPosition)
                SDL_Delay(1);
            Anim_Play(anim);
        }

        SDL_SetAlpha(anim->buffer[currentBufferFrame], SDL_SRCALPHA, transparency);
        if (alphaColor)
            SDL_SetColorKey(anim->buffer[currentBufferFrame], SDL_SRCCOLORKEY, *alphaColor);
        else SDL_SetColorKey(anim->buffer[currentBufferFrame], 0, 0);
        r = BlitRGBA(anim->buffer[currentBufferFrame], srcRect, dstSurf, dstRect);
    }

    anim->lastUniFrameRead = currentUniFrame;

    return r;
}

static int AnimationBufferBuilder(void* anim0)
{
    Animation *anim = (Animation*)anim0;
    char fullName[ANIM_MAX_STRING];
    int uniFrame, bufferFrame, i, done = 0;

    if (!anim->buffer || anim->bufferSize <= 0)
        return 0;

    for (i=0 ; i < anim->bufferSize ; i++)
        anim->buffer[i] = NULL;

    anim->bufferBuilderPosition = 0;
    uniFrame = 0;
    anim->uniFrameWanted = -1;
    while (!anim->threadTermNow && !done)
    {
        if (anim->uniFrameWanted >= 0)
        {
            uniFrame = anim->uniFrameWanted;
            anim->uniFrameWanted = -1;
            anim->lastUniFrameRead = uniFrame-1;
        }
        if (uniFrame < anim->lastUniFrameRead + anim->bufferSize)
        {
            bufferFrame = uniFrame % anim->bufferSize;
            snprintf(fullName, ANIM_MAX_STRING-1, "%s%04d%s", anim->filesAddr, anim->fileNumbersList[uniFrame%anim->nbFrames], anim->filesExtension);
            if (anim->buffer[bufferFrame])
                SDL_FreeSurface(anim->buffer[bufferFrame]);
            anim->buffer[bufferFrame] = IMG_Load(fullName);
            uniFrame++;
            anim->bufferBuilderPosition = uniFrame;
        }
        else SDL_Delay(1);

        if (anim->bufferSize >= anim->nbFrames)
        {
            done = 1;
            for (i=0 ; i < anim->nbFrames ; i++)
            {
                if (!anim->buffer[i])
                    done = 0;
            }
        }
    }

    if (done)
        anim->bufferBuilderPosition = 0x00FFFFFF;
    while (done && !anim->threadTermNow)
        SDL_Delay(10);

    for (i=0 ; i < anim->bufferSize ; i++)
    {
        if (anim->buffer[i])
            SDL_FreeSurface(anim->buffer[i]);
        anim->buffer[i] = NULL;
    }

    return 1;
}

static int BlitMPEGAnim(Animation *anim, SDL_Rect *srcRect, SDL_Surface *dstSurf, SDL_Rect *dstRect)
{
    if (!anim || !anim->smpeg)
        return 0;

    return SDL_BlitSurface(anim->smpegSurf, srcRect, dstSurf, dstRect);
}

void Anim_Play(Animation *anim)
{
    if (!anim || anim->isPlaying)
        return;

    anim->isPlaying = 1;
    if (anim->smpeg)
        SMPEG_play(anim->smpeg);
    else anim->beginTime = SDL_GetTicks();
}

void Anim_Stop(Animation *anim)
{
    if (!anim)
        return;

    anim->isPlaying = 0;
    if (anim->smpeg)
    {
        SMPEG_pause(anim->smpeg);
        SMPEG_rewind(anim->smpeg);
    }
    else
    {
        anim->lastUniFrameRead = -1;
        anim->firstUniFrame = 0;
        anim->uniFrameWanted = 0;
    }
}

void Anim_Pause(Animation *anim)
{
    if (!anim || !anim->isPlaying)
        return;

    anim->isPlaying = 0;
    if (anim->smpeg)
        SMPEG_pause(anim->smpeg);
    else anim->firstUniFrame += (SDL_GetTicks() - anim->beginTime) * anim->fps / 1000;
}

SDL_Surface* Anim_GetFirstSurf(Animation *anim)
{
    if (!anim)
        return NULL;
    else return anim->firstImgSurf;
}

void Anim_SetMaxReplay(Animation *anim, int maxReplay)
{
    if (!anim)
        return;

    if (maxReplay < 0)
        maxReplay = -1;
    anim->maxReplay = maxReplay;

    if (anim->smpeg)
        SMPEG_loop(anim->smpeg, maxReplay < 0 ? 1 : 0);
}

int Anim_IsPlaying(Animation *anim)
{
    return anim ? anim->isPlaying : 0;
}


Movement Anim_CreateMovement(SDL_Rect initPos, SDL_Rect finalPos, int time)
{
    Movement mov;

    mov.initPos = initPos;
    mov.finalPos = finalPos;
    mov.time = time;

    Anim_StopMovement(&mov);
    return mov;
}

SDL_Rect Anim_GetPosMovement(Movement mov)
{
    double p;

    if (!mov.isPlaying)
        return mov.firstPos;

    p = (SDL_GetTicks() - mov.beginTime) / (double)mov.timeLeft;
    SDL_Rect pos;

    if (p > 1)
        p = 1;

    pos.x = p*mov.finalPos.x + (1-p)*mov.firstPos.x;
    pos.y = p*mov.finalPos.y + (1-p)*mov.firstPos.y;

    return pos;
}

void Anim_PlayMovement(Movement *mov)
{
    if (mov->isPlaying)
        return;

    mov->isPlaying = 1;
    mov->beginTime = SDL_GetTicks();
}

void Anim_PauseMovement(Movement *mov)
{
    if (!mov->isPlaying)
        return;

    mov->firstPos = Anim_GetPosMovement(*mov);
    mov->isPlaying = 0;
    mov->timeLeft -= SDL_GetTicks() - mov->beginTime;
}

void Anim_StopMovement(Movement *mov)
{
    if (!mov)
        return;

    mov->isPlaying = 0;
    mov->firstPos = mov->initPos;
    mov->timeLeft = mov->time;
}

int Anim_IsMovementOver(Movement mov)
{
    return SDL_GetTicks() - mov.beginTime >= mov.timeLeft;
}


Distorsion Anim_CreateDistorsion(SDL_Surface *img, int ampl, double freq, int duration, int flags)
{
    Distorsion dst;

    dst.originalImg = SDL_ConvertSurface(img, img->format, SDL_HWSURFACE);
    dst.img = SDL_ConvertSurface(img, img->format, SDL_HWSURFACE);
    SDL_SetAlpha(dst.originalImg, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
    SDL_SetAlpha(dst.img, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    if ((flags & ANIM_VDISTORSION) == ANIM_VDISTORSION)
        dst.vDstTab = malloc(sizeof(int) * img->w);
    else dst.vDstTab = NULL;

    if ((flags & ANIM_HDISTORSION) == ANIM_HDISTORSION)
        dst.hDstTab = malloc(sizeof(int) * img->h);
    else dst.hDstTab = NULL;

    dst.amplitude = ampl;
    dst.frequence = freq;
    dst.duration = duration;

    dst.lastTime = SDL_GetTicks();
    dst.isPlaying = 0;
    dst.distorsion = 0;

    if (!Anim_initialized)
        srand(time(NULL));
    Anim_initialized = 1;

    dst.lifeTime = rand() % (int)(1000/dst.frequence);

    return dst;
}

SDL_Surface* Anim_GetDistorsionImg(Distorsion *dst)
{
    int i;

    if (!dst->originalImg || !dst->img)
        return NULL;

    if (!dst->isPlaying)
        return dst->img;

    if (dst->distorsion)
    {
        double fact = (SDL_GetTicks() - dst->lastTime)*1.0 / dst->duration;
        if (fact > 1)
            fact = 1 - (fact-1);
        if (fact < 0)
        {
            dst->distorsion = 0;

            if (dst->originalImg->format->Amask != 0)
                SDL_FillRect(dst->img, NULL, SDL_MapRGBA(dst->img->format, 0,0,0, SDL_ALPHA_TRANSPARENT));
            BlitRGBA(dst->originalImg, NULL, dst->img, NULL);

            dst->lastTime = SDL_GetTicks();
            dst->lifeTime = rand() % (int)(1000/dst->frequence);
        }
        else
        {
            SDL_Surface *bufferSurf = SDL_ConvertSurface(dst->originalImg, dst->originalImg->format, SDL_HWSURFACE);
            SDL_Rect rect, pos;

            SDL_SetAlpha(bufferSurf, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

            if (dst->img->format->Amask == 0)
                SDL_FillRect(dst->img, NULL, SDL_MapRGB(dst->img->format, 0,0,0));
            else SDL_FillRect(dst->img, NULL, SDL_MapRGBA(dst->img->format, 0,0,0, SDL_ALPHA_TRANSPARENT));

            if (dst->vDstTab)
            {
                if (bufferSurf->format->Amask == 0)
                    SDL_FillRect(bufferSurf, NULL, SDL_MapRGB(bufferSurf->format, 0,0,0));
                else SDL_FillRect(bufferSurf, NULL, SDL_MapRGBA(bufferSurf->format, 0,0,0, SDL_ALPHA_TRANSPARENT));

                rect.y = 0;
                rect.w = 1;
                rect.h = dst->img->h;
                for (i=0 ; i < dst->img->w ; i++)
                {
                    rect.x = i;
                    pos.x = i;
                    pos.y = dst->vDstTab[i] * fact;
                    BlitRGBA(dst->originalImg, &rect, bufferSurf, &pos);
                }
            }

            if (dst->hDstTab)
            {
                rect.x = 0;
                rect.w = dst->img->w;
                rect.h = 1;
                for (i=0 ; i < dst->img->h ; i++)
                {
                    rect.y = i;
                    pos.y = i;
                    pos.x = dst->hDstTab[i] * fact;
                    BlitRGBA(bufferSurf, &rect, dst->img, &pos);
                }
            }

            SDL_FreeSurface(bufferSurf);
        }

        return dst->img;
    }
    else
    {
        if (SDL_GetTicks() - dst->lastTime >= dst->lifeTime)
        {
            for (i=0 ; dst->vDstTab && i < dst->img->w ; i++)
                dst->vDstTab[i] = (rand() % (dst->amplitude*2)) - dst->amplitude;
            for (i=0 ; dst->hDstTab && i < dst->img->h ; i++)
                dst->hDstTab[i] = (rand() % (dst->amplitude*2)) - dst->amplitude;

            dst->distorsion = 1;
            dst->lastTime = SDL_GetTicks();
        }

        return dst->originalImg;
    }
}

int Anim_PlayDistorsion(Distorsion *dst)
{
    if (!dst)
        return 0;

    dst->isPlaying = 1;
    dst->lastTime = SDL_GetTicks();
    return 1;
}

int Anim_PauseDistorsion(Distorsion *dst)
{
    if (!dst)
        return 0;

    dst->isPlaying = 0;
    return 1;
}

int Anim_StopDistorsion(Distorsion *dst)
{
    if (!dst)
        return 0;

    dst->isPlaying = 0;
    dst->distorsion = 0;
    BlitRGBA(dst->originalImg, NULL, dst->img, NULL);

    return 1;
}

void Anim_FreeDistorsion(Distorsion *dst)
{
    if (!dst)
        return;

    if (dst->img)
        SDL_FreeSurface(dst->img);
    if (dst->originalImg)
        SDL_FreeSurface(dst->originalImg);
    if (dst->vDstTab)
        free(dst->vDstTab);
    if (dst->hDstTab)
        free(dst->hDstTab);

    return;
}

int Anim_ChangeDistorsionImg(Distorsion *dst, SDL_Surface *img)
{
    if (!dst || !img)
        return 0;

    if (dst->img)
        SDL_FreeSurface(dst->img);
    dst->img = SDL_ConvertSurface(img, img->format, SDL_HWSURFACE);
    SDL_SetAlpha(dst->img, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    if (dst->originalImg)
        SDL_FreeSurface(dst->originalImg);
    dst->originalImg = SDL_ConvertSurface(img, img->format, SDL_HWSURFACE);
    SDL_SetAlpha(dst->originalImg, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    return 1;
}





static int BlitRGBA(SDL_Surface *srcSurf, SDL_Rect *srcRect0, SDL_Surface *dstSurf, SDL_Rect *dstRect0)
{
    SDL_Rect srcRect = {0},
             dstRect = {0};
    int x, y,
        srcHasAlpha, dstHasAlpha,
        srcHasGlobalAlpha, srcHasColorKey;
    double x1, y1, x2, y2, p;
    Uint8 r1,g1,b1,a1,
          r2,g2,b2,a2;
    Uint32 pixel;

    if (!dstSurf || !srcSurf)
        return -1;

    srcHasAlpha = srcSurf->format->Amask != 0 && (srcSurf->flags & SDL_SRCALPHA) == SDL_SRCALPHA;
    dstHasAlpha = dstSurf->format->Amask != 0 && (dstSurf->flags & SDL_SRCALPHA) == SDL_SRCALPHA;
    srcHasGlobalAlpha = (srcSurf->flags & SDL_SRCALPHA) == SDL_SRCALPHA && srcSurf->format->alpha != SDL_ALPHA_OPAQUE;
    srcHasColorKey = (srcSurf->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY;

    if ((!dstHasAlpha && !srcHasGlobalAlpha && !srcHasColorKey) || !srcHasAlpha)
        return SDL_BlitSurface(srcSurf, srcRect0, dstSurf, dstRect0);

    if (srcRect0)
        srcRect = *srcRect0;
    else
    {
        srcRect.w = srcSurf->w;
        srcRect.h = srcSurf->h;
    }

    if (dstRect0)
        dstRect = *dstRect0;

    if (srcRect.x < 0)
        srcRect.x = 0;
    if (srcRect.y < 0)
        srcRect.y = 0;

    SDL_LockSurface(dstSurf);
    SDL_LockSurface(srcSurf);
    for (x=0 ; x < srcRect.w ; x++)
    {
        if (x+srcRect.x < srcSurf->w && x+dstRect.x < dstSurf->w && x+dstRect.x >= 0)
        {
            for (y=0 ; y < srcRect.h ; y++)
            {
                if (y+srcRect.y < srcSurf->h && y+dstRect.y < dstSurf->h && y+dstRect.y >= 0)
                {
                    a1 = SDL_ALPHA_OPAQUE;
                    pixel = GetPixel(srcSurf, x+srcRect.x, y+srcRect.y);
                    if (srcHasAlpha)
                        SDL_GetRGBA(pixel, srcSurf->format, &r1, &g1, &b1, &a1);
                    else SDL_GetRGB(pixel, srcSurf->format, &r1, &g1, &b1);

                    SDL_GetRGBA(GetPixel(dstSurf, x+dstRect.x, y+dstRect.y), dstSurf->format, &r2, &g2, &b2, &a2);

                    if (srcHasColorKey && srcSurf->format->colorkey == pixel)
                        a1 = SDL_ALPHA_TRANSPARENT;
                    else if (srcHasGlobalAlpha)
                        a1 *= srcSurf->format->alpha / 255.0;

                    y1 = a1/255.0; x1 = 1-y1;
                    y2 = a2/255.0; x2 = 1-y2;
                    p = y2 == 0 ? (y1==0 ? 0 : 1) : 1 - pow(x1, 1/y2);

                    PutPixel(dstSurf, x+dstRect.x, y+dstRect.y,
                            SDL_MapRGBA(dstSurf->format, p*r1+(1-p)*r2, p*g1+(1-p)*g2, p*b1+(1-p)*b2, 255*(1-x1*x2) ) );
                }
            }
        }
    }
    SDL_UnlockSurface(dstSurf);
    SDL_UnlockSurface(srcSurf);

    return 0;
}

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static Uint32 GetPixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
        case 1:
            return *p;

        case 2:
            return *(Uint16 *)p;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;

        case 4:
            return *(Uint32 *)p;

        default:
            return 0;       /* shouldn't happen, but avoids warnings */
    }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}



