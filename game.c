//
// Created by Weiju Wang on 8/17/24.
//

#include "game.h"

#include <stdlib.h>

bool parlGame_init(ParlGame* const g,
                   const int numJokers,
                   const int numPlayers,
                   const ParlPlayer myPosition,
                   const ParlIdx myFirstCard)
{
    g->numPlayers = numPlayers;
    g->myPosition = myPosition;

    g->pmPosition = PARL_NO_PM;
    g->pmCard = PARL_NO_PM;
    g->cabinet = PARL_EMPTY_STACK;

    g->turn = 0;
    g->parliament = PARL_EMPTY_STACK;
    g->drawDeckSize = PARL_JOKER_IDX + numJokers;
    g->mode = NORMAL;

    g->handSizes = malloc(numPlayers * sizeof(int));
    g->knownHands = calloc(numPlayers, sizeof(ParlStack));

    for(int i = 0; i < numPlayers; ++i)
        g->handSizes[i] = 1;
    g->knownHands[g->myPosition] = myFirstCard;

    g->faceDownCards = numJokers * PARL_JOKER_CARD
                       + PARL_COMPLETE_STACK_NO_JOKERS;

    // Memory allocation errors will set one of these to null.
    return g->handSizes && g->knownHands;
}

void parlGame_free(const ParlGame* const g)
{
    free(g->handSizes);
    free(g->knownHands);
}

unsigned int parlGame_legalActions(const ParlGame* const g)
{
    switch(g->mode)
    {
        case NORMAL:;
            register unsigned int legalMoves = 1l<<DRAW;
            #define PARL_ADD_LEGAL_MOVE(m) legalMoves += 1l<<m

            const register int handSize = g->handSizes[g->turn];
            const register int parlSize = parlStackSize(g->parliament);
            const register bool myTurn = PARL_MY_TURN(g);
            const register bool iAmPm = g->turn == g->pmPosition;
            const register ParlStack playerHand = g->knownHands[g->turn];

            // PM-exclusive actions
            if(iAmPm && parlStackSize(g->cabinet))
            {
                PARL_ADD_LEGAL_MOVE(APPOINT_PM);
                if(parlSize)
                    PARL_ADD_LEGAL_MOVE(CABINET_RESHUFFLE);
            }

            // All actions below here require a non-empty hand
            if(!handSize)
                return legalMoves;

            PARL_ADD_LEGAL_MOVE(DISCARD);

            if(parlSize < 2 * g->numPlayers)
                PARL_ADD_LEGAL_MOVE(APPOINT_MP);

            if(handSize >= 3)
            {
                register bool electionLegal = true;
                register bool vncLegal = true;

                if(myTurn)
                {
                    PARL_FOREACH_SUIT(s)
                        if (parlStackSize(PARL_STACK_FILTER_SUIT(playerHand, s)) >= 3)
                            goto electionLegal;

                    electionLegal = false;
                    electionLegal:;

                    register int numCards;
                    PARL_FOREACH_RANK_FROM(r, 0 /* TODO replace with one higher than the highest rank of the plurality suit */)
                    {
                        numCards = 0;

                        PARL_FOREACH_SUIT(s)
                            if(PARL_STACK_CONTAINS(playerHand, PARL_RS_TO_CARD(r, s)))
                                ++numCards;

                        if(numCards >= 3)
                            goto vncLegal;
                    }

                    vncLegal = false;
                    vncLegal:;
                }

                if(electionLegal) PARL_ADD_LEGAL_MOVE(ENTER_ELECTION);
                if(vncLegal) PARL_ADD_LEGAL_MOVE(VOTE_NO_CONF);
            }

            if(g->pmPosition != PARL_NO_PM)
            {
                if(myTurn)
                {
                    const register ParlRank pmCardRank = parlRank(g->pmCard);

                    // Kings can impeach kings
                    if(pmCardRank == PARL_KING_RANK)
                    {
                        PARL_FOREACH_SUIT(s)
                            if(PARL_STACK_CONTAINS(playerHand, PARL_RS_TO_CARD(PARL_KING_RANK, s)))
                                goto impeachPmLegal;
                    }
                    else
                    {
                        PARL_FOREACH_RANK_FROM(r, pmCardRank + 1)
                            PARL_FOREACH_SUIT(s)
                                if(PARL_STACK_CONTAINS(playerHand, PARL_RS_TO_CARD(r, s)))
                                    goto impeachPmLegal;
                    }

                    goto impeachPmIllegal;
                }

                impeachPmLegal: PARL_ADD_LEGAL_MOVE(IMPEACH_PM);
                impeachPmIllegal:;
            }

            if(handSize > 0 && parlSize)
            {
                if(myTurn)
                {
                    PARL_FOREACH_IN_STACK(g->parliament, mp)
                        PARL_FOREACH_IN_STACK(playerHand, h)
                            if(PARL_HIGHER_THAN(h, mp))
                                    goto impeachMpLegal;
                    goto impeachMpIllegal;
                }
                impeachMpLegal:
                PARL_ADD_LEGAL_MOVE(IMPEACH_MP);
                impeachMpIllegal:;
            }

            return legalMoves;
        case DISCARD_AFTER_DRAW:
            return 1l<<DISCARD;
        case IMPEACH:
            return (1l<<IMPEACH_MP) + (1l<<PASS);
        case ELECTION:
            return (1l<<ENTER_ELECTION) + (1l<<PASS);
        case BACKUP_PM:
            return 1l<<APPOINT_PM;
    }
}