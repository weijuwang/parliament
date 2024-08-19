//
// Created by Weiju Wang on 8/17/24.
//

#ifndef PARLIAMENT_GAME_H
#define PARLIAMENT_GAME_H

#include "cards.h"

#define PARL_NO_PM -1

/**
 * Returns the known player's hand.
 */
#define PARL_MY_HAND(g) (g->knownHands[g->myPosition])

/**
 * Returns whether it is the known player's turn to move.
 */
#define PARL_MY_TURN(g) (g->turn == g->myPosition)

/**
 * The index of a player.
 */
typedef int ParlPlayer;

/**
 * An action that can be taken in a Parliament game.
 */
typedef enum {
    /**
     * In NORMAL mode: always legal. Activates DISCARD_AFTER_DRAW mode if the player's hand size is 10 after drawing.
     */
    DRAW,

    /**
     * In NORMAL mode: legal if the player has at least one card in their hand.
     */
    DISCARD,

    /**
     * In NORMAL mode: legal if the player has at least one card in their hand and Parliament is not full.
     */
    APPOINT_MP,

    /**
     * Used both for calling and opposing elections depending on the mode.
     * Activates ELECTION mode when called in NORMAL mode.
     *
     * In NORMAL mode: legal if the player has at least three cards in their hand. If it's the known player's turn,
     * they must also have three cards in their hand of the same suit.
     */
    ENTER_ELECTION,

    /**
     * Used both for impeaching and blocking MPs depending on the mode.
     * Activates IMPEACH mode when called in NORMAL mode.
     */
    IMPEACH_MP,
    IMPEACH_PM,
    VOTE_NO_CONF,
    CABINET_RESHUFFLE,
    APPOINT_PM,
    PASS
} ParlAction;

/**
 * A Parliament game state from one player's perspective, referred to throughout the code as the "known player".
 */
typedef struct ParlGame {
    /**
     * The number of players in the game.
     */
    int numPlayers;

    /**
     * The number of turns away from the first turn that the known player sits.
     */
    ParlPlayer myPosition;

    /* Essentials -- required to enforce rules */

    /**
     * The number of turns away from the first turn that the PM sits, or `PARL_NO_PM` if there is no PM.
     */
    ParlPlayer pmPosition;

    /**
     * The PM card, or `PARL_NO_PM` if there is no PM.
     */
    ParlIdx pmCard;

    /**
     * The Cabinet.
     */
    ParlStack cabinet;

    /**
     * The index of the player whose turn it is.
     */
    ParlPlayer turn;

    /**
     * The Parliament.
     */
    ParlStack parliament;

    /**
     * The size of the draw deck.
     */
    int drawDeckSize;

    /**
     * In most cases, this is set to `NORMAL`. When there is a move awaiting a response, like an impeachment or election
     * call, it will be set to one of the other modes.
     */
    enum {
        /**
         * Active when the last impeachment, election, etc. was resolved and it's now the next player's turn.
         */
        NORMAL,

        /**
         * Active when a player has drawn their 10th card and must now discard 1.
         */
        DISCARD_AFTER_DRAW,

        /**
         * Active when a player impeached a card earlier and is now waiting for any block attempts.
         */
        IMPEACH,

        /**
         * Active when a player called an election earlier and is now waiting for opposition.
         */
        ELECTION,

        /**
         * Active when the PM card has been impeached and the PM must choose between 2 or more Cabinet members to replace the PM card.
         */
        BACKUP_PM
    } mode;

    /* Derived information */

    /**
     * The size of each player's hand.
     */
    int* handSizes;

    /**
     * Cards that are known to be in certain players' hands.
     */
    ParlStack* knownHands;

    /**
     * All cards that are either in the draw pile or someone's hand. These are combined since we can't see either of them.
     */
    ParlStack faceDownCards;

    /**
     * Temporary values that are saved through certain sequences of moves.
     */
    union {
        /**
         * Values used during MP impeachments.
         */
        struct {
            /**
             * The suit of the original MP. All blocks must be of this suit.
             */
            ParlSuit mpSuit;

            /**
             * The card to beat in the stack.
             */
            ParlIdx cardToBeat;

            /**
             * Whether the next action will be a block or another impeachment attempt.
             */
            bool awaitingBlock;
        } impeach;

        /**
         * Values used during elections, in addition to `callingCards`.
         *
         *
         */
        struct {

        } election;
    } temp;
} ParlGame;

/**
 * Initialize the g from midgame.
 * @param g The g to initialize.
 * @param numJokers The number of jokers in the deck.
 * @param numPlayers Same as the field.
 * @param myPosition Same as the field.
 * @param myFirstCard The first card in the known player's hand, obtained from pregame.
 * @return Whether the initialization was successful.
 */
bool parlGame_init(ParlGame* g,
                   int numJokers,
                   int numPlayers,
                   ParlPlayer myPosition,
                   ParlIdx myFirstCard);

/**
 * Free memory allocated for a `ParlGame`, not including the `ParlGame` struct itself.
 *
 * If you dynamically allocated memory to store the `ParlGame` itself, you must `free` it separately.
 * @param g
 */
void parlGame_free(const ParlGame* g);

/**
 * @param g
 * @return All legal actions in the game `g` in the current state, where a bit's index corresponds to its ID as a
 * `ParlAction` and a set bit means the move is legal.
 * @note For actions that are only legal when cards in hidden hands are available, they are assumed to be legal.
 */
unsigned int parlGame_legalActions(const ParlGame* g);

#endif //PARLIAMENT_GAME_H
