#include <stdio.h>

#include "timer.h"
#include "game.h"

void printParlStack(const ParlStack s)
{
    ParlCardSymbol symbol;
    bool first = true;
    putchar('[');
    PARL_FOREACH_IN_STACK(s, i)
    {
        if(!first)
            putchar(' ');
        parlCardSymbol(symbol, i);
        printf(
            "%.*s",
            PARL_SYMBOL_WIDTH,
            symbol
        );
        first = false;
    }

    const int numJokers = PARL_NUM_JOKERS(s);
    if(numJokers)
        printf(" %i jokers", numJokers);

    putchar(']');
}

void printParlGame(const ParlGame* const g)
{
    ParlCardSymbol symbol;
    printf("parl");
    printParlStack(g->parliament);
    printf(" disc");
    printParlStack(g->discard);
    puts("");

    for(ParlPlayer p = 0; p < g->numPlayers; ++p)
    {
        printf(" %i", g->handSizes[p]);
    }

    printf("; pm");

    if(g->pmPosition == PARL_NO_PM)
        printf("~");
    else
    {
        parlCardSymbol(symbol, g->pmCardIdx);
        printf(
            "=%i %.*s ",
            g->pmPosition,
            PARL_SYMBOL_WIDTH,
            symbol
        );
        printParlStack(g->cabinet);
    }

    printf("\n(");

    switch(g->mode)
    {
        case NORMAL_MODE:
            break;
        case DISCARD_AFTER_DRAW_MODE:
            printf("DISC ");
            break;
        case REIMPEACH_MODE:
        case BLOCK_IMPEACH_MODE:
            parlCardSymbol(symbol, g->impeachedMpIdx);
            printf(
                "%s-IMP %.*s->",
                g->mode == REIMPEACH_MODE ? "RE" : "BLOCK",
                PARL_SYMBOL_WIDTH,
                symbol
            );
            parlCardSymbol(symbol, g->cardToBeatIdx);
            printf(
                "%.*s? ",
                PARL_SYMBOL_WIDTH,
                symbol
            );
            break;
        case ELECTION_MODE:
            printf("ELEC? ");
            break;
        case BACKUP_PM_MODE:
            printf("NEW_PM ");
            break;
    }

    printf("%i)", g->turn);
    if(g->mode != NORMAL_MODE && g->mode != DISCARD_AFTER_DRAW_MODE)
        printf("->%i", g->nextNormalTurn);
    printf("/%i *%i %i", g->numPlayers, g->drawDeckSize, g->myPosition);
    printParlStack(g->knownHands[g->myPosition]);
    putchar('\n');
}

int main(void)
{
    ParlGame g;
    ParlTimer t;

    parlGame_init(
        &g,
        2,
        3,
        1,
        parlSymbolToIdx("3h")
    );
    parlTimer_start(&t);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, APPOINT_MP, parlSymbolToIdx("3h"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, IMPEACH_MP, parlSymbolToIdx("3h"), parlSymbolToIdx("4d"), PARL_NO_ARG);
    parlGame_applyAction(&g, BLOCK_IMPEACH, parlSymbolToIdx("5h"), PARL_NO_ARG, PARL_NO_ARG);
    //parlGame_applyAction(&g, REIMPEACH, parlSymbolToIdx("6d"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, NO_REIMPEACH, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, NO_REIMPEACH, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, NO_REIMPEACH, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("xc"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("7d"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, CALL_ELECTION, parlSymbolToIdx("qh"), parlSymbolToIdx("8h"), parlSymbolToIdx("4h"));
    parlGame_applyAction(&g, NO_CONTEST_ELECTION, PARL_NO_ARG, PARL_NO_ARG, parlSymbolToIdx("5s"));
    parlGame_applyAction(&g, CONTEST_ELECTION, parlSymbolToIdx("qd"), parlSymbolToIdx("xd"), PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("6h"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("as"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("8c"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, SELF_DRAW, parlSymbolToIdx("6c"), PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, DRAW, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    printParlGame(&g);
    parlGame_applyAction(&g, CALL_ELECTION, parlSymbolToIdx("kd"), parlSymbolToIdx("9d"), parlSymbolToIdx("2d"));
    parlGame_applyAction(&g, NO_CONTEST_ELECTION, PARL_NO_ARG, PARL_NO_ARG, PARL_NO_ARG);
    parlGame_applyAction(&g, CONTEST_ELECTION, parlSymbolToIdx("kh"), parlSymbolToIdx("7h"), PARL_NO_ARG);
    printParlGame(&g);
    parlTimer_stop(&t);
    printParlStack(g.faceDownCards);
    parlGame_free(&g);
    printf("\nTIME: %d μs\n", parlTimer_microSecs(&t));
    return 0;
}
