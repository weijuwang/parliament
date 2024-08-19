//
// Created by Weiju Wang on 8/17/24.
//

#include "cards.h"

#include <string.h>

const ParlCardSymbol PARL_JOKER_SYMBOL = "zz";

const char PARL_SUIT_SYMBOLS[5] = "cshdj";

ParlRank parlRank(const ParlIdx idx)
{
    if(PARL_IS_JOKER(idx))
        return PARL_JOKER_RANK;
    else
        return idx % PARL_NUM_RANKS;
}

int parlStackSize(const ParlStack s)
{
    register int numNonJokers = 0;

    PARL_FOREACH_IDX(i)
        if(PARL_STACK_CONTAINS(s, PARL_CARD(i)))
            ++numNonJokers;

    return numNonJokers + (int)PARL_NUM_JOKERS(s);
}

bool parlMoveCards(ParlStack* const dest, ParlStack* const orig, const ParlStack cards)
{
    if(!PARL_STACK_CONTAINS(*orig, cards))
        return false;

    *orig -= PARL_CARD(cards);
    *dest += PARL_CARD(cards);

    return true;
}

bool parlCardSymbol(ParlCardSymbol out, const ParlIdx idx)
{
    ParlRank r = parlRank(idx);

    // Non-joker card
    if(idx < PARL_JOKER_IDX && idx >= 0)
    {
        switch(r)
        {
#define PARLIAMENT_CARDS_SPECIAL_RANK_OUTPUT(rank) \
            case PARL_##rank##_RANK: \
                out[0] = PARL_##rank##_SYMBOL; \
                break;
            PARLIAMENT_CARDS_SPECIAL_RANK_OUTPUT(ACE)
            PARLIAMENT_CARDS_SPECIAL_RANK_OUTPUT(JACK)
            PARLIAMENT_CARDS_SPECIAL_RANK_OUTPUT(QUEEN)
            PARLIAMENT_CARDS_SPECIAL_RANK_OUTPUT(KING)
            default:
                out[0] = '0' + r;
        }

        out[1] = PARL_SUIT_SYMBOLS[PARL_SUIT(idx)];
        return true;
    }
    else if(PARL_IS_JOKER(idx))
    {
        // memcpy instead of strcpy or related functions b/c no null terminator
        memcpy(out, PARL_JOKER_SYMBOL, 2);
        return true;
    } else return false;
}

ParlIdx parlCardIdx(const ParlCardSymbol symbol)
{
    ParlRank rank;
    ParlSuit suit = PARL_PARSE_ERROR;

    if(strcmp(symbol, PARL_JOKER_SYMBOL) == 0)
        return PARL_JOKER_IDX;

    /* Parse rank */
    switch (symbol[0])
    {
#define PARLIAMENT_CARDS_SPECIAL_RANK_PARSE(specialRank) \
            case PARL_##specialRank##_SYMBOL: \
                rank = PARL_##specialRank##_RANK; \
                break;
        PARLIAMENT_CARDS_SPECIAL_RANK_PARSE(ACE)
        PARLIAMENT_CARDS_SPECIAL_RANK_PARSE(JACK)
        PARLIAMENT_CARDS_SPECIAL_RANK_PARSE(QUEEN)
        PARLIAMENT_CARDS_SPECIAL_RANK_PARSE(KING)
        default:
            rank = symbol[0] - '0';
    }

    /* Parse suit */
    for(register int i = 0; i < sizeof PARL_SUIT_SYMBOLS; ++i)
        if(PARL_SUIT_SYMBOLS[i] == symbol[1])
        {
            suit = i;
            break;
        }

    if(rank < 0 || suit < 0)
        return PARL_PARSE_ERROR;
    else
        return PARL_RS_TO_IDX(rank, suit);
}