//
// Created by Weiju Wang on 8/17/24.
//

#ifndef PARLIAMENT_GAME_H
#define PARLIAMENT_GAME_H

#include "cards.h"

#define PARL_NO_PM -1

#define PARL_NO_ARG -1

/**
 * The maximum number of cards a player is allowed to have in their hand.
 */
#define PARL_MAX_CARDS_IN_HAND 9

/**
 * Returns the known player's hand.
 */
#define PARL_MY_HAND(g) (g->knownHands[g->myPosition])

/**
 * Returns whether it is the known player's turn to move.
 */
#define PARL_MY_TURN(g) (g->turn == g->myPosition)

/**
 * Returns the index of the player who is due to play next.
 */
#define PARL_NEXT_TURN(g) ((g)->turn >= (g)->numPlayers - 1 ? 0 : (g)->turn + 1)

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
     * If it's the known player's turn, `idxA` is the drawn card; otherwise no arguments.
     */
    DRAW,

    /**
     * In NORMAL mode: legal if the player has at least one card in their hand.
     * `idxA` is the discarded card.
     * In DISCARD_AFTER_DRAW mode: The only legal action. Activates NORMAL mode.
     */
    DISCARD,

    /**
     * In NORMAL mode: legal if the player has at least one card in their hand and Parliament is not full. `idxA` is the
     * appointed MP.
     */
    APPOINT_MP,

    /**
     * In NORMAL mode: legal if the player has at least three cards in their hand. If it's the known player's turn,
     * they must also have three cards in their hand of the same suit. Activates ELECTION mode. `idxA` is the PM
     * candidate, and `idxB` and `idxC` are the calling cards.
     */
    ENTER_ELECTION,

    /**
     * In NORMAL mode: legal if the player has at least one card in their hand of higher rank than at least one MP.
     * Activates IMPEACH mode. `idxA` is the MP being impeached and `idxB` is the card replacing it.
     */
    IMPEACH_MP,

    /**
     * In NORMAL mode: legal if there is a PM and the player has at least one card in their hand. If it's the known
     * player's turn, they must also have either a) a king if the PM card is a king, or b) a card higher than the PM.
     * If Cabinet had at least 2 cards before this was played, REPLACE_PM mode is activated. `idxA` is the card used to
     * impeach.
     */
    IMPEACH_PM,

    /**
     * In NORMAL mode: legal if the player has at least three cards in their hand. If it's the known player's turn, all
     * three cards must also be of the same rank, and one of them must be of the tied plurality suit. `idxA`, `idxB`,
     * and `idxC` are the cards played.
     */
    VOTE_NO_CONF,

    /**
     * In NORMAL mode: legal if the player is the PM, Cabinet isn't empty, and Parliament isn't empty. `idxA` is the
     * Cabinet card and `idxB` is the MP.
     */
    CABINET_RESHUFFLE,

    /**
     * In NORMAL mode: legal if the player is the PM and Cabinet isn't empty. `idxA` is the Cabinet member that should
     * be swapped with the PM.
     */
    APPOINT_PM,

    BLOCK_IMPEACH,

    REIMPEACH,

    ALLOW_IMPEACH,

    CONTEST_ELECTION,

    NO_CONTEST_ELECTION,

    /**
     * In BACKUP_PM mode: The only legal action. Activates NORMAL mode and reverts the turn to whoever was going to go
     * next after the PM impeachment.
     */
    APPOINT_BACKUP_PM
} ParlAction;

/**
 * @brief A Parliament game state from one player's perspective, referred to throughout the code as the "known player".
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
    ParlIdx pmCardIdx;

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
     * The discard pile.
     */
    ParlStack discard;

    /**
     * The size of the draw deck.
     */
    int drawDeckSize;

    /**
     * In most cases, this is set to `NORMAL`. When there is a move awaiting a response, like an impeachment or election
     * call, it will be set to one of the other modes.
     */
    enum ParlGameMode {
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

    /**
     * The player that will have the turn after this impeachment or election is resolved.
     */
    ParlPlayer nextNormalTurn;

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
 * @brief Initialize the game from midgame.
 * @param g The g to initialize.
 * @param numJokers The number of jokers in the deck.
 * @param numPlayers Same as the field.
 * @param myPosition Same as the field.
 * @param myFirstCardIdx The first card in the known player's hand, obtained from pregame.
 * @return Whether the initialization was successful.
 */
bool parlGame_init(ParlGame* g,
                   int numJokers,
                   int numPlayers,
                   ParlPlayer myPosition,
                   ParlIdx myFirstCardIdx);

/**
 * @brief Free memory allocated for a `ParlGame`, not including the `ParlGame` struct itself.
 * @note If you dynamically allocated memory to store the `ParlGame` itself, you must `free` it separately.
 * @param g
 */
void parlGame_free(const ParlGame* g);

/**
 * @brief Deep-copies a ParlGame from `orig` to `dest`.
 * @param dest
 * @param orig
 * @return Whether the initialization was successful.
 */
bool parlGame_deepCopy(ParlGame* dest, const ParlGame* orig);

/**
 * @param g
 * @return All legal actions in the game `g` in the current state, where a bit's index corresponds to its ID as a
 * `ParlAction` and a set bit means the move is legal.
 * @note For actions that are only legal when cards in hidden hands are available, they are assumed to be legal.
 */
unsigned int parlGame_legalActions(const ParlGame* g);

/**
 * @param g
 * @return The tied plurality suits, with the indices of set bits being the indices of tied plurality suits.
 */
unsigned int parlGame_tiedPluralities(const ParlGame* g);

/**
 * @param g
 * @return The plurality suit, or `INVALID_SUIT` if two or more suits are tied. In that case, use
 * `parlGame_tiedPluralities`.
 */
ParlSuit parlGame_plurality(const ParlGame* g);

/**
 * @brief Applies the action `a` onto `g`. The caller is responsible for checking with `parlGame_legalActions()` that
 * the action `a` is legal.
 * @note An action that is supposedly legal may still be unable to be executed (thus causing this method to return
 * false) if the arguments are invalid.
 * @note If you supply extra arguments that are not needed for the specified action, they will not be read, but you
 * should still use PARL_NO_ARG.
 *
 * @param g
 * @param a
 * @param idxA
 * @param idxB
 * @param idxC
 * @return Whether the action is legal with the specified arguments.
 */
bool parlGame_applyAction(ParlGame* g,
                          ParlAction a,
                          ParlIdx idxA,
                          ParlIdx idxB,
                          ParlIdx idxC
                          );

/**
 * @note For internal use only. Do not call.
 * @brief Removes `s` from the hand of the player whose turn it is.
 * @param g
 * @param s
 */
void parlGame_removeFromHand(ParlGame* g, ParlStack s);

#endif //PARLIAMENT_GAME_H
