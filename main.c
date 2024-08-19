#include <stdio.h>
#include "game.h"

int main(void)
{
    ParlGame g, h;
    parlGame_init(&g,
                  2,
                  3,
                  1,
                  parlCardIdx("3h")
                  );
    parlGame_deepCopy(&h, &g);
    printf("%i\n", parlGame_legalActions(&h));
    parlGame_free(&g);
    parlGame_free(&h);
    return 0;
}
