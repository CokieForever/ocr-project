#ifndef OCRH

#define OCRH

#include <stdlib.h>
#include <math.h>
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

#include <time.h>
#include <dirent.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

typedef struct
{
    char **tab;
    int width, height;
} CharTab;

#define MAX_FONTS 30
#define MAX_WORD 50

#include "general.h"
#include "main.h"




double ColorDist(SDL_Color c1, SDL_Color c2);
CharTab Bichromize(SDL_Color **img, int width, int height);
CharTab BichromizeFast(SDL_Color **img, int width, int height);
SDL_Surface* SurfFromCharTab(CharTab charTab);
SDL_Surface* WriteCharTab(CharTab charTab, TTF_Font *font, SDL_Color color);
SDL_Rect GetZone(CharTab charTab, int x, int y);
SDL_Rect* LocateLetters(CharTab charTab);
SDL_Rect** LocateLines(CharTab charTab, SDL_Rect *lettersTab, int *nbLines);
SDL_Rect** LocateMainTab(CharTab charTab, SDL_Rect *lettersTab, SDL_Rect *mainTabSize);
double CorrelationFunction(CharTab charTab1, CharTab charTab2);
CharTab CreateCharTab(int width, int height, char* initValue);
CharTab ExtractCharTab(CharTab charTab, SDL_Rect pos);
CharTab DeformCharTab(CharTab charTab, int width, int height);
CharTab InverseCharTab(CharTab charTab);
CharTab CopyCharTab(CharTab charTab);
CharTab MergeCharTabs(CharTab charTab1, CharTab charTab2);
CharTab RecutCharTab(CharTab charTab);
CharTab ThinDownCharTab(CharTab charTab);
CharTab** CreateImagesRef(void);
void FreeImagesRef(void);
char RecognizeLetter(CharTab charTab0, SDL_Rect zone, double *rate, int print);
double GetX1Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMin);
double GetX2Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMin);
double GetY1Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMin);
double GetY2Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMin);
CharTab RecognizeMainTab(CharTab charTab, SDL_Rect** mainTab, int width, int height, double *meanRate);
int FullRecognition(SDL_Surface *img,
                    SDL_Color ***colorTab0, CharTab *charTab0, SDL_Surface **img2_0, SDL_Rect **rectTab0, SDL_Rect ***mainTab0,
                    SDL_Rect *mainTabSize0, CharTab ***tabImgR_0, CharTab *lettersTab0, double *recognitionRate0);
#endif
