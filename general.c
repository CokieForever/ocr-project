#include "general.h"

#ifdef SDL_UpperBlit
#undef SDL_UpperBlit
#endif

int BlitSurfaceSecured(SDL_Surface *s1, SDL_Rect *r1, SDL_Surface *s2, SDL_Rect *r2)
{
    return s1 && s2 ? SDL_BlitSurface(s1,r1,s2,r2) : -1;
}
