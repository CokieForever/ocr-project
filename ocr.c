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

#include "ocr.h"

#define IsRectInRect(rIn,rExt)  ((rIn).x>=(rExt).x && (rIn).x+(rIn).w<=(rExt).x+(rExt).w && (rIn).y>=(rExt).y && (rIn).y+(rIn).h<=(rExt).y+(rExt).h)

static CharTab *tabImagesRef[MAX_FONTS] = {NULL};

CharTab Bichromize(SDL_Color **img, int width, int height)
{
    SDL_Color center1, center2;
    int done = 0,
        sum1r, sum1g, sum1b,
        sum2r, sum2g, sum2b,
        count1, count2,
        x, y;
    CharTab charTab = CreateCharTab(width, height, NULL);

    srand(time(NULL));
    center1 = img[0][0];
    do
    {
        center2 = img[rand()%width][rand()%height];
    } while (AreSameColor(center1, center2));

    while (!done)
    {
        SDL_Color oldCenter1 = center1,
                  oldCenter2 = center2;

        count1 = 0; count2 = 0;
        sum1r = 0; sum1g = 0; sum1b = 0;
        sum2r = 0; sum2g = 0; sum2b = 0;

        for (x=0 ; x < width ; x++)
        {
            for (y=0 ; y < height ; y++)
            {
                if (ColorDist(img[x][y], center1) < ColorDist(img[x][y], center2))
                {
                    charTab.tab[x][y] = 0;
                    sum1r += img[x][y].r;
                    sum1g += img[x][y].g;
                    sum1b += img[x][y].b;
                    count1++;
                }
                else
                {
                    charTab.tab[x][y] = 1;
                    sum2r += img[x][y].r;
                    sum2g += img[x][y].g;
                    sum2b += img[x][y].b;
                    count2++;
                }
            }
        }

        center1.r = sum1r / count1;
        center1.g = sum1g / count1;
        center1.b = sum1b / count1;

        center2.r = sum2r / count2;
        center2.g = sum2g / count2;
        center2.b = sum2b / count2;

        if (AreSameColor(center1, oldCenter1) && AreSameColor(center2, oldCenter2))
            done = 1;
    }

    return charTab;
}

CharTab BichromizeFast(SDL_Color **img, int width, int height)
{
    CharTab charTab = CreateCharTab(width, height, NULL);
    int x, y;
    SDL_Color blackColor = {0},
              whiteColor = {255, 255, 255};

    for (x=0 ; x < width ; x++)
    {
        for (y=0 ; y < height ; y++)
        {
            if (ColorDist(img[x][y], blackColor) < ColorDist(img[x][y], whiteColor))
                charTab.tab[x][y] = 0;
            else charTab.tab[x][y] = 1;
        }
    }

    return charTab;
}

double ColorDist(SDL_Color c1, SDL_Color c2)
{
    return (c1.r-c2.r)*(c1.r-c2.r) + (c1.g-c2.g)*(c1.g-c2.g) + (c1.b-c2.b)*(c1.b-c2.b);
}

SDL_Surface* SurfFromCharTab(CharTab charTab)
{
    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_HWSURFACE, charTab.width, charTab.height, 32, 0,0,0,0);
    int x, y;

    SDL_LockSurface(surf);
    for (x=0 ; x < charTab.width ; x++)
    {
        for (y=0 ; y < charTab.height ; y++)
        {
            if (charTab.tab[x][y])
                PutPixel(surf, x, y, SDL_MapRGB(surf->format, 255,255,255));
            else PutPixel(surf, x, y, SDL_MapRGB(surf->format, 0,0,0));
        }
    }
    SDL_UnlockSurface(surf);

    return surf;
}

SDL_Surface* WriteCharTab(CharTab charTab, TTF_Font *font, SDL_Color color)
{
    int x, y, c,
        advance, lineSkip,
        advanceMax = 0;
    char str[5] = "\0\0";
    SDL_Rect pos, realPos;
    SDL_Surface *surf, *surfLetter;

    for (c=0 ; c < 256 ; c++)
    {
        if (!TTF_GlyphMetrics(font, (Uint16)c, NULL, NULL, NULL, NULL, &advance) && advance > advanceMax)
            advanceMax = advance;
    }

    lineSkip = TTF_FontLineSkip(font);
    surf = SDL_CreateRGBSurface(SDL_HWSURFACE, advanceMax*charTab.width, lineSkip*charTab.height, 32, 0,0,0,0);
    SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 255, 255, 255));

    pos.x = 0;
    for (x=0 ; x < charTab.width ; x++)
    {
        pos.y = 0;
        for (y=0 ; y < charTab.height ; y++)
        {
            str[0] = charTab.tab[x][y];
            surfLetter = TTF_RenderText_Blended(font, str, color);
            realPos.x = pos.x + (advanceMax-surfLetter->w) /2.0;
            realPos.y = pos.y;
            SDL_BlitSurface(surfLetter, NULL, surf, &realPos);
            SDL_FreeSurface(surfLetter);
            pos.y += lineSkip;
        }
        pos.x += advanceMax;
    }

    return surf;
}

SDL_Rect* LocateLetters(CharTab charTab)
{
    int x, y, i=0, j, k=0, ok;
    SDL_Rect *rectTab = malloc(sizeof(SDL_Rect) * charTab.width*charTab.height),
             *rectTab2 = NULL;

    for (x=0 ; x<charTab.width ; x++)
    {
        for (y=0 ; y<charTab.height ; y++)
        {
            if (!charTab.tab[x][y])
            {
                rectTab[i] = GetZone(charTab, x, y);
                if (rectTab[i].w >= 5 && rectTab[i].h >= 5)
                    i++;
            }
        }
    }

    memset(&(rectTab[i]), 0, sizeof(SDL_Rect));
    rectTab2 = malloc(sizeof(SDL_Rect) * (i+1));

    for (i=0 ; rectTab[i].w > 0 ; i++)
    {
        ok = 1;
        for (j=0 ; rectTab[j].w > 0 ; j++)
        {
            if (i != j && IsRectInRect(rectTab[j], rectTab[i]))
                ok = 0;
        }

        if (ok)
        {
            rectTab2[k] = rectTab[i];
            k++;
        }
    }

    free(rectTab);
    rectTab = malloc(sizeof(SDL_Rect) * (k+1));
    memcpy(rectTab, rectTab2, sizeof(SDL_Rect) * (k+1));
    memset(&(rectTab[k]), 0, sizeof(SDL_Rect));
    free(rectTab2);

    for (x=0 ; x<charTab.width ; x++)
    {
        for (y=0 ; y<charTab.height ; y++)
        {
            if (charTab.tab[x][y] != 1)
                charTab.tab[x][y] = 0;
        }
    }

    return rectTab;
}

int LocateMainTab_CompFn (const void *elem1, const void *elem2)
{
    SDL_Rect *r1 = (SDL_Rect*)elem1,
             *r2 = (SDL_Rect*)elem2;

    if (r1->x < r2->x)
        return -1;
    else if (r1->x > r2->x)
        return 1;
    else return 0;
}
SDL_Rect** LocateMainTab(CharTab charTab, SDL_Rect *lettersTab, SDL_Rect *mainTabSize)
{
    int nbLines, i, j, k, l,
        i0, n, n2,
        mx, done, linesOK,
        width = 1, height = -1,
        *linesTab;
    SDL_Rect **linesRect = LocateLines(charTab, lettersTab, &nbLines),
             **mainTab = NULL;

    linesTab = malloc(sizeof(int) * nbLines);

    for (i=0 ; i<nbLines ; i++)
    {
        for (j=0 ; linesRect[i][j].w > 0 ; j++);
        qsort(linesRect[i], j, sizeof(SDL_Rect), LocateMainTab_CompFn);
    }

    for (n=0 ; n<nbLines ; n++)
    {
        for (i0 = 0 ; linesRect[n][i0].w > 0 ; i0++)
        {
            k=0;
            for (n2=n+1 ; n2<nbLines ; n2++)
            {
                done = 0;
                for (j=0 ; linesRect[n2][j].w > 0 && !done ; j++)
                {
                    mx = round (linesRect[n2][j].x + linesRect[n2][j].w/2.0);
                    if (mx >= linesRect[n][i0].x && mx <= linesRect[n][i0].x + linesRect[n][i0].w)
                    {
                        linesTab[k] = j*nbLines + n2;
                        k++;
                        done = 1;
                    }
                }
            }
            if (k < nbLines)
                linesTab[k] = -1;
            linesOK = k;

            done = 0;
            for (i=i0+1 ; !done && linesRect[n][i].w > 0 ; i++)
            {
                for (k=0 ; !done && k < nbLines && linesTab[k] >= 0 ; k++)
                {
                    if (linesTab[k] > 0)
                    {
                        n2 = linesTab[k] % nbLines;
                        j = linesTab[k] / nbLines + i-i0;

                        mx = round (linesRect[n2][j].x + linesRect[n2][j].w/2.0);
                        if (linesRect[n2][j].w <= 0 || mx < linesRect[n][i].x || mx > linesRect[n][i].x + linesRect[n][i].w)
                        {
                            if (linesOK > i-i0+1)
                            {
                                linesTab[k] = 0;
                                linesOK--;
                            }
                            else done = 1;
                        }
                    }
                }
            }

            if (done)
                i--;

            if ((i-i0) * (linesOK+1) > width * height)
            {
                if (mainTab)
                    FreeTab((char**)mainTab, width);
                width = i-i0;
                height = linesOK+1;
                mainTab = malloc(sizeof(SDL_Rect*) * width);

                for (i=0 ; i < width ; i++)
                {
                    mainTab[i] = malloc(sizeof(SDL_Rect) * height);
                    mainTab[i][0] = linesRect[n][i+i0];
                }

                j = 1;
                for (k=0 ; k < nbLines && linesTab[k] >= 0 ; k++)
                {
                    if (linesTab[k] > 0)
                    {
                        n2 = linesTab[k] % nbLines;
                        l = linesTab[k] / nbLines;
                        for (i=0 ; i < width ; i++)
                            mainTab[i][j] = linesRect[n2][l+i];
                        j++;
                    }
                }
            }
        }
    }

    mainTabSize->w = width;
    mainTabSize->h = height;
    FreeTab((char**)linesRect, nbLines);
    return mainTab;
}

int LocateLines_CompFn (const void *elem1, const void *elem2)
{
    SDL_Rect *r1 = (SDL_Rect*)elem1,
             *r2 = (SDL_Rect*)elem2;
    double c1 = r1->y + r1->h/2.0,
           c2 = r2->y + r2->h/2.0;

    if (c1 < c2)
        return -1;
    else if (c1 > c2)
        return 1;
    else if (r1->x < r2->x)
        return -1;
    else if (r1->x > r2->x)
        return 1;
    else return 0;
}
SDL_Rect** LocateLines(CharTab charTab, SDL_Rect *lettersTab, int *nbLines)
{
    int i, j, k, nb, lineNb=0,
        *headChars, line,
        yMin, yMax;
    SDL_Rect *lettersTabCp,
             **rectTab;
    float *linesTab, prob;

    for (nb=0 ; lettersTab[nb].w > 0 ; nb++);
    lettersTabCp = malloc(sizeof(SDL_Rect)*nb);
    memcpy(lettersTabCp, lettersTab, sizeof(SDL_Rect)*nb);
    qsort(lettersTabCp, nb, sizeof(SDL_Rect), LocateLines_CompFn);

    linesTab = malloc(sizeof(float) * nb);
    linesTab[0] = lineNb + 0.6;
    headChars = malloc(sizeof(int) * nb);
    headChars[0] = 0;
    yMin = lettersTabCp[0].y;
    yMax = yMin + lettersTabCp[0].h;
    for (i=1 ; i < nb ; i++)
    {
        if (lettersTabCp[i].y >= yMin && lettersTabCp[i].y < yMax)
        {
            linesTab[i] = lineNb + (yMax - lettersTabCp[i].y) / (2.0*lettersTabCp[i].h);
            if (linesTab[i] > lineNb+0.5)
                linesTab[i] = lineNb + 0.5;
        }
        else
        {
            lineNb++;
            linesTab[i] = lineNb + 0.6;
            headChars[lineNb] = i;
            yMin = lettersTabCp[i].y;
            yMax = yMin + lettersTabCp[i].h;
        }
    }
    *nbLines = lineNb+1;

    for (i=0 ; i < nb ; i++)
    {
        line = trunc(linesTab[i]);
        prob = (linesTab[i] - line) * 2;
        if (prob < 1 && line+1 < (*nbLines)
            && lettersTabCp[i].y + lettersTabCp[i].h >= lettersTabCp[headChars[line+1]].y
            && (lettersTabCp[i].y + lettersTabCp[i].h - lettersTabCp[headChars[line+1]].y) / lettersTabCp[i].h < prob)
            linesTab[i] = line+1;
        else linesTab[i] = line;
    }

    rectTab = malloc(sizeof(SDL_Rect*) * (*nbLines));
    for (i=0 ; i < (*nbLines) ; i++)
    {
        k=0;
        for (j=0 ; j < nb ; j++)
        {
            if (linesTab[j] == i)
                k++;
        }
        rectTab[i] = malloc(sizeof(SDL_Rect) * (k+1));

        k=0;
        for (j=0 ; j < nb ; j++)
        {
            if (linesTab[j] == i)
            {
                rectTab[i][k] = lettersTabCp[j];
                k++;
            }
        }

        memset(&(rectTab[i][k]), 0, sizeof(SDL_Rect));
    }

    free(headChars);
    free(lettersTabCp);
    free(linesTab);
    return rectTab;
}


SDL_Rect GetZone(CharTab charTab, int x, int y)
{
    int x0 = x, y0 = y,
        yMin = y0, yMax = y0,
        xMin = x0, xMax = x0,
        done = 0;
    SDL_Rect rect;

    charTab.tab[x0][y0] = 6;

    while (!done)
    {
        if (y-1 > 0 && !charTab.tab[x][y-1])
        {
            y--;
            charTab.tab[x][y] = 2;
            if (y < yMin)
                yMin = y;
        }
        else if (x+1 < charTab.width && !charTab.tab[x+1][y])
        {
            x++;
            charTab.tab[x][y] = 3;
            if (x > xMax)
                xMax = x;
        }
        else if (y+1 < charTab.height && !charTab.tab[x][y+1])
        {
            y++;
            charTab.tab[x][y] = 4;
            if (y > yMax)
                yMax = y;
        }
        else if (x-1 > 0 && !charTab.tab[x-1][y])
        {
            x--;
            charTab.tab[x][y] = 5;
            if (x < xMin)
                xMin = x;
        }
        else if (x+1 < charTab.width && y+1 < charTab.height && !charTab.tab[x+1][y+1])
        {
            x++; y++;
            charTab.tab[x][y] = 6;
            if (x > xMax)
                xMax = x;
            if (y > yMax)
                yMax = y;
        }
        else if (x-1 > 0 && y+1 < charTab.height && !charTab.tab[x-1][y+1])
        {
            x--; y++;
            charTab.tab[x][y] = 7;
            if (x < xMin)
                xMin = x;
            if (y > yMax)
                yMax = y;
        }
        else if (x-1 > 0 && y-1 > 0 && !charTab.tab[x-1][y-1])
        {
            x--; y--;
            charTab.tab[x][y] = 8;
            if (x < xMin)
                xMin = x;
            if (y < yMin)
                yMin = y;
        }
        else if (x+1 < charTab.width && y-1 > 0 && !charTab.tab[x+1][y-1])
        {
            x++; y--;
            charTab.tab[x][y] = 9;
            if (x > xMax)
                xMax = x;
            if (y < yMin)
                yMin = y;
        }
        else
        {
            switch(charTab.tab[x][y])
            {
                case 2:
                    y++;
                    break;
                case 3:
                    x--;
                    break;
                case 4:
                    y--;
                    break;
                case 5:
                    x++;
                    break;
                case 6:
                    x--; y--;
                    break;
                case 7:
                    x++; y--;
                    break;
                case 8:
                    x++; y++;
                    break;
                case 9:
                    x--; y++;
                    break;
                default:
                    done = 1;
                    break;
            }

            if (x==x0 && y==y0)
                done = 1;
        }
    }

    rect.x = xMin; rect.y = yMin;
    rect.w = xMax - xMin +1; rect.h = yMax - yMin +1;

    return rect;
}

char RecognizeLetter(CharTab charTab0, SDL_Rect zone, double *rate, int print)
{
    CharTab charTab = ExtractCharTab(charTab0, zone),
            imgRef, cpCharTab;
    int i, l,
        newHeight, newWidth;
    SDL_Rect pos;
    double tabRates[MAX_FONTS][256], maxRate;
    char letter[MAX_FONTS] = {'\0'}, bestLetter;

    for (i=0 ; i < MAX_FONTS ; i++)
    {
        for (l=0 ; l<256 ; l++)
            tabRates[i][l] = -1;
    }

    //Rates calculation
    for (i=0 ; i < MAX_FONTS && tabImagesRef[i] ; i++)
    {
        for (l='A' ; l <= 'z' ; l = (l=='Z' ? 'a' : l+1))
        {
            imgRef = tabImagesRef[i][(int)l];
            newHeight = imgRef.height;
            newWidth = charTab.width * newHeight / charTab.height;
            cpCharTab = DeformCharTab(charTab, newWidth, newHeight);

            if (cpCharTab.width < imgRef.width)
            {
                CharTab cpCharTab2;

                pos.x = round((cpCharTab.width-imgRef.width) / 2.0);
                pos.y = 0;
                pos.w = imgRef.width;
                pos.h = cpCharTab.height;
                cpCharTab2 = ExtractCharTab(cpCharTab, pos);

                tabRates[i][l] = (CorrelationFunction(cpCharTab2, imgRef)+1) /2;
                FreeTab(cpCharTab2.tab, cpCharTab2.width);
            }
            else
            {
                CharTab cpImgRef;

                pos.x = round((imgRef.width-cpCharTab.width) / 2.0);
                pos.y = 0;
                pos.w = cpCharTab.width;
                pos.h = imgRef.height;
                cpImgRef = ExtractCharTab(imgRef, pos);

                tabRates[i][l] = (CorrelationFunction(cpCharTab, cpImgRef)+1) /2;
                FreeTab(cpImgRef.tab, cpImgRef.width);
            }

            FreeTab(cpCharTab.tab, cpCharTab.width);
        }
    }

    //Search for best letter of each font
    for (i=0 ; i < MAX_FONTS && tabImagesRef[i] ; i++)
    {
        maxRate = tabRates[i][0];
        letter[i] = 0;
        for (l=1 ; l<256 ; l++)
        {
            if (tabRates[i][l] > maxRate)
            {
                letter[i] = l;
                maxRate = tabRates[i][l];
            }
        }

        if (print)
            printf("Font %d: Best letter is '%c' with a rate of %.1f%%\n", i, letter[i], maxRate*100);
    }

    //Search for the best letter
    maxRate = tabRates[0][(int)letter[0]];
    bestLetter = letter[0];
    if (rate)
        *rate = tabRates[0][(int)letter[0]];
    for (i=1 ; i < MAX_FONTS && tabImagesRef[i] ; i++)
    {
        if (tabRates[i][(int)letter[i]] > maxRate)
        {
            maxRate = tabRates[i][(int)letter[i]];
            bestLetter = letter[i];

            if (rate)
                *rate = tabRates[i][(int)letter[i]];
        }
    }
    if (print)
        printf("The best letter is '%c'.\n", bestLetter);

    FreeTab(charTab.tab, charTab.width);
    return bestLetter;
}

double GetX1Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMin)
{
    double r, grad = 0;
    CharTab cpCharTab;

    if (pos.x < 0)
    {
        pos.x++;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(cpCharTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        grad = r > rate0 ? r - rate0 : 0;
        pos.x--;
    }

    if (pos.x > xMin)
    {
        pos.x--;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(cpCharTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        if (r > rate0 && (r - rate0) > grad)
            grad = rate0 - r;
        pos.x++;
    }

    return grad;
}

double GetY1Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int yMin)
{
    double r, grad = 0;
    CharTab cpCharTab;

    if (pos.y < 0)
    {
        pos.y++;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        grad = r > rate0 ? r - rate0 : 0;
        pos.y--;
    }

    if (pos.y > yMin)
    {
        pos.y--;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        if (r > rate0 && (r - rate0) > grad)
            grad = rate0 - r;
        pos.y++;
    }

    return grad;
}

double GetX2Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int xMax)
{
    double r, grad = 0;
    CharTab cpCharTab;
    int x2 = pos.x + pos.w - charTab.width;

    if (x2 < xMax)
    {
        pos.w++;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        grad = r > rate0 ? r - rate0 : 0;
        pos.w--;
    }

    if (x2 > 0)
    {
        pos.w--;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        if (r > rate0 && (r - rate0) > grad)
            grad = rate0 - r;
        pos.w++;
    }

    return grad;
}

double GetY2Grad(SDL_Rect pos, double rate0, CharTab charTab, CharTab imageRef, int yMax)
{
    double r, grad = 0;
    CharTab cpCharTab;
    int y2 = pos.y + pos.h - charTab.height;

    if (y2 < yMax)
    {
        pos.h++;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        grad = r > rate0 ? r - rate0 : 0;
        pos.h--;
    }

    if (y2 > 0)
    {
        pos.h--;
        cpCharTab = ExtractCharTab(charTab, pos);
        r = CorrelationFunction(charTab, imageRef);
        FreeTab(cpCharTab.tab, cpCharTab.width);
        if (r > rate0 && (r - rate0) > grad)
            grad = rate0 - r;
        pos.h++;
    }

    return grad;
}

double CorrelationFunction(CharTab charTab1, CharTab charTab2)
{
    CharTab charTab1b = DeformCharTab(charTab1, charTab2.width, charTab2.height);
    int x, y;
    double sum = 0;

    for (x=0 ; x < charTab2.width ; x++)
    {
        for (y=0 ; y < charTab2.height ; y++)
        {
            if (charTab1b.tab[x][y] == charTab2.tab[x][y])
                sum += charTab1b.tab[x][y] ? 0.5 : 1;
            else sum -= 1;
        }
    }

    FreeTab(charTab1b.tab, charTab1b.width);
    return sum / (charTab2.width*charTab2.height);
}

CharTab RecognizeMainTab(CharTab charTab, SDL_Rect** mainTab, int width, int height, double *meanRate)
{
    int x, y;
    double rate, sumRate=0;
    CharTab letters = CreateCharTab(width, height, NULL);

    SetWorkingPercent(0);
    for (x=0 ; x < width ; x++)
    {
        for (y=0 ; y < height ; y++)
        {
            letters.tab[x][y] = RecognizeLetter(charTab, mainTab[x][y], &rate, 0);
            sumRate += rate;
            SetWorkingPercent((x*height+y+1)*1.0 / (width*height));
        }
    }
    SetWorkingPercent(1);

    if (meanRate)
        *meanRate = sumRate / (width*height);
    return letters;
}

int FullRecognition(SDL_Surface *img,
                    SDL_Color ***colorTab0, CharTab *charTab0, SDL_Surface **img2_0, SDL_Rect **rectTab0, SDL_Rect ***mainTab0,
                    SDL_Rect *mainTabSize0, CharTab ***tabImgR_0, CharTab *lettersTab0, double *recognitionRate0)
{
    SDL_Color **colorTab;
    CharTab charTab, lettersTab,
            charTabInverted, lettersTab2,
            **tabImgR;
    SDL_Surface *img2, *img2b;
    SDL_Rect *rectTab, **mainTab, mainTabSize,
             *rectTab2, **mainTab2, mainTabSize2;
    double recognitionRate,
           recognitionRate2;
    int n;

    printf("Image bichromization...\n");
    colorTab = GetAllPixels(img);
    charTab = Bichromize(colorTab, img->w, img->h);
    img2 = SurfFromCharTab(charTab);
    charTabInverted = InverseCharTab(charTab);
    img2b = SurfFromCharTab(charTabInverted);

    printf("Letters localization...\n");
    rectTab = LocateLetters(charTab);
    rectTab2 = LocateLetters(charTabInverted);

    printf("Main tab localization...\n");
    mainTab = LocateMainTab(charTab, rectTab, &mainTabSize);
    mainTab2 = LocateMainTab(charTabInverted, rectTab2, &mainTabSize2);

    printf("Creation of reference images...\n");
    tabImgR = CreateImagesRef();

    printf("1st letters recognition.\n");
    lettersTab = RecognizeMainTab(charTab, mainTab, mainTabSize.w, mainTabSize.h, &recognitionRate);
    if ( (n = lettersTab.width * lettersTab.height) < 20)
        recognitionRate *= n / 20.0;
    printf("2nd letters recognition.\n");
    lettersTab2 = RecognizeMainTab(charTabInverted, mainTab2, mainTabSize2.w, mainTabSize2.h, &recognitionRate2);
    if ( (n = lettersTab2.width * lettersTab2.height) < 20)
        recognitionRate2 *= n / 20.0;
    printf("Rate for tab 1: %.1f%%\nRate for tab 2: %.1f%%\n", recognitionRate*100, recognitionRate2*100);

    if (recognitionRate >= recognitionRate2)
        printf("So, we keep tab 1.\n");
    else
    {
        ExchangeVars(&lettersTab, &lettersTab2, sizeof(CharTab));
        ExchangeVars(&charTab, &charTabInverted, sizeof(CharTab));
        ExchangeVars(&mainTab, &mainTab2, sizeof(SDL_Rect**));
        ExchangeVars(&mainTabSize, &mainTabSize2, sizeof(SDL_Rect));
        ExchangeVars(&rectTab, &rectTab2, sizeof(SDL_Rect*));
        ExchangeVars(&img2, &img2b, sizeof(SDL_Surface*));
        printf("So, we keep tab 2.\n");
    }

    FreeTab(charTabInverted.tab, charTabInverted.width);
    FreeTab((char**)mainTab2, mainTabSize2.w);
    FreeTab(lettersTab2.tab, lettersTab2.width);
    SDL_FreeSurface(img2b);
    free(rectTab2);

    if (colorTab0)
        *colorTab0 = colorTab;
    else FreeTab((char**)colorTab, img->w);
    if (charTab0)
        *charTab0 = charTab;
    else FreeTab(charTab.tab, charTab.width);
    if (img2_0)
        *img2_0 = img2;
    else SDL_FreeSurface(img2);
    if (rectTab0)
        *rectTab0 = rectTab;
    else free(rectTab);
    if (mainTab0)
        *mainTab0 = mainTab;
    else FreeTab((char**)mainTab, mainTabSize.w);
    if (mainTabSize0)
        *mainTabSize0 = mainTabSize;
    if (tabImgR_0)
        *tabImgR_0 = tabImgR;
    if (lettersTab0)
        *lettersTab0 = lettersTab;
    else FreeTab(lettersTab.tab, lettersTab.width);
    if (recognitionRate0)
        *recognitionRate0 = recognitionRate;

    return 1;
}

CharTab** CreateImagesRef(void)
{
    SDL_Color blackColor = {0},
              whiteColor = {255, 255, 255};
    char str[10] = "\0\0", fileName[MAX_PATH];
    SDL_Surface *surf;
    TTF_Font *font;
    SDL_Color **colorTab;
    int i, j=0;
    CharTab ct;
    struct dirent *lecture;
    DIR *rep;

    rep = opendir("fonts\\analysis");
    while ((lecture = readdir(rep)) && j < MAX_FONTS)
    {
        sprintf(fileName, "Fonts\\analysis\\%s", lecture->d_name);
        if ( (font = TTF_OpenFont(fileName, 75)) )
        {
            printf("Font %d: %s\n", j, lecture->d_name);
            tabImagesRef[j] = malloc(sizeof(CharTab) * 256);
            for (i=0 ; i < 256 ; i++)
                tabImagesRef[j][i].tab = NULL;

            for (i='A' ; i <= 'z' ; i = (i == 'Z' ? 'a' : i+1))
            {
                str[0] = (char)i;
                surf = TTF_RenderText_Shaded(font, str, blackColor, whiteColor);
                colorTab = GetAllPixels(surf);
                ct = BichromizeFast(colorTab, surf->w, surf->h);
                tabImagesRef[j][i] = RecutCharTab(ct);

                FreeTab((char**)colorTab, surf->w);
                FreeTab(ct.tab, ct.width);
                SDL_FreeSurface(surf);
            }

            TTF_CloseFont(font);
            j++;
        }
    }
    closedir(rep);

    return tabImagesRef;
}

void FreeImagesRef(void)
{
    int i,j;

    for (j=0 ; j < MAX_FONTS && tabImagesRef[j] ; j++)
    {
        for (i=0 ; i < 256 ; i++)
        {
            if (tabImagesRef[j][i].tab)
                FreeTab(tabImagesRef[j][i].tab, tabImagesRef[j][i].width);
        }

        free(tabImagesRef[j]);
        tabImagesRef[j] = NULL;
    }
}


CharTab CreateCharTab(int width, int height, char* initValue)
{
    CharTab charTab;
    int x, y;

    charTab.tab = malloc(sizeof(char*) * width);
    for (x=0 ; x < width ; x++)
    {
        charTab.tab[x] = malloc(sizeof(char) * height);
        for (y=0 ; initValue && y < height ; y++)
            charTab.tab[x][y] = *initValue;
    }

    charTab.width = width;
    charTab.height = height;
    return charTab;
}

CharTab ExtractCharTab(CharTab charTab, SDL_Rect pos)
{
    int x, y;
    CharTab charTab2 = CreateCharTab(pos.w, pos.h, NULL);

    for (x=0 ; x < pos.w ; x++)
    {
        for (y=0 ; y < pos.h ; y++)
        {
            if (x+pos.x >= 0 && x+pos.x < charTab.width && y+pos.y >= 0 && y+pos.y < charTab.height)
                charTab2.tab[x][y] = charTab.tab[x+pos.x][y+pos.y];
            else charTab2.tab[x][y] = 1;
        }
    }

    return charTab2;
}

CharTab DeformCharTab(CharTab charTab, int width, int height)
{
    int x, y,
        width0 = charTab.width,
        height0 = charTab.height;
    CharTab charTabFinal,
            charTab2;

    if (width0 < width)
    {
        width0 *= 2;
        charTab2 = CreateCharTab(width0, height0, NULL);
        for (x=0 ; x < width0 ; x++)
        {
            for (y=0 ; y < height0 ; y++)
                charTab2.tab[x][y] = charTab.tab[x/2][y];
        }
    }

    else if (height0 < height)
    {
        height0 *= 2;
        charTab2 = CreateCharTab(width0, height0, NULL);
        for (x=0 ; x < width0 ; x++)
        {
            for (y=0 ; y < height0 ; y++)
                charTab2.tab[x][y] = charTab.tab[x][y/2];
        }
    }

    else if (width0 > width)
    {
        int nbCol = width0 - width,
            x2 = 0, col, i;
        double coef = width0 / (nbCol + 1.0);

        x = -1;
        charTab2 = CreateCharTab(width, height0, NULL);

        for (i=1 ; i <= nbCol+1 ; i++)
        {
            col = round(i*coef);
            for (x++ ; x < col ; x++)
            {
                for (y=0 ; y < height0 ; y++)
                    charTab2.tab[x2][y] = charTab.tab[x][y];
                x2++;
            }
        }
    }

    else if (height0 > height)
    {
        int nbRows = height0 - height,
            y2, row, i;
        double coef = height0 / (nbRows + 1.0);

        charTab2 = CreateCharTab(width0, height, NULL);

        for (x=0 ; x < width0 ; x++)
        {
            y = -1; y2 = 0;
            for (i=1 ; i <= nbRows+1 ; i++)
            {
                row = round(i*coef);
                for (y++ ; y < row ; y++)
                {
                    charTab2.tab[x][y2] = charTab.tab[x][y];
                    y2++;
                }
            }
        }
    }

    else return CopyCharTab(charTab);

    charTabFinal = DeformCharTab(charTab2, width, height);
    FreeTab(charTab2.tab, charTab2.width);
    return charTabFinal;
}

CharTab InverseCharTab(CharTab charTab)
{
    int x,y;
    CharTab charTab2 = CreateCharTab(charTab.width, charTab.height, NULL);

    for (x=0 ; x < charTab.width ; x++)
    {
        for (y=0 ; y < charTab.height ; y++)
            charTab2.tab[x][y] = !charTab.tab[x][y];
    }

    return charTab2;
}

CharTab CopyCharTab(CharTab charTab)
{
    int x,y;
    CharTab charTab2 = CreateCharTab(charTab.width, charTab.height, NULL);

    for (x=0 ; x < charTab.width ; x++)
    {
        for (y=0 ; y < charTab.height ; y++)
            charTab2.tab[x][y] = charTab.tab[x][y];
    }

    return charTab2;
}

CharTab MergeCharTabs(CharTab charTab1, CharTab charTab2)
{
    int x, y, elmt1, elmt2,
        width = charTab1.width > charTab2.width ? charTab1.width : charTab2.width,
        height = charTab1.height > charTab2.height ? charTab1.height : charTab2.height;
    CharTab charTab = CreateCharTab(width, height, NULL);

    for (x=0 ; x < width ; x++)
    {
        for (y=0 ; y < height ; y++)
        {
            elmt1 = (x<charTab1.width && y<charTab1.height) ? charTab1.tab[x][y] : 1;
            elmt2 = (x<charTab2.width && y<charTab2.height) ? charTab2.tab[x][y] : 1;
            charTab.tab[x][y] = (elmt1 == elmt2);
        }
    }

    return charTab;
}

CharTab RecutCharTab(CharTab charTab)
{
    int x, y,
        xMax = 0, yMax = 0,
        xMin = charTab.width, yMin = charTab.height;
    SDL_Rect rect;

    for (x=0 ; x < charTab.width ; x++)
    {
        for (y=0 ; y < charTab.height ; y++)
        {
            if (!charTab.tab[x][y])
            {
                if (x > xMax)
                    xMax = x;
                if (x < xMin)
                    xMin = x;
                if (y > yMax)
                    yMax = y;
                if (y < yMin)
                    yMin = y;
            }
        }
    }

    rect.x = xMin;
    rect.y = yMin;
    rect.w = xMax - xMin;
    rect.h = yMax - yMin;

    return ExtractCharTab(charTab, rect);
}

CharTab ThinDownCharTab(CharTab charTab)
{
    int x, y, done = 0, k=0, nb;
    char px[9];
    CharTab cpCharTab;
    SDL_Rect rect;

    rect.x = -1; rect.y = -1;
    rect.w = charTab.width+2;
    rect.h = charTab.height+2;
    cpCharTab = ExtractCharTab(charTab, rect);


    while (!done)
    {
        done=1;

        //Perimeter establishement
        for (x=1 ; x < cpCharTab.width-1 ; x++)
        {
            for (y=1 ; y < cpCharTab.height-1 ; y++)
            {
                if (cpCharTab.tab[x][y] != 1)
                {
                    px[1] = cpCharTab.tab[x][y-1]!=1;
                    px[3] = cpCharTab.tab[x+1][y]!=1;
                    px[5] = cpCharTab.tab[x][y+1]!=1;
                    px[7] = cpCharTab.tab[x-1][y]!=1;

                    nb = 4 - (px[1] + px[3] + px[5] + px[7]);
                    if (nb >= 1)
                        cpCharTab.tab[x][y] = 2;
                }
            }
        }

        //Rule #1
        for (x=1 ; x < cpCharTab.width-1 ; x++)
        {
            for (y=1 ; y < cpCharTab.height-1 ; y++)
            {
                if (cpCharTab.tab[x][y] == 2)
                {
                    px[1] = cpCharTab.tab[x][y-1];
                    px[3] = cpCharTab.tab[x+1][y];
                    px[5] = cpCharTab.tab[x][y+1];
                    px[7] = cpCharTab.tab[x-1][y];
                    px[2] = cpCharTab.tab[x+1][y-1];
                    px[4] = cpCharTab.tab[x+1][y+1];
                    px[6] = cpCharTab.tab[x-1][y+1];
                    px[8] = cpCharTab.tab[x-1][y-1];

                    nb = (px[1]==2) + (px[3]==2) + (px[5]==2) + (px[7]==2);
                    if (nb < 2)
                    {
                        if ((px[2] == 2 && px[1] == 0 && px[3] != 2) || (px[8] == 2 && px[1] == 0 && px[7] != 2))
                            cpCharTab.tab[x][y-1] = 2;
                        if ((px[2] == 2 && px[3] == 0 && px[1] != 2) || (px[4] == 2 && px[3] == 0 && px[5] != 2))
                            cpCharTab.tab[x+1][y] = 2;
                        if ((px[4] == 2 && px[5] == 0 && px[3] != 2) || (px[6] == 2 && px[5] == 0 && px[7] != 2))
                            cpCharTab.tab[x][y+1] = 2;
                        if ((px[6] == 2 && px[7] == 0 && px[5] != 2) || (px[8] == 2 && px[7] == 0 && px[1] != 2))
                            cpCharTab.tab[x-1][y] = 2;
                    }
                }
            }
        }

        //Rule #2
        for (x=1 ; x < cpCharTab.width-1 ; x++)
        {
            for (y=1 ; y < cpCharTab.height-1 ; y++)
            {

                if (cpCharTab.tab[x][y] == 2)
                {
                    px[1] = cpCharTab.tab[x][y-1]!=1;
                    px[3] = cpCharTab.tab[x+1][y]!=1;
                    px[5] = cpCharTab.tab[x][y+1]!=1;
                    px[7] = cpCharTab.tab[x-1][y]!=1;
                    px[2] = cpCharTab.tab[x+1][y-1]!=1;
                    px[4] = cpCharTab.tab[x+1][y+1]!=1;
                    px[6] = cpCharTab.tab[x-1][y+1]!=1;
                    px[8] = cpCharTab.tab[x-1][y-1]!=1;

                    nb = 4 - (px[1] + px[3] + px[5] + px[7]);
                    if (nb == 3 || (nb == 2 && (px[1] != px[3] && px[1] != px[7])))
                        cpCharTab.tab[x][y] = 3;
                    else if (nb == 2)
                    {
                        if (   (!px[1] && !px[3] && cpCharTab.tab[x-1][y+1]>=1 && cpCharTab.tab[x-1][y+1]<3)
                            || (!px[3] && !px[5] && cpCharTab.tab[x-1][y-1]>=1 && cpCharTab.tab[x-1][y-1]<3)
                            || (!px[5] && !px[7] && cpCharTab.tab[x+1][y-1]>=1 && cpCharTab.tab[x+1][y-1]<3)
                            || (!px[7] && !px[1] && cpCharTab.tab[x+1][y+1]>=1 && cpCharTab.tab[x+1][y+1]<3) )
                            cpCharTab.tab[x][y] = 3;
                    }
                }
            }
        }

        //Rule #3
        for (x=1 ; x < cpCharTab.width-1 ; x++)
        {
            for (y=1 ; y < cpCharTab.height-1 ; y++)
            {
                if (cpCharTab.tab[x][y] == 2)
                {
                    px[1] = cpCharTab.tab[x][y-1];
                    px[3] = cpCharTab.tab[x+1][y];
                    px[5] = cpCharTab.tab[x][y+1];
                    px[7] = cpCharTab.tab[x-1][y];
                    px[2] = cpCharTab.tab[x+1][y-1];
                    px[4] = cpCharTab.tab[x+1][y+1];
                    px[6] = cpCharTab.tab[x-1][y+1];
                    px[8] = cpCharTab.tab[x-1][y-1];

                    nb = (px[1]>=2) + (px[3]>=2) + (px[5]>=2) + (px[7]>=2);
                    if (   (nb > 2)
                        || (px[2]>=2 && px[1]<2 && px[3]<2) || (px[4]>=2 && px[3]<2 && px[5]<2)
                        || (px[6]>=2 && px[5]<2 && px[7]<2) || (px[8]>=2 && px[7]<2 && px[1]<2) )
                        cpCharTab.tab[x][y] = 3;
                }
            }
        }

        //Perimeter validation and deletion
        for (x=0 ; x < cpCharTab.width ; x++)
        {
            for (y=0 ; y < cpCharTab.height ; y++)
            {
                if (cpCharTab.tab[x][y] == 2)
                {
                    done = 0;
                    cpCharTab.tab[x][y] = 1;
                }
                else if (cpCharTab.tab[x][y] == 3)
                    cpCharTab.tab[x][y] = 0;
            }
        }

        if (k++ >= 11 && 0)
            done = 1;
    }

    return cpCharTab;
}
