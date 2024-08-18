//
// Created by Weiju Wang on 8/17/24.
//

#include "cards.h"

#include <string.h>

const ParlCardSymbol PARL_JOKER_SYMBOL = "zz";

const char PARL_SUIT_SYMBOLS[5] = "cshdj";

ParlRank parlRank(ParlCardIdx idx)
{
    if(PARL_IS_JOKER(idx))
        return PARL_JOKER_RANK;
    else
        return idx % PARL_NUM_RANKS;
}

bool parlCardSymbol(ParlCardSymbol out, ParlCardIdx idx)
{
    ParlRank r = parlRank(idx);

    if(idx < PARL_JOKER_IDX && idx >= 0) {
        // Non-joker card
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
    } else if (PARL_IS_JOKER(idx)) {
        // memcpy instead of strcpy or related functions b/c no null terminator
        memcpy(out, PARL_JOKER_SYMBOL, 2);
        return true;
    } else return false;
}

ParlCardIdx parlCardIdx(ParlCardSymbol symbol)
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
    for(int i = 0; i < sizeof PARL_SUIT_SYMBOLS; ++i)
    {
        if(PARL_SUIT_SYMBOLS[i] == symbol[1])
        {
            suit = i;
            break;
        }
    }

    if(rank < 0 || suit < 0)
        return PARL_PARSE_ERROR;
    else
        return suit * PARL_NUM_RANKS + rank;
}