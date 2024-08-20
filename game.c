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
    g->drawDeckSize = PARL_NUM_NON_JOKER_CARDS + numJokers - numPlayers;
    g->mode = NORMAL_MODE;

    if(!(g->knownHands = calloc(numPlayers, sizeof(ParlStack))))
        return false;

    for(ParlPlayer p = 0; p < numPlayers; ++p)
        g->handSizes[p] = 1;
    g->knownHands[g->myPosition] = PARL_CARD(myFirstCardIdx);

    g->faceDownCards = numJokers * PARL_JOKER_CARD
                       + PARL_COMPLETE_STACK_NO_JOKERS
                       - PARL_CARD(myFirstCardIdx);

    return true;
}

void parlGame_free(const ParlGame* const g)
{
    free(g->knownHands);
}

bool parlGame_deepCopy(ParlGame* const dest, const ParlGame* const orig)
{
    const register unsigned int KNOWN_HANDS_SIZE = dest->numPlayers * sizeof(ParlStack);

    memcpy(dest, orig, sizeof(ParlGame));

    if(!(dest->knownHands = malloc(KNOWN_HANDS_SIZE)))
        return false;

    memcpy(dest->knownHands, orig->knownHands, KNOWN_HANDS_SIZE);

    return true;
}

unsigned int parlGame_legalActions(const ParlGame* const g)
{
    switch(g->mode)
    {
        case NORMAL_MODE:;
            register unsigned int legalMoves = 1u<<DRAW;
            #define PARL_ADD_LEGAL_MOVE(m) legalMoves += 1u<<m

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
        case DISCARD_AFTER_DRAW_MODE:
            return 1u<<DISCARD;
        case REIMPEACH_MODE:
            return (1u<<REIMPEACH) + (1u << NO_REIMPEACH);
        case BLOCK_IMPEACH_MODE:
            return (1u<<BLOCK_IMPEACH) + (1u << NO_BLOCK_IMPEACH);
        case ELECTION_MODE:
            return (1u<<CONTEST_ELECTION) + (1u<<NO_CONTEST_ELECTION);
        case BACKUP_PM_MODE:
            return 1u<<APPOINT_PM;
        default:
            return 0;
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
            --g->drawDeckSize;

            if(g->handSizes[g->turn] >= PARL_MAX_CARDS_IN_HAND)
            {
                g->mode = DISCARD_AFTER_DRAW_MODE;
                goto noIncTurn;
            } else goto incTurn;

        case IMPEACH_PM:
            g->discard += PARL_CARD(g->pmCardIdx);

            switch(parlStackSize(g->cabinet))
            {
                // No more PM!
                case 0:
                    g->pmPosition = PARL_NO_PM;
                    g->pmCardIdx = PARL_NO_PM;
                    break;
                // There is only one card that the PM card can be replaced with, so the PM doesn't get a choice
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
                    g->mode = BACKUP_PM_MODE;
                    parlGame_saveNextNormalTurn(g);
                    break;
            }

            // Then fall through to discard cardA -- missing break statement is intentional
        case DISCARD:
            if(!parlGame_moveFromHandTo(g, &g->discard, cardA))
                return false;

            switch(g->mode)
            {
                // This means we came from IMPEACH_PM, not from DISCARD, and the move after this one is the PM's
                // replacement of the PM card.
                case BACKUP_PM_MODE:
                    g->turn = g->pmPosition;
                    goto noIncTurn;

                case DISCARD_AFTER_DRAW_MODE:
                    g->mode = NORMAL_MODE;
                default:
                    goto incTurn;
            }

        case APPOINT_MP:
            if(parlGame_moveFromHandTo(g, &g->parliament, cardA))
                goto incTurn;
            else
                return false;

        case ENTER_ELECTION:
            // TODO

            g->mode = ELECTION_MODE;
            DECREASE_HAND_SIZE(3);
            goto noIncTurn;

        case IMPEACH_MP:
            if(
                !PARL_CONTAINS(g->parliament, cardA)
                || !PARL_HIGHER_THAN(idxB, idxA)
                || !parlGame_removeFromHand(g, cardB)
            )
                return false;

            g->temp.shared.impeachedMpIdx = idxA;
            g->temp.cardToBeatIdx = idxB;

            g->mode = BLOCK_IMPEACH_MODE;
            parlGame_saveNextNormalTurn(g);
            g->turn = 0;
            goto noIncTurn;

        case VOTE_NO_CONF:;
            if(!parlGame_removeFromHand(g, cardA + cardB + cardC))
                return false;

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

                if(i == selectedIdx)
                    // Legal
                    break;
            }

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
            if(
                !parlGame_handContains(g, cardA)
                || PARL_SUIT(idxA) != PARL_SUIT(g->temp.shared.impeachedMpIdx)
            )
                return false;

            if(PARL_HIGHER_THAN(idxA, g->temp.cardToBeatIdx))
            {
                g->discard |= PARL_CARD(g->temp.cardToBeatIdx);
                parlGame_removeFromHand(g, cardA);
                g->temp.cardToBeatIdx = idxA;
                g->turn = 0;
                g->mode = REIMPEACH_MODE;
            }
            else if(
                g->temp.cardToBeatIdx == PARL_JOKER_IDX
                && PARL_RANK(idxA) == PARL_ACE_RANK
            )
            {
                parlGame_confirmImpeachedMp(g);
            }
            else return false;

            goto noIncTurn;

        case REIMPEACH:
            if(
                !parlGame_handContains(g, cardA)
                || PARL_SUIT(idxA) != PARL_SUIT(g->temp.shared.impeachedMpIdx)
            )
                return false;

            if(PARL_HIGHER_THAN(idxA, g->temp.cardToBeatIdx))
            {
                g->discard |= PARL_CARD(g->temp.cardToBeatIdx);
                parlGame_removeFromHand(g, cardA);
                g->temp.cardToBeatIdx = idxA;
                g->turn = 0;
                g->mode = REIMPEACH_MODE;
            }
            else if(
                g->temp.cardToBeatIdx == PARL_JOKER_IDX
                && PARL_RANK(idxA) == PARL_ACE_RANK
                )
            {
                parlGame_confirmImpeachedMp(g);
            }
            else return false;

            goto noIncTurn;

        case NO_REIMPEACH:
        case NO_BLOCK_IMPEACH:
            if(g->turn < g->numPlayers - 1)
                goto incTurn;

            parlGame_confirmImpeachedMp(g);
            goto noIncTurn;

        case CONTEST_ELECTION:
            // TODO
            DECREASE_HAND_SIZE(2);
            goto noIncTurn;

        case NO_CONTEST_ELECTION:
            if(g->turn < g->numPlayers - 1)
                goto incTurn;

            // TODO
            goto noIncTurn;

        case APPOINT_BACKUP_PM:
            if(parlRemoveCards(&g->cabinet, cardA))
                return false;

            g->pmCardIdx = idxA;
            g->mode = NORMAL_MODE;
            goto incTurn;
    }

    incTurn:
        g->turn = PARL_NEXT_TURN(g);
    noIncTurn:

    return true;
}

bool parlGame_handContains(const ParlGame* g, const ParlStack s)
{
    const register ParlStack nj = PARL_WITHOUT_JOKERS(s);
    return nj == ((nj & g->knownHands[g->turn]) + (nj & g->faceDownCards))
        &&
            PARL_NUM_JOKERS(s) <=
            PARL_NUM_JOKERS(g->faceDownCards) + PARL_NUM_JOKERS(g->knownHands[g->turn]);
}

void parlGame_saveNextNormalTurn(ParlGame* g)
{
    g->temp.nextNormalTurn = PARL_NEXT_TURN(g);
}

bool parlGame_removeFromHand(ParlGame* g, const ParlStack s)
{
    if(!parlGame_handContains(g, s))
        return false;

    DECREASE_HAND_SIZE(parlStackSize(s));
    parlRemoveCardsPartial(&g->knownHands[g->turn], s);
    parlRemoveCardsPartial(&g->faceDownCards, s);

    return true;
}

bool parlGame_moveFromHandTo(ParlGame* const g, ParlStack* const dest, const ParlStack s)
{
    /*
     * TODO Should these be partial moves instead? Check with parlGame_removeFromHand()
     */
    if(
        parlMoveCards(dest,&(g->knownHands[g->turn]),s)
        || parlMoveCards(dest,&g->faceDownCards,s)
    )
    {
        DECREASE_HAND_SIZE(parlStackSize(s));
        return true;
    }

    return false;
}

void parlGame_confirmImpeachedMp(ParlGame* const g)
{
    parlMoveCards(&g->discard, &g->parliament, PARL_CARD(g->temp.shared.impeachedMpIdx));
    g->parliament |= PARL_CARD(g->temp.cardToBeatIdx);
    g->mode = NORMAL_MODE;
    g->turn = g->temp.nextNormalTurn;
}