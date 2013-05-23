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

#ifndef MAINH

#define MAINH

#define SCREENW 640
#define SCREENH 640
#define CONSOLEH 320

#define MAX_CONSOLE 5000

#define DISPLAY_MENU 1
#define DISPLAY_PROCESSING 2
#define DISPLAY_RESULT 3

#include "general.h"
#include <FMOD.h>
#include "ocr.h"
#include "textedition.h"
#include "anim.h"



int BlitRGBA(SDL_Surface *srcSurf, SDL_Rect *srcRect0, SDL_Surface *dstSurf, SDL_Rect *dstRect0);
Uint32 GetPixel(SDL_Surface *surface, int x, int y);
void PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
SDL_Color** GetAllPixels(SDL_Surface *surf);

int AreSameColor(SDL_Color c1, SDL_Color c2);
void FreeTab(char** tab, int length);
void DrawRect(SDL_Rect rect, SDL_Surface *surf, Uint32 color);
int IsInRect(int x, int y, SDL_Rect rect);
void BlitSurfCenter(SDL_Surface *surf1, SDL_Surface *surf2);
void ExchangeVars(void *pvar1, void *pvar2, size_t size);
SDL_Surface* MainInit(void);
SDL_Surface* CreateBackground(void);
Distorsion CreateMenu(int selectedItem, Distorsion *prevDst);
SDL_Rect* SearchWord(CharTab lettersTab, const char *word, int caseSensitive, int getResult);
int SearchAllWords(CharTab lettersTab, int minLength, int *nbWords0, int *nbWordsFound0, char *wordsFound0, size_t bufSize);
int FullProcess(void *ptv);
void DrawAllLettersRect(const SDL_Rect *specialLetters, int nbSpecialLetters);
void Temporize(int fps);
void MemoryFree(int all);
int LoadData(void *ptv);

int ManageEvent(SDL_Event event);
void ManageEvent_Result(SDL_Event event);
int ManageEvent_Menu(SDL_Event event);

void ManageDisplay(void);

void InitConsole(void);
void UninitConsole(void);
void PrintInConsole(const char *text);
void DeleteLastConsoleLine(void);
void DisplayConsole(void);
void PrintGeneralMsg(const char *title, const char *msg, const char *voiceFile);
int PrintGeneralTextBox(const char *title, char *text, int nbChar, const char *voiceFile);

int PrintLoadingBar(SDL_Surface *dstSurf, SDL_Rect *dstPos, double percent);
SDL_Rect GetLoadingBarDim(void);
int SetWorkingPercent(double percent);

void DisplayMainMenu(void);

#endif
