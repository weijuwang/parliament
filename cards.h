//
// Created by Weiju Wang on 8/17/24.
//

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

#define PARL_NUM_SUITS PARL_JOKER_SUIT
#define PARL_NUM_RANKS 13

#define PARL_ACE_RANK 0
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
#define PARL_HIGHER_THAN(i0, i1) (parlRank(i0) > parlRank(i1))

/**
 * Returns whether s0 contains the entirety of s1.
 */
#define PARL_STACK_CONTAINS(s0, s1) ( \
    ( /* Excluding jokers, s0 and s1 share at least one card, or s1 is empty */ \
        ((s0) & (s1) & PARL_COMPLETE_STACK_NO_JOKERS) \
        || !((s1) & PARL_COMPLETE_STACK_NO_JOKERS) \
    ) && PARL_NUM_JOKERS(s0) >= PARL_NUM_JOKERS(s1))

/**
 * Filter a stack to only cards of a given suit.
 */
#define PARL_STACK_FILTER_SUIT(s, suit) ((s) & (PARL_CARD(suit + 1) - PARL_CARD(suit)))

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
#define PARL_FOREACH_IDX(i) for(register ParlIdx i = 0; i < PARL_JOKER_IDX; ++i)

/**
 * Iterates through all indices of all cards in the stack `s`.
 */
#define PARL_FOREACH_IN_STACK(s, i) \
    PARL_FOREACH_IDX(i) \
        if(PARL_STACK_CONTAINS((s), PARL_CARD(i)))
/**
 * An integer that uniquely identifies a card.
 *
 * More precisely, each `ParlIdx` is the position of a bit flag in a `ParlStack` that indicates whether the card is
 * in the stack.
 *
 * Jokers are complicated; see `PARL_JOKER_IDX`. Any `ParlIdx` higher than 52 is treated as a joker for simplicity.
 */
typedef int ParlIdx;

/**
 * A sequence of two alphanumeric characters that uniquely identify each card.
 *
 * The first letter is the rank, indexed from 1. 'a' is ace, 'x' is 10, 'j' is jack', 'q' is queen, and 'k' is king.
 * The second letter is the first letter of the suit in lowercase -- e.g. 's' for spades.
 * Jokers are 'zz'.
 */
typedef char ParlCardSymbol[2];

/**
 * Ranks are zero-indexed from 0 to 12 inclusive.
 */
typedef int ParlRank;

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
typedef enum {
    CLUBS, SPADES,
    HEARTS, DIAMONDS,
    PARL_JOKER_SUIT,

    /**
     * This serves to indicate to the compiler that this enum type must be signed.
     */
    INVALID_SUIT = -1
} ParlSuit;

/**
 * The symbol that represents a joker.
 */
const ParlCardSymbol PARL_JOKER_SYMBOL;

/**
 * @param idx
 * @return The rank of card `idx`.
 */
ParlRank parlRank(ParlIdx idx);

/**
 * @param s
 * @return The number of cards in the stack `s`.
 */
int parlStackSize(ParlStack s);

/**
 * Moves `card` from `dest` to `orig` only if the cards already exist in `orig`.
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
 * @return `true` if `idx` was a valid card.
 */
bool parlCardSymbol(ParlCardSymbol out, ParlIdx idx);

/**
 * @param symbol
 * @return Returns the `ParlIdx` corresponding to `symbol`, or `PARL_PARSE_ERROR` if `symbol` is not a valid card symbol.
 */
ParlIdx parlCardIdx(const ParlCardSymbol symbol);

#endif //PARLIAMENT_CARDS_H
