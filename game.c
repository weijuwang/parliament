//
// Created by Weiju Wang on 8/17/24.
//

#include "game.h"

#include <stdlib.h>
#include <string.h>

#define DECREASE_HAND_SIZE(n) g->handSizes[g->turn] -= n

bool parlGame_init(ParlGame* const g,
                   const int numJokers,
                   const int numPlayers,
                   const ParlPlayer myPosition,
                   const ParlIdx myFirstCardIdx)
{
    g->numPlayers = numPlayers;
    g->myPosition = myPosition;

    g->pmPosition = PARL_NO_PM;
    g->pmCardIdx = PARL_NO_PM;
    g->cabinet = PARL_EMPTY_STACK;

    g->turn = 0;
    g->parliament = PARL_EMPTY_STACK;
    g->discard = PARL_EMPTY_STACK;
    g->drawDeckSize = PARL_NUM_NON_JOKER_CARDS + numJokers;
    g->mode = NORMAL;

    g->handSizes = malloc(numPlayers * sizeof(int));
    g->knownHands = calloc(numPlayers, sizeof(ParlStack));

    for(int i = 0; i < numPlayers; ++i)
        g->handSizes[i] = 1;
    g->knownHands[g->myPosition] = PARL_CARD(myFirstCardIdx);

    g->faceDownCards = numJokers * PARL_JOKER_CARD
                       + PARL_COMPLETE_STACK_NO_JOKERS
                       - PARL_CARD(myFirstCardIdx);

    // Memory allocation errors will set one of these to null.
    return g->handSizes && g->knownHands;
}

void parlGame_free(const ParlGame* const g)
{
    free(g->handSizes);
    free(g->knownHands);
}

bool parlGame_deepCopy(ParlGame* const dest, const ParlGame* const orig)
{
    const register unsigned int HAND_SIZES_SIZE = orig->numPlayers * sizeof(int);
    const register unsigned int KNOWN_HANDS_SIZE = dest->numPlayers * sizeof(ParlStack);

    memcpy(dest, orig, sizeof(ParlGame));

    if(!(dest->handSizes = malloc(HAND_SIZES_SIZE)))
        return false;
    if(!(dest->knownHands = malloc(KNOWN_HANDS_SIZE)))
        return false;

    memcpy(dest->handSizes, orig->handSizes, HAND_SIZES_SIZE);
    memcpy(dest->knownHands, orig->knownHands, KNOWN_HANDS_SIZE);

    return true;
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
                        if (parlStackSize(PARL_FILTER_SUIT(playerHand, s)) >= 3)
                            goto electionLegal;

                    electionLegal = false;
                    electionLegal:;

                    register int numCards;
                    PARL_FOREACH_RANK_FROM(r, 0 /* TODO replace with one higher than the highest rank of the plurality suit */)
                    {
                        numCards = 0;

                        PARL_FOREACH_SUIT(s)
                            if(PARL_CONTAINS(playerHand, PARL_RS_TO_CARD(r, s)))
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
                    const register ParlRank pmCardRank = PARL_RANK(g->pmCardIdx);

                    // Kings can impeach kings
                    if(pmCardRank == PARL_KING_RANK)
                    {
                        PARL_FOREACH_SUIT(s)
                            if(PARL_CONTAINS(playerHand, PARL_RS_TO_CARD(PARL_KING_RANK, s)))
                                goto impeachPmLegal;
                    }
                    else
                    {
                        PARL_FOREACH_RANK_FROM(r, pmCardRank + 1)
                            PARL_FOREACH_SUIT(s)
                                if(PARL_CONTAINS(playerHand, PARL_RS_TO_CARD(r, s)))
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
            return (1l<<REIMPEACH) + (1l<<ALLOW_IMPEACH);
        case ELECTION:
            return (1l<<CONTEST_ELECTION) + (1l<<NO_CONTEST_ELECTION);
        case BACKUP_PM:
            return 1l<<APPOINT_PM;
    }
}

unsigned int parlGame_tiedPluralities(const ParlGame* const g)
{
    register unsigned int tiedPluralities = 0;
    register int pluralitySuitSize = 0, thisSuitSize;

    PARL_FOREACH_SUIT(s)
    {
        thisSuitSize = parlStackSize(PARL_FILTER_SUIT(g->parliament, s));
        if(thisSuitSize > pluralitySuitSize)
        {
            pluralitySuitSize = thisSuitSize;
            tiedPluralities = 1u << s;
        }
        else if(thisSuitSize == pluralitySuitSize)
            tiedPluralities |= 1u << s;
    }

    return tiedPluralities;
}

ParlSuit parlGame_plurality(const ParlGame* g)
{
    register ParlSuit plurality = INVALID_SUIT;
    register int pluralitySuitSize = 0, thisSuitSize;

    PARL_FOREACH_SUIT(s)
    {
        thisSuitSize = parlStackSize(PARL_FILTER_SUIT(g->parliament, s));
        if(thisSuitSize > pluralitySuitSize)
        {
            plurality = s;
            pluralitySuitSize = thisSuitSize;
        }
        else if(thisSuitSize == pluralitySuitSize)
            return INVALID_SUIT;
    }

    return plurality;
}

bool parlGame_applyAction(ParlGame* const g,
                          const ParlAction a,
                          const ParlIdx idxA,
                          const ParlIdx idxB,
                          const ParlIdx idxC
                          )
{
    register bool myTurn = g->turn == g->myPosition;
    register ParlStack cardA = PARL_CARD(idxA),
        cardB = PARL_CARD(idxB),
        cardC = PARL_CARD(idxC);

    switch(a)
    {
        case DRAW:
            if(myTurn && !parlMoveCards(
                &(g->knownHands[g->turn]),
                &g->faceDownCards,
                cardA
            ))
                // The card that was drawn is already face-up or known to be in someone's hand
                return false;
            DECREASE_HAND_SIZE(-1);
            goto incTurn;

        case IMPEACH_PM:
            g->discard += PARL_CARD(g->pmCardIdx);

            switch(parlStackSize(g->cabinet))
            {
                // No more PM!
                case 0:
                    g->pmPosition = PARL_NO_PM;
                    g->pmCardIdx = PARL_NO_PM;
                    break;
                // There is only one card that the
                case 1:
                    PARL_FOREACH_IDX(i)
                        // We know this must be the only card in Cabinet since we also know its size is 1
                        if(PARL_CONTAINS(g->cabinet, PARL_CARD(i)))
                        {
                            g->pmCardIdx = i;
                            break;
                        }
                    // After removing the only card in Cabinet, it must be empty
                    g->cabinet = PARL_EMPTY_STACK;
                    break;
                default:
                    g->mode = BACKUP_PM;
                    break;
            }

            // Then fall through to discard cardA -- missing break statement is intentional
        case DISCARD:
            if(
                !parlMoveCards(
                    &g->discard,
                    &(g->knownHands[g->turn]),
                    cardA
                )
                && !parlMoveCards(
                    &g->discard,
                    &g->faceDownCards,
                    cardA
                )
            )
                return false;

            DECREASE_HAND_SIZE(1);

            switch(g->mode)
            {
                case BACKUP_PM:
                    goto noIncTurn;

                case DISCARD_AFTER_DRAW:
                    g->mode = NORMAL;
                default:
                    goto incTurn;
            }

        case APPOINT_MP:
            if(
                !parlMoveCards(
                    &g->parliament,
                    &(g->knownHands[g->turn]),
                    cardA
                )
                && !parlMoveCards(
                    &g->parliament,
                    &g->faceDownCards,
                    cardA
                )
            )
                return false;
            DECREASE_HAND_SIZE(1);
            goto incTurn;

        case ENTER_ELECTION:
            // TODO

            g->mode = ELECTION;
            DECREASE_HAND_SIZE(3);
            goto noIncTurn;

        case IMPEACH_MP:

            g->mode = IMPEACH;
            DECREASE_HAND_SIZE(1);
            goto noIncTurn;

        case VOTE_NO_CONF:;
            ParlSuit rankA = PARL_RANK(idxA),
                rankB = PARL_RANK(idxB),
                rankC = PARL_RANK(idxC);

            if(rankA != rankB || rankB != rankC)
                return false;

            register ParlIdx selectedIdx;
            register unsigned int pluralitySuits = parlGame_tiedPluralities(g);

            if((1u << PARL_SUIT(idxA)) & pluralitySuits)
                selectedIdx = idxA;
            else if((1u << PARL_SUIT(idxB)) & pluralitySuits)
                selectedIdx = idxB;
            else if((1u << PARL_SUIT(idxC)) & pluralitySuits)
                selectedIdx = idxC;
            else return false;

            register ParlSuit selectedSuit = PARL_SUIT(selectedIdx);

            for(
                ParlIdx i = PARL_RS_TO_IDX(PARL_KING_RANK, selectedSuit);
                i >= PARL_RS_TO_IDX(PARL_ACE_RANK, selectedSuit);
                --i)
            {
                // Iterate through all cards of the selected plurality suit going down.
                // If we hit the calling card before hitting any MPs it means there are no MPs higher than that card.
                if(PARL_CONTAINS(g->parliament, PARL_CARD(i)))
                    // Illegal -- calling cards aren't high enough
                    return false;
                else if(i == selectedIdx)
                    // Legal
                    break;
            }

            DECREASE_HAND_SIZE(3);
            goto incTurn;

        case CABINET_RESHUFFLE:
            if(!PARL_CONTAINS(g->cabinet, cardA) || !PARL_CONTAINS(g->parliament, cardB))
                return false;

            // "Move to Cabinet from Parliament card B"
            parlMoveCards(&g->cabinet, &g->parliament, cardB);
            // "Move to Parliament from Cabinet card A"
            parlMoveCards(&g->parliament, &g->cabinet, cardA);

            goto incTurn;

        case APPOINT_PM:
            if(PARL_CONTAINS(g->cabinet, cardA))
            {
                g->cabinet |= PARL_CARD(g->pmCardIdx);
                g->pmCardIdx = idxA;
                goto incTurn;
            } else return false;

        case BLOCK_IMPEACH:
            // TODO
            DECREASE_HAND_SIZE(1);
            goto noIncTurn;

        case REIMPEACH:
            // TODO
            DECREASE_HAND_SIZE(1);
            goto noIncTurn;

        case ALLOW_IMPEACH:
            // TODO
            break;

        case CONTEST_ELECTION:
            // TODO
            DECREASE_HAND_SIZE(2);
            goto noIncTurn;

        case NO_CONTEST_ELECTION:
            // TODO
            break;

        case APPOINT_BACKUP_PM:
            // TODO
            break;
    }

    incTurn:
        if(++(g->turn) == g->numPlayers)
            g->turn = 0;
    noIncTurn:

    return true;
}

void parlGame_removeFromHand(ParlGame* g, ParlStack s)
{
    DECREASE_HAND_SIZE(parlStackSize(s));
    parlRemoveCardsPartial(&g->knownHands[g->turn], s);
    if(g->turn != g->myPosition)
        parlRemoveCardsPartial(&g->faceDownCards, s);
}