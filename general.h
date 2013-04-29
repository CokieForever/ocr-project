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

#ifndef GENERALH

#define GENERALH

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <SDL.h>
#include <SDL_image.h>
#include "ttf_secure.h"

#define MAX_STRING 2000


#if SDL_BYTEORDER == SDL_BIG_ENDIAN

#define RED_MASK   0xFF000000
#define GREEN_MASK 0x00FF0000
#define BLUE_MASK  0x0000FF00
#define ALPHA_MASK 0x000000FF

#else

#define RED_MASK   0x000000FF
#define GREEN_MASK 0x0000FF00
#define BLUE_MASK  0x00FF0000
#define ALPHA_MASK 0xFF000000

#endif

#define printf(...)   do { char txdfg[MAX_STRING]; snprintf(txdfg, MAX_STRING-1, __VA_ARGS__); PrintInConsole(txdfg); } while(0)

#endif
