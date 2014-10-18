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

#include "main.h"

static CharTab **tabImagesRef = NULL;
static TextEdition consoleTE;
static TTF_Font *consoleFont = NULL;
static SDL_Rect consolePos,
                arrowButtonPos;
static SDL_Surface *consoleSurf = NULL,
                   *arrowDownSurf = NULL,
                   *arrowHighSurf = NULL;
static int consoleInitialized = 0,
           consoleDisplayed = 0,
           consoleDisplayChange = 0;
static Movement consoleMov;

static CharTab imgBichromized, recognizedLetters;
static SDL_Surface *imgToProcess = NULL,
                   *imgProcessed = NULL,
                   *imgRecognizedLetters = NULL,
                   *mainBackground = NULL;
static SDL_Rect *lettersPos = NULL,
                **lettersPosMainTab = NULL,
                lettersPosMainTabSize;

static Animation *logoAnim = NULL,
                 *spaceAnimIntro = NULL,
                 *spaceAnimTravel = NULL,
                 *spaceAnimEnd = NULL;


static int displayMode = DISPLAY_MENU,
           resultDisplayed = 0;
static double workingPercent = 0;

static SDL_Surface *loadingBarBG = NULL,
                   *loadingBarDeco = NULL;
static Uint8 loadingBarBGAlpha = 255;

static Distorsion menuDistorsion;
static char menuTexts[][MAX_STRING] = {"Load image", "Process", "Show character", "Quit", ""};

static SDL_Thread *processThread = NULL;

static FMOD_SYSTEM *mainFMODSystem = NULL;


#define All_Quit() do { TTF_Quit(); IMG_Quit(); SDL_Quit(); TE_Quit();\
                        FMOD_System_Close(mainFMODSystem); FMOD_System_Release(mainFMODSystem); } while (0)


int main ( int argc, char** argv )
{
    int done = 0;
    SDL_Surface *screen;
    SDL_Event event;
    SDL_Thread *loadingThread = NULL;

    printf("Initialization...\n");
    if ( !(screen = MainInit()) )
        return 1;
    InitConsole();

    loadingThread = SDL_CreateThread(LoadData, NULL);
    PrintGeneralMsg("THE OCR PROJECT", "Welcome on the Optical Character Recognition Project.\n\nVersion: 0.1.2 beta (October 18th, 2014)\nAuthor: Cokie\nLicense: GNU GPL v2.0", NULL);


    while (!done)
    {
        if (SDL_PollEvent(&event))
            done = ManageEvent(event);

        ManageDisplay();
        SDL_Flip(screen);
        Temporize(25);
    }

    if (loadingThread)
        SDL_WaitThread(loadingThread, NULL);
    if (processThread)
        SDL_KillThread(processThread);

    MemoryFree(1);
    All_Quit();

    printf("Exited cleanly.\n");
    return 0;
}

int LoadData(void *ptv)
{
    mainBackground = IMG_Load("img\\gui\\background-3.png");
    arrowHighSurf = IMG_Load("img\\gui\\arrow-high.png");
    arrowDownSurf = IMG_Load("img\\gui\\arrow-down.png");

    menuDistorsion = CreateMenu(0, NULL);

    logoAnim = Anim_Load("img\\gui\\logo\\.png", 25);
    Anim_Play(logoAnim);

    spaceAnimIntro = Anim_LoadEx("img\\gui\\spacetrip\\intro\\x2-.png", 25, 0, -1, 0);
    spaceAnimTravel = Anim_LoadEx("img\\gui\\spacetrip\\travel\\x2-.png", 25, 0, -1, 0);
    spaceAnimEnd = Anim_LoadEx("img\\gui\\spacetrip\\end\\x2-.png", 25, 0, -1, 0);

    Anim_SetMaxReplay(spaceAnimIntro, 0);
    Anim_SetMaxReplay(spaceAnimEnd, 0);

    return 1;
}

int FullProcess(void *ptv)
{
    int nbWords,
        nbWordsFound;
    char wordsFound[MAX_STRING] = "\0";
    const char musicFileAddr[] = "sounds\\musics\\space-dream.mp3";
    TTF_Font *mainFont;
    SDL_Color blackColor = {0, 0, 0};
    FMOD_SOUND *music = NULL;
    FMOD_CHANNEL *channel = NULL;

    displayMode = DISPLAY_PROCESSING;

    if (FMOD_OK == FMOD_System_CreateSound(mainFMODSystem, musicFileAddr, FMOD_SOFTWARE | FMOD_2D | FMOD_CREATESTREAM, 0, &music) && music)
        FMOD_System_PlaySound(mainFMODSystem, FMOD_CHANNEL_FREE, music, 0, &channel);

    FullRecognition(imgToProcess, NULL, &imgBichromized, &imgProcessed, &lettersPos, &lettersPosMainTab,
                    &lettersPosMainTabSize, &tabImagesRef, &recognizedLetters, NULL);

    DrawAllLettersRect(NULL, 0);

    printf("Writing result...\n");
    mainFont = TTF_OpenFont("fonts\\gui\\mainFont.ttf", 25);
    imgRecognizedLetters = WriteCharTab(recognizedLetters, mainFont, blackColor);

    printf("Searching for words.\n");
    SearchAllWords(recognizedLetters, 3, &nbWords, &nbWordsFound, wordsFound, MAX_STRING-1);
    printf("%d words tested, %d found.\nList of found words:\n*******%s\n*******\n\n", nbWords, nbWordsFound, wordsFound);

    TTF_CloseFont(mainFont);

    printf("Ready.\n\n");

    displayMode = DISPLAY_RESULT;

    if (music)
    {
        int time = SDL_GetTicks();
        while (SDL_GetTicks() - time < 7500)
        {
            FMOD_Channel_SetVolume(channel, 1 - (SDL_GetTicks() - time)/7500.0);
            SDL_Delay(10);
        }
        FMOD_Sound_Release(music);
    }

    return 1;
}

void DrawAllLettersRect(const SDL_Rect *specialLetters, int nbSpecialLetters)
{
    int i, j;

    for (i=0 ; lettersPos[i].w > 0 ; i++)
        DrawRect(lettersPos[i], imgProcessed, SDL_MapRGB(imgProcessed->format, 0,0,255));
    for (i=0 ; i < lettersPosMainTabSize.w ; i++)
    {
        for (j=0 ; j < lettersPosMainTabSize.h ; j++)
            DrawRect(lettersPosMainTab[i][j], imgProcessed, SDL_MapRGB(imgProcessed->format, 255,0,0));
    }
    if (specialLetters)
    {
        for (i=0 ; i < nbSpecialLetters ; i++)
            DrawRect(lettersPosMainTab[specialLetters[i].x][specialLetters[i].y], imgProcessed, SDL_MapRGB(imgProcessed->format, 0,200,0));
    }
}

int ManageEvent(SDL_Event event)
{
    int done = 0, r;

    switch (event.type)
    {
        case SDL_QUIT:
            done = 1;
            break;

        case SDL_KEYDOWN:
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                if (displayMode == DISPLAY_MENU)
                    done = 1;
                else
                {
                    MemoryFree(0);
                    displayMode = DISPLAY_MENU;
                }
            }
            else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
            {
                if (displayMode == DISPLAY_RESULT)
                {
                    char word[101] = "";
                    SDL_Rect *result;

                    resultDisplayed = 1;
                    if (PrintGeneralTextBox("Please enter the word you are looking for:", word, 100, "sounds\\voices\\enter-word.wav") && word[0] && word[1])
                    {
                        printf("Looking for word %s...\n", word);
                        if ( !(result = SearchWord(recognizedLetters, word, 0, 1)) )
                        {
                            printf("Word not found.\n");
                            DrawAllLettersRect(NULL, 0);
                        }
                        else
                        {
                            printf("Word found!\n");
                            DrawAllLettersRect(result, strlen(word));
                            free(result);
                        }
                    }
                }
            }

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (IsInRect(event.button.x, event.button.y, arrowButtonPos) && !consoleDisplayChange)
                {
                    SDL_Rect pos = {0};

                    consoleDisplayChange = 1;
                    if (consoleDisplayed)
                        pos.y = SCREENH;
                    else pos.y = SCREENH - CONSOLEH;
                    consoleMov = Anim_CreateMovement(consolePos, pos, 500);
                    Anim_PlayMovement(&consoleMov);

                    break;
                }
            }
            break;
        }
    }

    switch (displayMode)
    {
        case DISPLAY_MENU:
            r = ManageEvent_Menu(event);
            if (r == 1)
            {
                if (!imgToProcess)
                    PrintGeneralMsg("WARNING", "No image is currently loaded.\nPlease load an image before trying to process.", "sounds\\voices\\no-image-loaded.wav");
                else
                {
                    displayMode = DISPLAY_PROCESSING;
                    processThread = SDL_CreateThread(FullProcess, NULL);
                    Anim_Play(spaceAnimIntro);
                }
            }
            else if (r == 0)
            {
                char buffer[MAX_STRING] = "img/to-process/";
                if (PrintGeneralTextBox("Please enter the full address of the image you want to load:", buffer, MAX_STRING-1, "sounds\\voices\\enter-address.wav"))
                {
                    if (imgToProcess)
                        SDL_FreeSurface(imgToProcess);
                    imgToProcess = IMG_Load(buffer);
                    if (!imgToProcess)
                        PrintGeneralMsg("ERROR", "Failed to load the specified image.\nPlease double-check your address.", "sounds\\voices\\error-loading-image.wav");
                }
            }
            else if (r == 3)
                done = 1;
            break;
        case DISPLAY_PROCESSING:
            break;
        case DISPLAY_RESULT:
            ManageEvent_Result(event);
            break;
    }

    if (!consoleDisplayChange && consoleDisplayed)
        TE_HoldTextEdition(&consoleTE, event);

    return done;
}

void ManageEvent_Result(SDL_Event event)
{
    switch (event.type)
    {
        case SDL_KEYDOWN:
        {
            if (event.key.keysym.sym == SDLK_RIGHT)
                resultDisplayed = (resultDisplayed+1) % 3;
            else if (event.key.keysym.sym == SDLK_LEFT)
                resultDisplayed = (resultDisplayed-1 +3) % 3;

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (resultDisplayed == 1)
                {
                    int x = event.button.x - (SCREENW - imgProcessed->w)/2,
                        y = event.button.y - (SCREENH - imgProcessed->h)/2,
                        i;

                    for (i=0 ; lettersPos[i].w > 0 && !IsInRect(x,y,lettersPos[i]) ; i++);
                    if (lettersPos[i].w > 0)
                        RecognizeLetter(imgBichromized, lettersPos[i], NULL, 1);
                }
            }
            break;
        }
    }

    return;
}

int ManageEvent_Menu(SDL_Event event)
{
    static int nbMenuItems = -1;
    static int selectedItem = 0;

    if (nbMenuItems < 0)
        for (nbMenuItems = 0 ; menuTexts[nbMenuItems][0] ; nbMenuItems++);


    switch (event.type)
    {
        case SDL_KEYDOWN:
        {
            if (event.key.keysym.sym == SDLK_DOWN)
            {
                selectedItem = (selectedItem+1) % nbMenuItems;
                menuDistorsion = CreateMenu(selectedItem, &menuDistorsion);
            }
            else if (event.key.keysym.sym == SDLK_UP)
            {
                selectedItem = (selectedItem-1 + nbMenuItems) % nbMenuItems;
                menuDistorsion = CreateMenu(selectedItem, &menuDistorsion);
            }
            else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
                return selectedItem;

            break;
        }


    }

    return -1;
}

void ManageDisplay(void)
{
    SDL_Surface *screen = SDL_GetVideoSurface();

    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
    BlitSurfCenter(mainBackground, screen);



    if (displayMode == DISPLAY_MENU)
    {
        DisplayMainMenu();
    }
    else if (displayMode == DISPLAY_RESULT)
    {
        SDL_Rect rect = {0};
        SDL_Surface *surf = Anim_GetFirstSurf(logoAnim);
        int n;

        rect.x = (SCREENW - surf->w) / 2;
        rect.y = (SCREENH - surf->h) / 2;

        if (Anim_IsPlaying(spaceAnimTravel))
        {
            if (spaceAnimTravel->maxReplay < 0)
                Anim_SetMaxReplay(spaceAnimTravel, spaceAnimTravel->lastUniFrameRead / spaceAnimTravel->nbFrames);
            Anim_Blit(spaceAnimTravel, NULL, screen, NULL);
            Anim_Blit(logoAnim, NULL, screen, &rect);
        }
        else
        {
            if (spaceAnimTravel->maxReplay > 0 && !Anim_IsPlaying(spaceAnimEnd))
            {
                Anim_Stop(spaceAnimTravel);
                Anim_SetMaxReplay(spaceAnimTravel, -1);
                Anim_Play(spaceAnimEnd);
            }

            if (Anim_IsPlaying(spaceAnimEnd))
            {
                Anim_Blit(spaceAnimEnd, NULL, screen, NULL);
                n = spaceAnimEnd->lastUniFrameRead % spaceAnimEnd->nbFrames;
                if (n < 25)
                    Anim_BlitEx(logoAnim, NULL, screen, &rect, (24-n)*255/25, NULL);
            }
            else
            {
                if (spaceAnimEnd->lastUniFrameRead >= 0)
                    Anim_Stop(spaceAnimEnd);

                if (resultDisplayed == 1)
                    BlitSurfCenter(imgProcessed, screen);
                else if (resultDisplayed == 2)
                    BlitSurfCenter(imgRecognizedLetters, screen);
                else BlitSurfCenter(imgToProcess, screen);
            }
        }
    }
    else if (displayMode == DISPLAY_PROCESSING)
    {
        SDL_Rect rect = {0},
                 dim = GetLoadingBarDim();
        SDL_Surface *surf = Anim_GetFirstSurf(logoAnim);
        int n;

        rect.x = (SCREENW - surf->w) / 2;
        rect.y = (SCREENH - surf->h) / 2;

        if (Anim_IsPlaying(spaceAnimIntro))
        {
            Anim_Blit(spaceAnimIntro, NULL, screen, NULL);
            n = spaceAnimIntro->nbFrames - (spaceAnimIntro->lastUniFrameRead % spaceAnimIntro->nbFrames);
            if (n <= 25)
                Anim_BlitEx(logoAnim, NULL, screen, &rect, (25-n)*255/25, NULL);
        }
        else
        {
            if (!Anim_IsPlaying(spaceAnimTravel))
            {
                Anim_Stop(spaceAnimIntro);
                Anim_Play(spaceAnimTravel);
            }
            Anim_Blit(spaceAnimTravel, NULL, screen, NULL);

            Anim_Blit(logoAnim, NULL, screen, &rect);

            rect.y = SCREENH - (dim.h+20);
            rect.x = (SCREENW - dim.w) / 2;
            PrintLoadingBar(screen, &rect, workingPercent);
        }
    }

    DisplayConsole();
}

void DisplayConsole(void)
{
    SDL_Surface *screen = SDL_GetVideoSurface();

    if (consoleDisplayChange)
    {
        if (Anim_IsMovementOver(consoleMov))
        {
            consoleDisplayChange = 0;
            consoleDisplayed = !consoleDisplayed;
        }
        consolePos = Anim_GetPosMovement(consoleMov);
    }

    if (consoleDisplayed)
    {
        arrowButtonPos.x = screen->w - (arrowDownSurf ? arrowDownSurf->w : 0) - 2;
        arrowButtonPos.y = consolePos.y - (arrowDownSurf ? arrowDownSurf->h : 0) - 2;

        if (!consoleDisplayChange)
            TE_DisplayTextEdition(&consoleTE);
        SDL_BlitSurface(consoleSurf, NULL, screen, &consolePos);
        SDL_BlitSurface(arrowDownSurf, NULL, screen, &arrowButtonPos);
    }
    else
    {
        arrowButtonPos.x = screen->w - (arrowHighSurf ? arrowHighSurf->w : 0) - 2;
        arrowButtonPos.y = consolePos.y - (arrowHighSurf ? arrowHighSurf->h : 0) - 2;

        if (consoleDisplayChange)
            SDL_BlitSurface(consoleSurf, NULL, screen, &consolePos);
        SDL_BlitSurface(arrowHighSurf, NULL, screen, &arrowButtonPos);
    }
}

void DisplayMainMenu(void)
{
    SDL_Surface *screen = SDL_GetVideoSurface(),
                *menuSurf = Anim_GetDistorsionImg(&menuDistorsion);

    SDL_BlitSurface(menuSurf, NULL, screen, NULL);

    return;
}

void Temporize(int fps)
{
    static int timeLast = 0;
    int waitingTime = (1000/fps) - (SDL_GetTicks()-timeLast);

    if (waitingTime > 0)
        SDL_Delay(waitingTime);

    timeLast = SDL_GetTicks();
}

void MemoryFree(int all)
{
    FreeTab(imgBichromized.tab, imgBichromized.width);
    imgBichromized.tab = NULL;
    FreeTab((char**)lettersPosMainTab, lettersPosMainTabSize.w);
    lettersPosMainTab = NULL;
    FreeTab(recognizedLetters.tab, recognizedLetters.width);
    recognizedLetters.tab = NULL;

    if (imgProcessed)
        SDL_FreeSurface(imgProcessed);
    imgProcessed = NULL;

    if (imgRecognizedLetters)
        SDL_FreeSurface(imgRecognizedLetters);
    imgRecognizedLetters = NULL;

    if (lettersPos)
        free(lettersPos);
    lettersPos = NULL;

    if (all)
    {
        if (imgToProcess)
            SDL_FreeSurface(imgToProcess);
        imgToProcess = NULL;

        if (mainBackground)
            SDL_FreeSurface(mainBackground);
        mainBackground = NULL;

        if (arrowHighSurf)
            SDL_FreeSurface(arrowHighSurf);
        arrowHighSurf = NULL;

        if (arrowDownSurf)
            SDL_FreeSurface(arrowDownSurf);
        arrowDownSurf = NULL;

        UninitConsole();

        FreeImagesRef();
        Anim_Free(logoAnim);
        logoAnim = NULL;
        Anim_Free(spaceAnimIntro);
        spaceAnimIntro = NULL;
        Anim_Free(spaceAnimTravel);
        spaceAnimTravel = NULL;
        Anim_Free(spaceAnimEnd);
        spaceAnimEnd = NULL;
        Anim_FreeDistorsion(&menuDistorsion);

        if (loadingBarBG)
            SDL_FreeSurface(loadingBarBG);
        loadingBarBG = NULL;
        if (loadingBarDeco)
            SDL_FreeSurface(loadingBarDeco);
        loadingBarDeco = NULL;
    }
}

SDL_Rect* SearchWord(CharTab lettersTab, const char *word, int caseSensitive, int getResult)
{
    int x, y, x2, y2, i,
        dir, done;
    char c;
    SDL_Rect *result = getResult ? malloc(sizeof(SDL_Rect) * strlen(word)) : NULL;

    for(x=0 ; x < lettersTab.width ; x++)
    {
        for (y=0 ; y < lettersTab.height ; y++)
        {
            if (word[0] == lettersTab.tab[x][y] || (!caseSensitive && toupper(word[0]) == toupper(lettersTab.tab[x][y])))
            {
                dir = 1;
                x2 = x; y2 = y;
                done = 0;
                if (result)
                {
                    result[0].x = x;
                    result[0].y = y;
                }

                for (i=1 ; !done && word[i] ; i++)
                {
                    switch (dir)
                    {
                        case 1:
                            y2--;
                            break;
                        case 2:
                            x2++; y2--;
                            break;
                        case 3:
                            x2++;
                            break;
                        case 4:
                            x2++; y2++;
                            break;
                        case 5:
                            y2++;
                            break;
                        case 6:
                            x2--; y2++;
                            break;
                        case 7:
                            x2--;
                            break;
                        case 8:
                            x2--; y2--;
                            break;
                        default:
                            break;
                    }

                    c = x2 >= 0 && y2 >= 0 && x2 < lettersTab.width && y2 < lettersTab.height ? lettersTab.tab[x2][y2] : '\0';

                    if (toupper(c) != toupper(word[i]) || (caseSensitive && c != word[i]))
                    {
                        dir++;
                        i = 0;
                        x2 = x; y2 = y;
                    }

                    if (result)
                    {
                        result[i].x = x2;
                        result[i].y = y2;
                    }

                    if (dir > 8)
                        done = 1;
                }

                if (!done)
                    return getResult ? result : (SDL_Rect*)1;
            }
        }
    }

    if (result)
        free(result);
    return NULL;
}

int SearchAllWords(CharTab lettersTab, int minLength, int *nbWords0, int *nbWordsFound0, char *wordsFound0, size_t bufSize)
{
    FILE *dictFile = fopen("dict.txt", "rb");
    int i = 0, nbWords, nbWordsFound = 0;
    char word[MAX_WORD+1], *p = NULL,
         wordsFound[MAX_STRING+1] = "\0";

    for (nbWords = 0 ; fgets(word, MAX_WORD-1, dictFile) ; nbWords++);
    fseek(dictFile, 0, SEEK_SET);

    SetWorkingPercent(0);
    while (fgets(word, MAX_WORD-1, dictFile))
    {
        if ( (p = strchr(word, '\n')) )
            *p = '\0';

        if (strlen(word) >= minLength && SearchWord(lettersTab, word, 0, 0))
        {
            snprintf(wordsFound, MAX_STRING-1, "%s\n%s", wordsFound, word);
            nbWordsFound++;
        }

        SetWorkingPercent((++i)*1.0/nbWords);
    }
    SetWorkingPercent(1);
    fclose(dictFile);

    if (nbWords0)
        *nbWords0 = nbWords;
    if (*nbWordsFound0)
        *nbWordsFound0 = nbWordsFound;
    if (wordsFound0 && bufSize > 1)
        strncpy(wordsFound0, wordsFound, bufSize-1);

    return 1;
}



SDL_Surface* CreateBackground(void)
{
    SDL_Rect rect;
    SDL_Surface *background;
    int i;

    rect.w = SCREENW;
    rect.h = 1;
    rect.x = 0;
    background = SDL_CreateRGBSurface(SDL_HWSURFACE, SCREENW, SCREENH, 32, 0,0,0,0);
    for (i=0 ; i < SCREENH ; i++)
    {
        int rate = fabs(round(i * 510.0 / SCREENH)-255);
        rect.y = i;
        SDL_FillRect(background, &rect, SDL_MapRGB(background->format, rate, 255, 0));
    }

    return background;
}

Distorsion CreateMenu(int selectedItem, Distorsion *prevDst)
{
    SDL_Surface *screen = SDL_GetVideoSurface(),
                *menuSurf = NULL,
                *textSurf = NULL,
                *arrowSurf = IMG_Load("img\\gui\\arrow-right.png");
    TTF_Font *font = TTF_OpenFont("fonts\\gui\\digi-graphics.ttf", 20);
    int lineSkip = TTF_FontLineSkip(font)+5, i;
    Distorsion dst;
    SDL_Rect pos, pos2;
    SDL_Color white = {255,255,255};

    menuSurf = SDL_CreateRGBSurface(SDL_HWSURFACE, screen->w, screen->h, 32, RED_MASK,GREEN_MASK,BLUE_MASK,ALPHA_MASK);
    SDL_FillRect(menuSurf, NULL, SDL_MapRGBA(menuSurf->format, 0,0,0,SDL_ALPHA_TRANSPARENT));

    pos.x = 40;
    pos.y = SCREENH - 200;
    for (i=0 ; menuTexts[i][0] ; i++)
    {
        textSurf = TTF_RenderText_Blended(font, menuTexts[i], white);
        BlitRGBA(textSurf, NULL, menuSurf, &pos);

        if (i == selectedItem)
        {
            pos2.x = pos.x - (arrowSurf->w + 10);
            pos2.y = pos.y - (arrowSurf->h - textSurf->h)/2;
            BlitRGBA(arrowSurf, NULL, menuSurf, &pos2);
        }

        pos.y += lineSkip;
        SDL_FreeSurface(textSurf);
    }

    TTF_CloseFont(font);
    SDL_FreeSurface(arrowSurf);

    if (!prevDst || !prevDst->img)
    {
        dst = Anim_CreateDistorsion(menuSurf, 25, 0.1, 100, ANIM_HDISTORSION);
        Anim_PlayDistorsion(&dst);
    }
    else
    {
        Anim_ChangeDistorsionImg(prevDst, menuSurf);
        dst = *prevDst;
    }

    SDL_FreeSurface(menuSurf);
    return dst;
}

SDL_Surface* MainInit(void)
{
    int flags;
    SDL_Surface *screen;

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return NULL;
    }

    if (TTF_Init() < 0)
    {
        printf( "Unable to init TTF: %s\n", TTF_GetError() );
        SDL_Quit();
        return NULL;
    }

    flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(flags)&flags) != flags)
    {
        printf( "Unable to init IMG: %s\n", IMG_GetError() );
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    if ( !(screen = SDL_SetVideoMode(SCREENW, SCREENH, 16, SDL_HWSURFACE|SDL_DOUBLEBUF)) )
    {
        printf("Unable to set %dx%d video: %s\n", SCREENW, SCREENH, SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    FMOD_System_Create(&mainFMODSystem);
    FMOD_System_Init(mainFMODSystem, 1, FMOD_INIT_NORMAL, NULL);

    memset(&consoleTE, 0, sizeof(TextEdition));
    memset(&consoleMov, 0, sizeof(Movement));
    memset(&imgBichromized, 0, sizeof(CharTab));
    memset(&recognizedLetters, 0, sizeof(CharTab));
    memset(&menuDistorsion, 0, sizeof(Distorsion));

    return screen;
}

void InitConsole(void)
{
    SDL_Color color = {255, 255, 255};
    SDL_Rect pos;

    if (consoleInitialized)
        return;

    consoleSurf = SDL_CreateRGBSurface(SDL_HWSURFACE, SCREENW, CONSOLEH, 32, 0,0,0,0);
    SDL_FillRect(consoleSurf, NULL, SDL_MapRGB(consoleSurf->format, 0,0,0));
    SDL_SetAlpha(consoleSurf, SDL_SRCALPHA, 230);

    consoleFont = TTF_OpenFont("fonts\\gui\\console.ttf", 15);
    consolePos.x = 0; consolePos.y = consoleDisplayed ? SCREENH - CONSOLEH : SCREENH;
    consolePos.w = SCREENW; consolePos.h = CONSOLEH;
    pos.x = 5; pos.y = 5;
    pos.w = SCREENW-10; pos.h = CONSOLEH-10;

    memset(&consoleTE, 0, sizeof(TextEdition));
    consoleTE.colorBG = color;
    consoleTE.blitStyle = TE_BLITSTYLE_BLENDED;
    consoleTE.blitSurf = consoleSurf;
    consoleTE.blitSurfPos = consolePos;
    consoleTE.blitSurfPos.y = SCREENH - CONSOLEH;

    TE_NewTextEdition(&consoleTE, MAX_CONSOLE, pos, consoleFont, color,
                      TE_STYLE_MULTILINE | TE_STYLE_AUTOJUMP | TE_STYLE_VSCROLL | TE_STYLE_JUSTDISPLAY);

    TE_SetEditionText(&consoleTE, "Console initialized. System ready.\n");
    consoleInitialized = 1;
}

void UninitConsole(void)
{
    if (!consoleInitialized)
        return;

    TE_DeleteTextEdition(&consoleTE);
    if (consoleFont)
        TTF_CloseFont(consoleFont);
    consoleInitialized = 0;
}

void PrintInConsole(const char *text)
{
    int l1;
    char cpText[MAX_CONSOLE], *p,
         consoleText[MAX_CONSOLE];

    if (!consoleInitialized)
    {
        fputs(text, stdout);
        return;
    }

    strncpy(cpText, text, MAX_CONSOLE-10);
    l1 = strlen(cpText);
    strncpy(consoleText, consoleTE.text, MAX_CONSOLE-1);

    while (strlen(consoleText)+l1 >= MAX_CONSOLE-5)
    {
        if ( !(p = strchr(consoleText, '\n')) )
            consoleText[0] = '\0';
        else strcpy(consoleText, p+1);
    }

    strcat(consoleText, cpText);
    TE_SetEditionText(&consoleTE, consoleText);
    TE_SetCursorPos(&consoleTE, strlen(consoleTE.text));
}

void DeleteLastConsoleLine(void)
{
    char consoleText[MAX_CONSOLE]="", *p;
    int i;

    strncpy(consoleText, consoleTE.text, MAX_CONSOLE-1);

    if (!consoleInitialized)
        return;

    for(i=strlen(consoleText)-1 ; i >= 0 && consoleText[i] == '\n' ; i--);
    consoleText[i+1] = '\0';

    if ( (p = strrchr(consoleText, '\n')) )
        *(p+1) = '\0';
    else consoleText[0] = '\0';

    TE_SetEditionText(&consoleTE, consoleText);
}

SDL_Color** GetAllPixels(SDL_Surface *surf)
{
    int x, y;
    SDL_Color **tab = malloc(sizeof(SDL_Color*) * surf->w);

    SDL_LockSurface(surf);
    for (x=0 ; x < surf->w ; x++)
    {
        tab[x] = malloc(sizeof(SDL_Color) * surf->h);
        for (y=0 ; y < surf->h ; y++)
            SDL_GetRGB(GetPixel(surf, x, y), surf->format, &(tab[x][y].r), &(tab[x][y].g), &(tab[x][y].b));
    }
    SDL_UnlockSurface(surf);

    return tab;
}

int AreSameColor(SDL_Color c1, SDL_Color c2)
{
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

void FreeTab(char** tab, int length)
{
    int i;

    if (!tab)
        return;

    for (i=0 ; i<length ; i++)
    {
        if (tab[i])
            free(tab[i]);
    }
    free(tab);
}

void DrawRect(SDL_Rect rect, SDL_Surface *surf, Uint32 color)
{
    int x,y;

    SDL_LockSurface(surf);

    for (x=rect.x ; x < rect.x + rect.w ; x++)
    {
        PutPixel(surf, x, rect.y, color);
        PutPixel(surf, x, rect.y + rect.h -1, color);
    }

    for (y=rect.y ; y < rect.y + rect.h ; y++)
    {
        PutPixel(surf, rect.x, y, color);
        PutPixel(surf, rect.x + rect.w -1, y, color);
    }

    SDL_UnlockSurface(surf);
}

int IsInRect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && y >= rect.y && x <= rect.x+rect.w && y <= rect.y+rect.h;
}

void BlitSurfCenter(SDL_Surface *surf1, SDL_Surface *surf2)
{
    SDL_Rect rect;

    if (!surf1 || !surf2)
        return;

    rect.x = (surf2->w - surf1->w) / 2;
    rect.y = (surf2->h - surf1->h) / 2;

    SDL_BlitSurface(surf1, NULL, surf2, &rect);
}

void ExchangeVars(void *pvar1, void *pvar2, size_t size)
{
    void *pvarTmp = malloc(size);
    memcpy(pvarTmp, pvar1, size);
    memcpy(pvar1, pvar2, size);
    memcpy(pvar2, pvarTmp, size);
    free(pvarTmp);
}

int PrintLoadingBar(SDL_Surface *dstSurf, SDL_Rect *dstPos, double percent)
{
    int xMax, x, y, shading, margin;
    Uint8 r, g, b, a;
    SDL_Rect pos;
    double y1, y2, x1, x2, p;

    if (loadingBarBG)
        SDL_FreeSurface(loadingBarBG);

    loadingBarBG = IMG_Load("img\\gui\\loading-bar-bg-small.png");
    SDL_LockSurface(loadingBarBG);
    SDL_GetRGBA(GetPixel(loadingBarBG, loadingBarBG->w/2, loadingBarBG->h/2), loadingBarBG->format, &r, &g, &b, &loadingBarBGAlpha);
    SDL_UnlockSurface(loadingBarBG);

    if (!loadingBarDeco)
        loadingBarDeco = IMG_Load("img\\gui\\loading-bar-deco-small.png");

    if (percent > 1)
        percent = 1;
    else if (percent < 0)
        percent = 0;

    margin = round(0.01077 * loadingBarBG->w);
    xMax = (loadingBarBG->w - margin) * percent +margin/2;
    shading = xMax >= loadingBarBG->w*0.25 ? xMax : loadingBarBG->w*0.25;

    SDL_LockSurface(loadingBarBG);
    for (x=0 ; x < xMax ; x++)
    {
        for (y=0 ; y < loadingBarBG->h ; y++)
        {
            SDL_GetRGBA(GetPixel(loadingBarBG, x, y), loadingBarBG->format, &r, &g, &b, &a);
            y2 = a/255.0; x2 = 1-y2;
            y1 = 1 - ((xMax-x)*1.0/shading); x1 = 1-y1;
            p = y2 == 0 ? (y1==0 ? 0 : 1) : 1 - pow(x1, 1/y2);

            if (y2 == 0)
                x1 = 1;
            PutPixel(loadingBarBG, x, y, SDL_MapRGBA(loadingBarBG->format, p*255 + (1-p)*r, (1-p)*g, (1-p)*b, 255*(1-x1*x2) ) );
        }
    }
    SDL_UnlockSurface(loadingBarBG);

    SDL_BlitSurface(loadingBarBG, NULL, dstSurf, dstPos);
    pos = *dstPos;
    pos.x += xMax - loadingBarDeco->w/2;
    pos.y += loadingBarBG->h/2 - loadingBarDeco->h/2;
    SDL_BlitSurface(loadingBarDeco, NULL, dstSurf, &pos);

    return 1;
}

SDL_Rect GetLoadingBarDim(void)
{
    SDL_Rect dim = {0};

    if (!loadingBarDeco || !loadingBarBG)
        return dim;

    dim.w = loadingBarBG->w;
    dim.h = loadingBarDeco->h*0.5;

    return dim;
}

int SetWorkingPercent(double percent)
{
    static int lastCallTime = 0;
    int remainingTime,
        remainingSec,
        remainingMin,
        remainingHours;

    if (percent < 0 || percent > 1)
        return 0;

    if (percent == 0)
        lastCallTime = SDL_GetTicks();

    if (percent == 0 || workingPercent != percent)
    {
        if (percent == 0 || (int)(workingPercent*100) != (int)(percent*100))
        {
            if (percent > 0)
                DeleteLastConsoleLine();
            printf("Processing... %d%%", (int)(percent*100));

            if (percent > 0)
            {
                remainingTime = (SDL_GetTicks() - lastCallTime) * (percent==1 ? 1 : (1-percent)/percent) / 1000;
                remainingMin = remainingTime / 60;
                remainingSec = remainingTime % 60;
                remainingHours = remainingMin / 60;
                remainingMin %= 60;
                printf(" (%02d:%02d:%02d)\n", remainingHours, remainingMin, remainingSec);
            }
            else printf("\n");
        }
        workingPercent = percent;
    }

    return 1;
}

void PrintGeneralMsg(const char *title, const char *msg, const char *voiceFile)
{
    if (!title)
        return;

    SDL_Surface *frameSurf = IMG_Load("img\\gui\\frame.png"),
                *titleSurf = NULL,
                *screen = SDL_GetVideoSurface();
    TTF_Font *bigFont = TTF_OpenFont("fonts\\gui\\space-age.ttf", 40),
             *smallFont = TTF_OpenFont("fonts\\gui\\neuropol.ttf", 15);
    SDL_Color titleColor = {200, 0, 0},
              textColor = {255, 255, 255};
    SDL_Event event;
    SDL_Rect msgPos = {0},
             titlePos = {0},
             framePos = {0};
    int done = 0, time = SDL_GetTicks()/1000,
        sTime = time % 60,
        mTime = (time / 60) % 60,
        hTime = time / 3600;
    static int msgNumber = 1;
    char fullMsg[MAX_STRING] = "";
    TextEdition te;
    FMOD_SOUND *voice = NULL;

    framePos.x = 108;
    framePos.y = 225 -30;

    memset(&te, 0, sizeof(TextEdition));
    te.blitStyle = TE_BLITSTYLE_BLENDED;
    msgPos.x = framePos.x +1 +10;
    msgPos.y = framePos.y +11 +10;
    msgPos.w = 423 -20;
    msgPos.h = 292 -20;

    titleSurf = TTF_RenderText_Blended(bigFont, title, titleColor);
    titlePos.x = (SCREENW - titleSurf->w) / 2;
    titlePos.y = msg ? msgPos.y - titleSurf->h - 20 : (SCREENH - titleSurf->h) / 2;

    if (msg)
    {
        snprintf(fullMsg, MAX_STRING-1, "Message #%d at T+%02d:%02d:%02d:\n\n%s\n\nPress <ESC> to quit.", msgNumber, hTime, mTime, sTime, msg);
        TE_NewTextEdition(&te, MAX_STRING, msgPos, smallFont, textColor,
                          TE_STYLE_MULTILINE | TE_STYLE_AUTOJUMP | TE_STYLE_VSCROLL | TE_STYLE_JUSTDISPLAY | TE_STYLE_BLITRGBA);
        TE_SetEditionText(&te, fullMsg);
    }

    if (!voiceFile || FMOD_OK != FMOD_System_CreateSound(mainFMODSystem, voiceFile, FMOD_SOFTWARE | FMOD_2D | FMOD_CREATESTREAM, 0, &voice))
        voice = NULL;
    else FMOD_System_PlaySound(mainFMODSystem, FMOD_CHANNEL_FREE, voice, 0, NULL);

    while (!done)
    {
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                done = 1;
            else if (msg)
                TE_HoldTextEdition(&te, event);
        }

        ManageDisplay();
        SDL_BlitSurface(frameSurf, NULL, screen, &framePos);
        if (msg)
            TE_DisplayTextEdition(&te);
        SDL_BlitSurface(titleSurf, NULL, screen, &titlePos);

        SDL_Flip(screen);
        Temporize(25);
    }


    if (voice)
        FMOD_Sound_Release(voice);
    TE_DeleteTextEdition(&te);
    SDL_FreeSurface(titleSurf);
    SDL_FreeSurface(frameSurf);
    TTF_CloseFont(bigFont);
    TTF_CloseFont(smallFont);

    msgNumber++;
}

int PrintGeneralTextBox(const char *title, char *text, int nbChar, const char *voiceFile)
{
    if (!title)
        return 0;

    SDL_Surface *bgSurf = NULL,
                *titleSurf = NULL,
                *titleBg = NULL,
                *screen = SDL_GetVideoSurface();
    TTF_Font *font = TTF_OpenFont("fonts\\gui\\neuropol.ttf", 15);
    SDL_Color whiteColor = {255, 255, 255};
    SDL_Event event;
    SDL_Rect editPos = {0},
             titlePos = {0},
             titleBgPos = {0};
    TextEdition te;
    FMOD_SOUND *voice = NULL;
    int done = 0;

    memset(&te, 0, sizeof(TextEdition));
    te.blitStyle = TE_BLITSTYLE_BLENDED;
    editPos.h = TTF_FontLineSkip(font);
    editPos.x = 50;
    editPos.w = SCREENW - editPos.x*2;
    editPos.y = (SCREENH - editPos.h) /2;
    bgSurf = SDL_CreateRGBSurface(SDL_HWSURFACE, editPos.w, editPos.h, 32,0,0,0,0);
    SDL_FillRect(bgSurf, NULL, SDL_MapRGB(bgSurf->format, 255,0,0));
    SDL_SetAlpha(bgSurf, SDL_SRCALPHA, 240);

    titleSurf = TTF_RenderText_Blended(font, title, whiteColor);
    titlePos.x = (SCREENW - titleSurf->w) / 2;
    titlePos.y = editPos.y - titleSurf->h - 10;

    titleBg = SDL_CreateRGBSurface(SDL_HWSURFACE, SCREENW, editPos.y+editPos.h - titlePos.y + 20, 32,0,0,0,0);
    titleBgPos.x = 0;
    titleBgPos.y = titlePos.y - 10;
    SDL_FillRect(titleBg, NULL, SDL_MapRGB(titleBg->format, 0,0,10));
    SDL_SetAlpha(titleBg, SDL_SRCALPHA, 200);

    TE_NewTextEdition(&te, nbChar-1, editPos, font, whiteColor, TE_STYLE_HSCROLL | TE_STYLE_BLITRGBA);
    if (text[0])
    {
        TE_SetEditionText(&te, text);
        TE_SetCursorPos(&te, strlen(text));
    }
    TE_SetFocusEdition(&te, 1);

    if (!voiceFile || FMOD_OK != FMOD_System_CreateSound(mainFMODSystem, voiceFile, FMOD_SOFTWARE | FMOD_2D | FMOD_CREATESTREAM, 0, &voice))
        voice = NULL;
    else FMOD_System_PlaySound(mainFMODSystem, FMOD_CHANNEL_FREE, voice, 0, NULL);

    while (!done)
    {
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                done = 1;
            else if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER))
                done = 2;
            else TE_HoldTextEdition(&te, event);
        }

        ManageDisplay();
        SDL_BlitSurface(bgSurf, NULL, screen, &editPos);
        SDL_BlitSurface(titleBg, NULL, screen, &titleBgPos);
        TE_DisplayTextEdition(&te);
        SDL_BlitSurface(titleSurf, NULL, screen, &titlePos);

        SDL_Flip(screen);
        Temporize(25);
    }

    if (done == 2)
        strncpy(text, te.text, nbChar-1);

    if (voice)
        FMOD_Sound_Release(voice);
    TE_DeleteTextEdition(&te);
    SDL_FreeSurface(titleSurf);
    SDL_FreeSurface(titleBg);
    SDL_FreeSurface(bgSurf);
    TTF_CloseFont(font);

    return done==2;
}





int BlitRGBA(SDL_Surface *srcSurf, SDL_Rect *srcRect0, SDL_Surface *dstSurf, SDL_Rect *dstRect0)
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
Uint32 GetPixel(SDL_Surface *surface, int x, int y)
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
void PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
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
