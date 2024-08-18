//
// Created by Weiju Wang on 8/17/24.
//

#ifndef PARLIAMENT_CARDS_H
#define PARLIAMENT_CARDS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * The number of jokers present in a `ParlCardStack` will be shifted to this position.
 */
#define PARL_JOKER_IDX 52

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

#define PARL_PARSE_ERROR -1

/**
 * Returns a stack of empty cards.
 */
#define PARL_EMPTY_STACK 0u

/**
 * Returns the stack only containing the card of the given index.
 */
#define PARL_CARD(i) (1 << i)

/**
 * Obtains a card's suit from its index.
 */
#define PARL_SUIT(i) (i / PARL_NUM_RANKS)

/**
 * Returns whether the card of the given index is a joker.
 */
#define PARL_IS_JOKER(i) (i >= PARL_JOKER_IDX)

/**
 * Returns whether i0 is of a higher rank than i1.
 */
#define PARL_HIGHER_THAN(i0, i1) (parlRank(i0) > parlRank(i1))

/**
 * Returns whether a stack contains a card of a given index.
 */
#define PARL_STACK_CONTAINS(s, idx) (s & PARL_CARD(idx))

/**
 * Returns the number of jokers in a stack.
 */
#define PARL_JOKER_COUNT(s) (s >> PARL_JOKER_IDX)

/**
 * An integer that uniquely identifies a card.
 *
 * More precisely, each `ParlCardIdx` is the position of a bit flag in a `ParlCardStack` that indicates whether the card is
 * in the stack.
 *
 * Jokers are complicated; see `PARL_JOKER_IDX`. Any `ParlCardIdx` higher than 52 is treated as a joker for simplicity.
 */
typedef int ParlCardIdx;

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
 * For non-joker cards, if the bit at the nth position in a `ParlCardStack` is set, it means that card exists in the
 * stack.
 *
 * Use the method `parlJokerCount` to find the number of jokers in a `ParlCardStack`.
 */
typedef uint64_t ParlCardStack;

/**
 * The suit of a `ParlCardIdx`.
 */
typedef enum {
    CLUBS = 0,
    SPADES = 1,
    HEARTS = 2,
    DIAMONDS = 3,
    PARL_JOKER_SUIT = 4,

    /**
     * This serves to indicate to the compiler that this enum type must be signed.
     */
    INVALID_SUIT = -1
} ParlSuit;

/**
 * @param idx
 * @return The rank of card `idx`.
 */
ParlRank parlRank(ParlCardIdx idx);

/**
 * The symbol that represents a joker.
 */
const ParlCardSymbol PARL_JOKER_SYMBOL;

/**
 * Writes the symbol of card `idx` to `out`.
 * @param out The ParlCardSymbol to which the card's symbol should be outputted.
 * @param idx
 * @return `true` if `idx` was a valid card.
 */
bool parlCardSymbol(ParlCardSymbol out, ParlCardIdx idx);

/**
 * @param symbol
 * @return Returns the `ParlCardIdx` corresponding to `symbol`, or `PARL_PARSE_ERROR` if `symbol` is not a valid card symbol.
 */
ParlCardIdx parlCardIdx(ParlCardSymbol symbol);

#endif //PARLIAMENT_CARDS_H
