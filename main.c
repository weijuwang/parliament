#include <stdio.h>
#include "game.h"

int main(void)
{
    ParlGame g;
    parlGame_init(&g,
                  2,
                  3,
                  1,
                  parlCardIdx("3h")
                  );
    printf("%i\n", parlStackSize(g.faceDownCards));
    return 0;
}
