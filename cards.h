//
// Created by Weiju Wang on 8/17/24.
//

/**
 * @file
 * @brief An API for performing operations on/with stacks of cards in games where all jokers are considered equivalent.
 *
 * @details
 * This API is extremely fast compared to many other card libraries because it stores each deck of cards as a single
 * 64-bit integer. This means there is no dynamic memory allocation anywhere in this API and all operations are bitwise
 * or arithmetic.
 * Internally, this is implemented by treating each card in a stack of cards (ParlStack = uint_64) as a flag that can be
 * set to indicate the card exists in the stack. The 52 rightmost bits in the integer indicate the status of the 52
 * non-joker cards. The number of jokers in the card can be obtained with PARL_NUM_JOKERS(), which simply right-shifts
 * the 52 non-joker bits away. Jokers have to be implemented like this because they are not unique. This API would be
 * much faster if the different jokers were considered to be different cards.
 *
 * TODO I haven't bothered to explain how to use any of the code in here -- would be helpful
 */

#ifndef PARLIAMENT_CARDS_H
#define PARLIAMENT_CARDS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * The number of jokers present in a `ParlStack` will be shifted to this position.
 *
 * This is also always the number of non-joker cards in the deck.
 */
#define PARL_JOKER_IDX 52

#define PARL_JOKER_CARD (PARL_CARD(PARL_JOKER_IDX))

#define PARL_NUM_NON_JOKER_CARDS PARL_JOKER_IDX
#define PARL_NUM_SUITS PARL_JOKER_SUIT
#define PARL_NUM_RANKS PARL_JOKER_RANK

#define PARL_ACE_RANK 0
#define PARL_TEN_RANK 9
#define PARL_JACK_RANK 10
#define PARL_QUEEN_RANK 11
#define PARL_KING_RANK 12
#define PARL_JOKER_RANK 13

#define PARL_ACE_SYMBOL 'a'
#define PARL_TEN_SYMBOL 'x'
#define PARL_JACK_SYMBOL 'j'
#define PARL_QUEEN_SYMBOL 'q'
#define PARL_KING_SYMBOL 'k'

#define PARL_PARSE_ERROR (-1)

/**
 * Returns a stack of empty cards.
 */
#define PARL_EMPTY_STACK 0ull

/**
 * The width of a ParlCardSymbol in bytes.
 */
#define PARL_SYMBOL_WIDTH 2

/**
 * The minimum width of a ParlIdx in bits. The maximum ParlIdx is 54 and 6 bits are needed to store that.
 */
#define PARL_IDX_WIDTH 6

/**
 * Returns a complete deck without jokers.
 */
#define PARL_COMPLETE_STACK_NO_JOKERS (PARL_JOKER_CARD - 1)

/**
 * Returns the stack only containing the card of the given index.
 */
#define PARL_CARD(i) (1ull << (i))

/**
 * Returns a card index given a card's rank and suit.
 */
#define PARL_RS_TO_IDX(r, s) ((s) * PARL_NUM_RANKS + (r))

/**
 * Returns a card given a card's rank and suit.
 */
#define PARL_RS_TO_CARD(r, s) (PARL_CARD(PARL_RS_TO_IDX((r), (s))))

/**
 * Obtains a card's rank from its index.
 */
#define PARL_RANK(i) (PARL_IS_JOKER(i) ? PARL_JOKER_RANK : i % PARL_NUM_RANKS)

/**
 * Obtains a card's suit from its index.
 */
#define PARL_SUIT(i) ((i) / PARL_NUM_RANKS)

/**
 * Returns whether the card of the given index is a joker.
 */
#define PARL_IS_JOKER(i) ((i) >= PARL_JOKER_IDX)

/**
 * Returns whether i0 is of a higher rank than i1.
 */
#define PARL_HIGHER_THAN(i0, i1) (PARL_RANK(i0) > PARL_RANK(i1))

/**
 * Returns a stack with no jokers.
 */
#define PARL_WITHOUT_JOKERS(s) ((s) & PARL_COMPLETE_STACK_NO_JOKERS)

/**
 * Returns whether s0 contains the entirety of s1.
 */
#define PARL_CONTAINS(s0, s1) ( \
    ( /* Excluding jokers, s0 and s1 share at least one card, or s1 is empty */ \
        PARL_WITHOUT_JOKERS((s0) & (s1)) \
        || !PARL_WITHOUT_JOKERS(s1) \
    ) && PARL_NUM_JOKERS(s0) >= PARL_NUM_JOKERS(s1))

/**
 * Filter a stack to only cards of a given suit.
 */
#define PARL_FILTER_SUIT(s, suit) ((s) & ( \
    PARL_CARD(PARL_NUM_RANKS * ((suit) + 1)) - PARL_CARD(PARL_NUM_RANKS * (suit)) \
    ))

/**
 * Returns the number of jokers in a stack.
 */
#define PARL_NUM_JOKERS(s) ((s) >> PARL_JOKER_IDX)

/**
 * Iterates through all 4 suits, not including the joker suit.
 */
#define PARL_FOREACH_SUIT(s) for(register ParlSuit s = 0; s < PARL_NUM_SUITS; ++s)

/**
 * Iterates through all ranks starting from the given rank, not including the joker rank.
 */
#define PARL_FOREACH_RANK_FROM(r, start) for(register ParlRank r = start; r < PARL_NUM_RANKS; ++r)

/**
 * Iterates through all 13 ranks, not including the joker rank.
 */
#define PARL_FOREACH_RANK(r) PARL_FOREACH_RANK_FROM(r, 0)

/**
 * Iterates through all 52 non-joker card indices.
 */
#define PARL_FOREACH_IDX(i) for(register ParlIdx i = 0; i < PARL_NUM_NON_JOKER_CARDS; ++i)

/**
 * Iterates through all indices of all cards in the stack `s`.
 */
#define PARL_FOREACH_IN_STACK(s, i) \
    PARL_FOREACH_IDX(i) \
        if(PARL_CONTAINS((s), PARL_CARD(i)))
/**
 * An integer that uniquely identifies a card.
 *
 * More precisely, each `ParlIdx` is the position of a bit flag in a `ParlStack` that indicates whether the card is
 * in the stack.
 *
 * Jokers are complicated; see `PARL_JOKER_IDX`. Any `ParlIdx` higher than 52 is treated as a joker for simplicity.
 */
typedef unsigned int ParlIdx;

/**
 * A sequence of two alphanumeric characters that uniquely identify each card.
 *
 * The first letter is the rank, indexed from 1. 'a' is ace, 'x' is 10, 'j' is jack', 'q' is queen, and 'k' is king.
 * The second letter is the first letter of the suit in lowercase -- e.g. 's' for spades.
 * Jokers are 'zz'.
 */
typedef char ParlCardSymbol[PARL_SYMBOL_WIDTH];

/**
 * Ranks are zero-indexed from 0 to 12 inclusive.
 */
typedef unsigned int ParlRank;

/**
 * A collection of cards. There is only one of each card in play at any time except for jokers, of which there can be
 * any number.
 *
 * For non-joker cards, if the bit at the nth position in a `ParlStack` is set, it means that card exists in the
 * stack.
 *
 * Use the method `parlJokerCount` to find the number of jokers in a `ParlStack`.
 */
typedef uint64_t ParlStack;

/**
 * The suit of a `ParlIdx`.
 */
typedef enum
{
    INVALID_SUIT = -1,
    CLUBS, SPADES,
    HEARTS, DIAMONDS,
    PARL_JOKER_SUIT,
} ParlSuit;

/**
 * The symbol that represents a joker.
 */
const ParlCardSymbol PARL_JOKER_SYMBOL;

/**
 * @param s
 * @return The number of cards in the stack `s`.
 */
int parlStackSize(ParlStack s);

/**
 * Removes `cards` from `orig` only if the cards all exist in `orig`.
 * @param orig
 * @param cards
 * @return Whether the operation was successful.
 */
bool parlRemoveCards(ParlStack* orig, ParlStack cards);

/**
 * Removes all `cards` from `orig` that exist in `orig`. Assumes `*orig` has at least as many jokers as `cards`.
 * @param orig
 * @param cards
 */
void parlRemoveCardsPartial(ParlStack* orig, ParlStack cards);

/**
 * Moves `cards` from `dest` to `orig` only if the cards already exist in `orig`.
 *
 * Assumes the card doesn't already exist in `dest`, which would mean something's gone terribly wrong.
 *
 * @param dest
 * @param orig
 * @param cards
 * @return `true` if the card `idx` existed in `orig` and thus the move was executed.
 */
bool parlMoveCards(ParlStack* dest, ParlStack* orig, ParlStack cards);

/**
 * Writes the symbol of card `idx` to `out`.
 * @param out The ParlCardSymbol to which the card's symbol should be outputted.
 * @param idx
 */
void parlCardSymbol(ParlCardSymbol out, ParlIdx idx);

/**
 * @param symbol
 * @return Returns the `ParlIdx` corresponding to `symbol`, or `PARL_PARSE_ERROR` if `symbol` is not a valid card symbol.
 */
ParlIdx parlSymbolToIdx(const ParlCardSymbol symbol);

#endif //PARLIAMENT_CARDS_H
