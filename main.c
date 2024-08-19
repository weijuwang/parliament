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
    printf("%i\n", parlGame_legalActions(&g));
    parlGame_free(&g);
    return 0;
}
