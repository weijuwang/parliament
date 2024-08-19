#include <stdio.h>
#include "game.h"

void printParlStack(const ParlStack s)
{
    ParlCardSymbol symbol;
    PARL_FOREACH_IN_STACK(s, i)
    {
        parlCardSymbol(symbol, i);
        printf("%c%c ", symbol[0], symbol[1]);
    }

    const int numJokers = PARL_NUM_JOKERS(s);
    if(numJokers)
        printf("%i jokers", numJokers);
}

int main(void)
{
    ParlGame g, h;
    parlGame_init(&g,
                  2,
                  3,
                  1,
                  parlSymbolToIdx("3h")
                  );
    parlGame_deepCopy(&h, &g);
    printf("%i\n", parlGame_legalActions(&h));
    printParlStack(g.faceDownCards);
    parlGame_free(&g);
    parlGame_free(&h);
    return 0;
}
