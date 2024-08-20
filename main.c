#include <stdio.h>
#include "game.h"

void printParlStack(const ParlStack s)
{
    ParlCardSymbol symbol;
    putchar('[');
    PARL_FOREACH_IN_STACK(s, i)
    {
        parlCardSymbol(symbol, i);
        printf(
            "%.*s",
            PARL_SYMBOL_WIDTH,
            symbol
        );
        if(i < PARL_NUM_NON_JOKER_CARDS - 1)
            putchar(' ');
    }

    const int numJokers = PARL_NUM_JOKERS(s);
    if(numJokers)
        printf(" %i jokers", numJokers);

    putchar(']');
}

void printParlGame(const ParlGame* const g)
{
    printf("parl");
    printParlStack(g->parliament);
    printf(" disc");
    printParlStack(g->discard);
    puts("");

    if(g->pmPosition == PARL_NO_PM)
        printf("pm~");
    else
    {
        ParlCardSymbol pmSymbol;
        parlCardSymbol(pmSymbol, g->pmCardIdx);
        printf(
            "pm=%.*s cab",
            PARL_SYMBOL_WIDTH,
            pmSymbol
        );
        printParlStack(g->cabinet);
    }

    printf("\nhands ");
    for(ParlPlayer p = 0; p < g->numPlayers; ++p)
    {
        printf("%i ", g->handSizes[p]);
    }

    putchar('\n');

    switch(g->mode)
    {
        case NORMAL:
            break;
        case DISCARD_AFTER_DRAW:
            printf("DISC ");
            break;
        case IMPEACH:
            printf("IMP ");
            break;
        case ELECTION:
            printf("ELEC ");
            break;
        case BACKUP_PM:
            printf("NEW_PM ");
            break;
    }

    printf("%i", g->turn);
    if(g->mode != NORMAL && g->mode != DISCARD_AFTER_DRAW)
        printf("->%i", g->nextNormalTurn);
    printf("/%i %i *%i", g->numPlayers, g->myPosition, g->drawDeckSize);
    printParlStack(g->knownHands[g->myPosition]);
    puts(" >>>");
}

int main(void)
{
    ParlGame g;
    parlGame_init(
        &g,
        2,
        3,
        1,
        parlSymbolToIdx("3h")
    );
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    printParlGame(&g);
    printParlStack(g.faceDownCards);
    parlGame_free(&g);
    return 0;
}
