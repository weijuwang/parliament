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
    printf("%i\n", PARL_STACK_CONTAINS(g.faceDownCards, PARL_JOKER_CARD));
    return 0;
}
