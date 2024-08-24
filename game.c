//
// Created by Weiju Wang on 8/17/24.
//

/*
 * TODO If ace beats joker it automatically wins the cycle
 */

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
    *g = (ParlGame){
        .numPlayers = numPlayers,
        .myPosition = myPosition,
        .turn = 0,
        .pmPosition = PARL_NO_PM,
        .pmCardIdx = PARL_NO_PM,
        .cabinet = PARL_EMPTY_STACK,
        .parliament = PARL_EMPTY_STACK,
        .discard = PARL_EMPTY_STACK,
        .drawDeckSize = PARL_NUM_NON_JOKER_CARDS + numJokers - numPlayers,
        .mode = NORMAL_MODE,
        .elecCands = NULL,

        .knownHands = calloc(numPlayers, sizeof(ParlStack)),
        .faceDownCards = numJokers * PARL_JOKER_CARD
            + PARL_COMPLETE_STACK_NO_JOKERS
            - PARL_CARD(myFirstCardIdx),
    };

    if(!g->knownHands)
        return false;
    g->knownHands[g->myPosition] = PARL_CARD(myFirstCardIdx);

    PARL_FOREACH_PLAYER(g, p)
        g->handSizes[p] = 1;

    return true;
}

void parlGame_free(const ParlGame* const g)
{
    free(g->knownHands);
    if(g->mode == ELECTION_MODE)
        free(g->elecCands);
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
            register unsigned int legalMoves = 0u;
            #define PARL_ADD_LEGAL_MOVE(m) legalMoves |= 1u<<m

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

            if(handSize <= PARL_MAX_CARDS_IN_HAND)
                PARL_ADD_LEGAL_MOVE(g->turn == g->myPosition ? 1u<<SELF_DRAW : 1u<<DRAW);

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

                PARL_FOREACH_SUIT(s)
                    if (parlStackSize(PARL_FILTER_SUIT(playerHand + g->faceDownCards, s)) >= 3)
                        goto electionLegal;

                electionLegal = false;
                electionLegal:;

                register int numCards;
                register bool matchesPluralitySuit;
                register unsigned int pluralitySuits = parlGame_tiedPluralities(g);

                PARL_FOREACH_RANK_FROM(r, 0)
                {
                    numCards = 0;
                    matchesPluralitySuit = false;

                    PARL_FOREACH_SUIT(s)
                    {
                        if(parlGame_handContains(g, PARL_RS_TO_CARD(r, s)))
                            ++numCards;

                        if(
                            ((1u<<s) & pluralitySuits)
                            && (PARL_FILTER_SUIT(g->parliament, s) >> PARL_RS_TO_IDX(r, s))
                        )
                            matchesPluralitySuit = true;
                    }

                    if(numCards >= 3 && matchesPluralitySuit)
                        goto vncLegal;
                }

                vncLegal = false;
                vncLegal:;

                if(electionLegal) PARL_ADD_LEGAL_MOVE(CALL_ELECTION);
                if(vncLegal) PARL_ADD_LEGAL_MOVE(VOTE_NO_CONF);
            }

            if(g->pmPosition != PARL_NO_PM)
            {
                const register ParlRank pmCardRank = PARL_RANK(g->pmCardIdx);

                // Kings can impeach kings
                if(pmCardRank == PARL_KING_RANK)
                {
                    PARL_FOREACH_SUIT(s)
                        if(parlGame_handContains(g, PARL_RS_TO_CARD(PARL_KING_RANK, s)))
                            goto impeachPmLegal;
                }
                else
                {
                    PARL_FOREACH_RANK_FROM(r, pmCardRank + 1)
                        PARL_FOREACH_SUIT(s)
                            if(parlGame_handContains(g, PARL_RS_TO_CARD(r, s)))
                                goto impeachPmLegal;
                }

                goto impeachPmIllegal;

                impeachPmLegal: PARL_ADD_LEGAL_MOVE(IMPEACH_PM);
                impeachPmIllegal:;
            }

            if(handSize > 0 && parlSize)
            {
                PARL_FOREACH_IN_STACK(g->parliament, mp)
                    PARL_FOREACH_IN_STACK(playerHand + g->faceDownCards, h)
                        if(PARL_HIGHER_THAN(h, mp))
                            goto impeachMpLegal;
                goto impeachMpIllegal;

                impeachMpLegal:
                PARL_ADD_LEGAL_MOVE(IMPEACH_MP);
                impeachMpIllegal:;
            }

            return legalMoves;
        case DISCARD_AFTER_DRAW_MODE:
            return 1u<<DISCARD;
        case REIMPEACH_MODE:
            return (1u<<REIMPEACH) | (1u << NO_REIMPEACH);
        case BLOCK_IMPEACH_MODE:
            return (1u<<BLOCK_IMPEACH) | (1u << NO_BLOCK_IMPEACH);
        case ELECTION_MODE:
            return (1u<<CONTEST_ELECTION) | (1u<<NO_CONTEST_ELECTION);
        case BACKUP_PM_MODE:
            return 1u<<APPOINT_PM;
        case ENDGAME_MODE:
            return (1u<<ENDGAME_TRY_FORMATION) | (1u<<ENDGAME_PASS_FORMATION);
        case PM_CHOOSE_FIRST_LAST_MODE:
            return (1u<<ENDGAME_PM_FIRST) | (1u<<ENDGAME_PM_LAST);
        case BLOCK_COALITION_MODE:
            return (1u<<ENDGAME_BLOCK_COALITION) | (1u<<ENDGAME_NO_BLOCK_COALITION);
        case COUNTER_BLOCK_COALITION_MODE:
            return (1u<<ENDGAME_COUNTER_BLOCK_COALITION) | (1u<<ENDGAME_NO_COUNTER_BLOCK_COALITION);
        case GAME_OVER:
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

        if(!thisSuitSize)
            continue;

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
    // TODO Sanity check: idxA, idxB, and idxC must all be diff cards unless they're PARL_NO_ARG

    register ParlStack cardA = PARL_CARD(idxA),
        cardB = PARL_CARD(idxB),
        cardC = PARL_CARD(idxC);

    switch(a)
    {
        case SELF_DRAW:
            if(!parlMoveCards(
                &(g->knownHands[g->turn]),
                &g->faceDownCards,
                cardA
            ))
                return false;
        case DRAW:
            DECREASE_HAND_SIZE(-1);
            --g->drawDeckSize;

            if(g->handSizes[g->turn] > PARL_MAX_CARDS_IN_HAND)
                g->mode = DISCARD_AFTER_DRAW_MODE;
            else if(g->drawDeckSize == 0)
                parlGame_moveToEndgame(g);
            else
                parlGame_incTurn(g);
            return true;

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
                    parlGame_saveNormalTurn(g);
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
                    return true;

                case DISCARD_AFTER_DRAW_MODE:
                    if(!g->drawDeckSize)
                    {
                        parlGame_moveToEndgame(g);
                        return true;
                    }

                    g->mode = NORMAL_MODE;
                    // Fallthrough to default
                default:
                    parlGame_incTurn(g);
                    return true;
            }

        case APPOINT_MP:
            if(!parlGame_moveFromHandTo(g, &g->parliament, cardA))
                return false;

            parlGame_incTurn(g);
            return true;

        case CALL_ELECTION:
            // All 3 cards must be the same suit
            if(PARL_SUIT(idxA) != PARL_SUIT(idxB) || PARL_SUIT(idxB) != PARL_SUIT(idxC))
                return false;

            ParlCallingCardOrigin cardAOrigin = parlGame_cardOrigin(g, idxA),
                cardBOrigin = parlGame_cardOrigin(g, idxB),
                cardCOrigin = parlGame_cardOrigin(g, idxC);

            if(
                (g->turn == g->pmPosition && (cardAOrigin == UNKNOWN || cardBOrigin == UNKNOWN || cardCOrigin == UNKNOWN))
                || !parlGame_handContains(g, cardA | cardB | cardC)
            )
                return false;

            bool cardAFromHand = cardAOrigin == FROM_HAND,
                cardBFromHand = cardBOrigin == FROM_HAND,
                cardCFromHand = cardCOrigin == FROM_HAND;

            g->elecCands = malloc(g->numPlayers * sizeof(struct ParlElectionCand));
            g->elecCands[g->turn] = (struct ParlElectionCand){
                .pmIdx = idxA,
                .callingCards = cardA | cardB | cardC,
                .preCallNumCards = g->handSizes[g->turn]
            };

            // The hand size will decrease regardless of where the cards go
            DECREASE_HAND_SIZE(cardAFromHand + cardBFromHand + cardCFromHand);

            g->mode = ELECTION_MODE;
            g->cycleStarter = g->turn;
            parlGame_saveNormalTurn(g);
            g->turn = g->cycleStarter == 0 ? 1 : 0;
            return true;

        case IMPEACH_MP:
            if(
                !PARL_CONTAINS(g->parliament, cardA)
                || !PARL_HIGHER_THAN(idxB, idxA)
                || !parlGame_removeFromHand(g, cardB)
            )
                return false;

            g->impeachedMpIdx = idxA;
            g->cardToBeatIdx = idxB;

            g->mode = BLOCK_IMPEACH_MODE;
            parlGame_saveNormalTurn(g);
            g->turn = 0;
            return true;

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
                --i
            )
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

            g->discard |= PARL_CARD(g->pmCardIdx) | g->cabinet;
            g->pmPosition = PARL_NO_PM;
            g->pmCardIdx = PARL_NO_PM;
            g->cabinet = PARL_EMPTY_STACK;

            parlGame_incTurn(g);
            return true;

        case CABINET_RESHUFFLE:
            if(!PARL_CONTAINS(g->cabinet, cardA) || !PARL_CONTAINS(g->parliament, cardB))
                return false;

            // "Move to Cabinet from Parliament card B"
            parlMoveCards(&g->cabinet, &g->parliament, cardB);
            // "Move to Parliament from Cabinet card A"
            parlMoveCards(&g->parliament, &g->cabinet, cardA);

            parlGame_incTurn(g);
            return true;

        case APPOINT_PM:
            if(!PARL_CONTAINS(g->cabinet, cardA))
                return false;

            g->cabinet |= PARL_CARD(g->pmCardIdx);
            g->pmCardIdx = idxA;
            parlGame_incTurn(g);
            return true;

        case BLOCK_IMPEACH:
            if(
                !parlGame_handContains(g, cardA)
                || PARL_SUIT(idxA) != PARL_SUIT(g->impeachedMpIdx)
            )
                return false;

            if(PARL_HIGHER_THAN(idxA, g->cardToBeatIdx))
            {
                g->discard |= PARL_CARD(g->cardToBeatIdx);
                parlGame_removeFromHand(g, cardA);
                g->cardToBeatIdx = idxA;
                g->turn = 0;
                g->mode = REIMPEACH_MODE;
            }
            else if(
                g->cardToBeatIdx == PARL_JOKER_IDX
                && PARL_RANK(idxA) == PARL_ACE_RANK
            )
            {
                parlGame_confirmImpeachedMp(g);
            }
            else return false;

            return true;

        case REIMPEACH:
            if(
                !parlGame_handContains(g, cardA)
                || PARL_SUIT(idxA) != PARL_SUIT(g->impeachedMpIdx)
            )
                return false;

            if(PARL_HIGHER_THAN(idxA, g->cardToBeatIdx))
            {
                g->discard |= PARL_CARD(g->cardToBeatIdx);
                parlGame_removeFromHand(g, cardA);
                g->cardToBeatIdx = idxA;
                g->turn = 0;
                g->mode = REIMPEACH_MODE;
            }
            else if(
                g->cardToBeatIdx == PARL_JOKER_IDX
                && PARL_RANK(idxA) == PARL_ACE_RANK
                )
            {
                parlGame_confirmImpeachedMp(g);
            }
            else return false;

            return true;

        case NO_REIMPEACH:
        case NO_BLOCK_IMPEACH:
            parlGame_incTurn(g);

            if(g->turn == 0)
                parlGame_confirmImpeachedMp(g);

            return true;

        case CONTEST_ELECTION:
            if(!parlGame_handContains(g, cardA | cardB))
                return false;

            g->elecCands[g->turn] = (struct ParlElectionCand){
                .pmIdx = idxA,
                .callingCards = cardA | cardB,
                .preCallNumCards = g->handSizes[g->turn]
            };

            DECREASE_HAND_SIZE(2);

            goto contestElection;

        case NO_CONTEST_ELECTION:
            g->elecCands[g->turn].callingCards = PARL_EMPTY_STACK;
        contestElection:

            // If not the last player
            if(g->turn < g->numPlayers - 1)
            {
                // If the next player is the election caller, skip them
                if(g->turn + 1 == g->cycleStarter)
                    ++g->turn;

                parlGame_incTurn(g);
                return true;
            }

            /* Last player, so no more candidates. Election is over */

            /* Step 1: Find winners */

            register unsigned int winners = 0u;
            register int numWinners = 0;
            register ParlRank highestRank = PARL_ACE_RANK;
            register ParlRank thisRank;

            PARL_FOREACH_PLAYER(g, p)
            {
                // Skip players that didn't run
                if(g->elecCands[p].callingCards == PARL_EMPTY_STACK)
                    continue;

                thisRank = PARL_RANK(g->elecCands[p].pmIdx);
                if(thisRank > highestRank)
                {
                    highestRank = thisRank;
                    winners = 1u<<p;
                    numWinners = 1;
                }
                else if(thisRank == highestRank)
                {
                    winners |= 1u<<p;
                    ++numWinners;
                }
            }

            /* Step 2: Tiebreakers */

            register ParlPlayer winner = -1;

            // Multiple winners, have to check Parliament
            if(numWinners > 1)
            {
                register ParlSuit singlePlurality = parlGame_plurality(g);

                // There exists a single plurality suit
                if(singlePlurality != INVALID_SUIT)
                    // Find the player p whose PM candidate is of the plurality suit
                    PARL_FOREACH_PLAYER(g, p)
                        if(PARL_SUIT(g->elecCands[p].pmIdx) == singlePlurality)
                        {
                            /* We don't need to keep searching because it's impossible for two election candidates to
                             * and be of the same suit -- that would mean they played the same exact PM candidate. If
                             * we got here, we know this is the only candidate of this suit and thus the winner. */
                            winner = p;
                            goto winnerFound;
                        }

                // Election canceled -- return everyone's calling cards to `knownHands`
                PARL_FOREACH_PLAYER(g, p)
                {
                    if(g->elecCands[p].callingCards == PARL_EMPTY_STACK)
                        continue;

                    g->faceDownCards &= ~g->elecCands[p].callingCards;
                    g->knownHands[p] |= g->elecCands[p].callingCards;
                    g->handSizes[p] = g->elecCands[p].preCallNumCards;
                }

                goto cancelElection;
            }
            // Only one winner -- find out who it is
            else PARL_FOREACH_PLAYER(g, p)
                if(winners & (1u<<p))
                {
                    winner = p;
                    break;
                }

            winnerFound:;

            /* Step 3: Discard losing candidates' calling cards, confirm new PM */

            if(g->pmPosition != PARL_NO_PM)
            {
                // Remove PM's calling cards from hand
                parlRemoveCardsPartial(&g->knownHands[g->pmPosition], g->elecCands[g->pmPosition].callingCards);
                parlRemoveCardsPartial(&g->faceDownCards, g->elecCands[g->pmPosition].callingCards);
            }

            PARL_FOREACH_PLAYER(g, p)
            {
                if(g->elecCands[p].callingCards == PARL_EMPTY_STACK)
                    continue;

                if(p == winner)
                {
                    /* p was and remains PM */
                    if(p == g->pmPosition)
                    {
                        // Move calling cards and old PM card to cabinet -- new PM card is now somewhere in cabinet
                        g->cabinet |= g->elecCands[p].callingCards | PARL_CARD(g->pmCardIdx);
                        // Remove new PM card from Cabinet
                        g->cabinet &= ~PARL_CARD(g->elecCands[p].pmIdx);
                        // Update PM card
                        g->pmCardIdx = g->elecCands[p].pmIdx;
                    }
                    /* No PM until now or PM lost election */
                    else
                    {
                        // Remove calling cards from hand
                        parlGame_removeFromHandOf(g, g->elecCands[p].callingCards, p);

                        // Update PM
                        g->pmPosition = p;
                        g->pmCardIdx = g->elecCands[p].pmIdx;
                        g->cabinet = g->elecCands[p].callingCards & ~PARL_CARD(g->elecCands[p].pmIdx);
                    }
                }
                /* p was PM but lost election */
                else if(p == g->pmPosition)
                {
                    // Discard PM card and Cabinet
                    g->discard |= PARL_CARD(g->pmCardIdx) | g->cabinet;
                    // PM's calling cards were already removed from hand earlier
                }
                /* Non-PM player who lost election */
                else
                {
                    // Remove calling cards from hand
                    parlGame_removeFromHandOf(g, g->elecCands[p].callingCards, p);
                    // Discard calling cards
                    g->discard |= g->elecCands[p].callingCards;
                }
            }

            cancelElection:

            free(g->elecCands);
            g->elecCands = NULL;

            parlGame_revertToNormalModeAndTurn(g);
            parlGame_incTurn(g);
            return true;

        case APPOINT_BACKUP_PM:
            if(parlRemoveCards(&g->cabinet, cardA))
                return false;

            g->pmCardIdx = idxA;
            g->mode = NORMAL_MODE;
            parlGame_incTurn(g);
            return true;

        case ENDGAME_TRY_FORMATION:
            if(g->pmPosition == PARL_NO_PM && !parlGame_removeFromHand(g, cardA))
                return false;

            const register ParlIdx pmCandIdx = g->pmPosition == PARL_NO_PM ? idxA : g->pmCardIdx;

            g->discard |= pmCandIdx;
            g->cardToBeatIdx = pmCandIdx;

            g->coalitionSize = parlStackSize(PARL_FILTER_SUIT(g->parliament, PARL_SUIT(idxA)));

            // PM card counts as MP for the PM
            if(g->pmPosition != PARL_NO_PM)
                ++g->coalitionSize;

            // Majority w/o blocking?
            if(g->coalitionSize > g->numPlayers)
            {
                g->mode = GAME_OVER;
                return true;
            }

            // Needs coalition -- wait for blocks
            parlGame_saveNormalTurn(g);
            g->turn = 0;
            g->mode = BLOCK_COALITION_MODE;
            return true;

        case ENDGAME_PASS_FORMATION:
            parlGame_incTurnEndgame(g);
            return true;

        case ENDGAME_PM_FIRST:
            // Turn is currently set to PM
            g->cycleStarter = g->pmPosition;
            g->mode = ENDGAME_MODE;
            g->endgameSkipPm = false;
            return true;

        case ENDGAME_PM_LAST:
            // Turn is currently set to PM
            g->cycleStarter = PARL_NEXT_TURN(g);
            g->mode = ENDGAME_MODE;
            g->endgameSkipPm = true;

            parlGame_incTurn(g);
            return true;

        case ENDGAME_BLOCK_COALITION:
            if(
                !PARL_HIGHER_THAN(idxA, g->cardToBeatIdx)
                || PARL_SUIT(idxA) != PARL_SUIT(g->cardToBeatIdx)
                || !parlGame_handContains(g, cardA)
            )
                return false;

            g->discard |= g->cardToBeatIdx;
            g->cardToBeatIdx = idxA;
            g->turn = g->currNormalTurn;
            g->mode = COUNTER_BLOCK_COALITION_MODE;
            return true;

        case ENDGAME_NO_BLOCK_COALITION:
            parlGame_incTurn(g);

            if(g->turn == 0)
            {
                if(
                    g->coalitionSize
                    + parlStackSize(
                        PARL_FILTER_SUIT(g->parliament,
                            PARL_COALITION_PARTNERS[PARL_SUIT(g->cardToBeatIdx)]
                        )
                    )
                    <= g->numPlayers
                )
                    goto coalitionFail;

                g->mode = GAME_OVER;
                g->turn = g->currNormalTurn;
            }
            return true;

        case ENDGAME_COUNTER_BLOCK_COALITION:
            if(
                !PARL_HIGHER_THAN(idxA, g->cardToBeatIdx)
                || PARL_SUIT(idxA) != PARL_SUIT(g->cardToBeatIdx)
                || !parlGame_handContains(g, cardA)
            )
                return false;

            g->discard |= g->cardToBeatIdx;
            g->cardToBeatIdx = idxA;
            g->turn = 0;
            g->mode = BLOCK_COALITION_MODE;
            return true;

        case ENDGAME_NO_COUNTER_BLOCK_COALITION:
        coalitionFail:
            g->mode = ENDGAME_MODE;
            g->discard |= g->cardToBeatIdx;
            parlGame_revertToNormalModeAndTurn(g);
            parlGame_incTurnEndgame(g);

            return true;
    }
}

bool parlGame_handContains(const ParlGame* g, const ParlStack s)
{
    if(g->turn == g->myPosition)
        return PARL_CONTAINS(g->knownHands[g->turn], s);

    const register ParlStack nj = PARL_WITHOUT_JOKERS(s);
    return nj == ((nj & g->knownHands[g->turn]) + (nj & g->faceDownCards))
        &&
            PARL_NUM_JOKERS(s) <=
            PARL_NUM_JOKERS(g->faceDownCards) + PARL_NUM_JOKERS(g->knownHands[g->turn]);
}

void parlGame_incTurn(ParlGame* const g)
{
    g->turn = PARL_NEXT_TURN(g);
}

void parlGame_incTurnEndgame(ParlGame* const g)
{
    // If the PM chose to play last and the PM just went
    if(g->turn == g->pmPosition && g->endgameSkipPm)
        goto dissolveParliament;

    parlGame_incTurn(g);

    // If next player is PM and we're supposed to skip the PM (b/c they chose to play last)
    if(g->turn == g->pmPosition && g->endgameSkipPm)
        parlGame_incTurn(g);

    // If we've gotten to all players
    if(g->turn == g->cycleStarter)
    {
        // ...but not the PM? Then the PM needs to have a go
        if(g->endgameSkipPm)
            g->turn = g->pmPosition;
        // ...including the PM? Then dissolve Parliament
        else
        {
            dissolveParliament:

            g->pmPosition = PARL_NO_PM;

            // Move entire discard pile to draw deck
            g->drawDeckSize = parlStackSize(g->discard);
            parlMoveCards(&g->faceDownCards, &g->discard, g->discard);

            g->turn = g->cycleStarter;
            g->mode = NORMAL_MODE;
        }
    }
}

void parlGame_saveNormalTurn(ParlGame *const g)
{
    g->currNormalTurn = g->turn;
}

void parlGame_revertToNormalModeAndTurn(ParlGame *const g)
{
    g->mode = NORMAL_MODE;
    g->turn = g->currNormalTurn;
}

void parlGame_moveToEndgame(ParlGame* const g)
{
    // Move Cabinet to PM's hand
    g->knownHands[g->pmPosition] |= g->cabinet;
    g->cabinet = PARL_EMPTY_STACK;

    if(g->pmPosition == PARL_NO_PM)
    {
        g->mode = ENDGAME_MODE;
        g->cycleStarter = PARL_NEXT_TURN(g);
        g->endgameSkipPm = false;
        parlGame_incTurn(g);
    }
    else
    {
        g->mode = PM_CHOOSE_FIRST_LAST_MODE;
        g->turn = g->pmPosition;
    }
}

bool parlGame_removeFromHand(ParlGame* const g, const ParlStack s)
{
    if(!parlGame_removeFromHandOf(g, s, g->turn))
        return false;

    DECREASE_HAND_SIZE(parlStackSize(s));
    return true;
}

bool parlGame_removeFromHandOf(ParlGame* const g, const ParlStack s, const ParlPlayer p)
{
    if(!parlGame_handContains(g, s))
        return false;

    parlRemoveCardsPartial(&g->knownHands[p], s);
    parlRemoveCardsPartial(&g->faceDownCards, s);

    return true;
}

bool parlGame_moveFromHandTo(ParlGame* const g, ParlStack* const dest, const ParlStack s)
{
    if(!parlGame_removeFromHand(g, s))
        return false;

    *dest += s;
    return true;
}

void parlGame_confirmImpeachedMp(ParlGame* const g)
{
    parlMoveCards(&g->discard, &g->parliament, PARL_CARD(g->impeachedMpIdx));
    g->parliament |= PARL_CARD(g->cardToBeatIdx);
    parlGame_revertToNormalModeAndTurn(g);
    parlGame_incTurn(g);
}

ParlCallingCardOrigin parlGame_cardOrigin(const ParlGame* const g, const ParlIdx i)
{
    if(g->pmCardIdx == i)
        return FROM_CABINET;

    const register ParlStack c = PARL_CARD(i);

    if(PARL_CONTAINS(g->cabinet, c))
        return FROM_PARL;

    if((g->faceDownCards & c) || (g->knownHands[g->turn] & c))
        return FROM_HAND;

    return UNKNOWN;
}