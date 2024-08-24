//
// Created by Weiju Wang on 8/17/24.
//

/**
 * @file
 * @brief An API for the card game Parliament.
 */

/*
 * TODO Possible future optimizations:
 * - Make knownHands the same size every time so we don't need to call `calloc`, which is slow afaik
 * - Get rid of modes, just precompute legal moves at the end of every action
 */

#ifndef PARLIAMENT_GAME_H
#define PARLIAMENT_GAME_H

#include "cards.h"

#define PARL_NO_PM -1

#define PARL_NO_ARG -1

/**
 * The maximum number of cards a player is allowed to have in their hand.
 */
#define PARL_MAX_CARDS_IN_HAND 6

/**
 * The maximum number of players in one game.
 */
#define PARL_MAX_NUM_PLAYERS 16

/**
 * The width, in bits, of a player ID.
 */
#define PARL_PLAYER_WIDTH 4

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
 * Iterates through all player indices in the game.
 */
#define PARL_FOREACH_PLAYER(g, p) for(register ParlPlayer p = 0; p < (g)->numPlayers; ++p)

/**
 * The index of a player.
 */
typedef int ParlPlayer;

/**
 * Used during elections called by the PM to keep track of where cards came from so that they can be properly removed or
 * returned depending on the result.
 */
typedef enum
{
    UNKNOWN, FROM_HAND, FROM_CABINET, FROM_PARL
} ParlCallingCardOrigin;

/**
 * An action that can be taken in a Parliament game.
 */
typedef enum ParlAction
{
    /**
     * In NORMAL_MODE: always legal. Activates DISCARD_AFTER_DRAW_MODE mode if the player's hand size is 7 after drawing.
     * No arguments.
     */
    DRAW,

    /**
     * In NORMAL_MODE: legal if it's the known player's turn. Activates DISCARD_AFTER_DRAW mode if the player's hand
     * size is 7 after drawing. `idxA` is the card drawn.
     */
    SELF_DRAW,

    /**
     * In NORMAL_MODE: legal if the player has at least one card in their hand.
     * `idxA` is the discarded card.
     * In DISCARD_AFTER_DRAW_MODE: The only legal action. Activates NORMAL_MODE.
     */
    DISCARD,

    /**
     * In NORMAL_MODE: legal if the player has at least one card in their hand and Parliament is not full. `idxA` is the
     * appointed MP.
     */
    APPOINT_MP,

    /**
     * In NORMAL_MODE: legal if the player has at least three cards in their hand. If it's the known player's turn,
     * they must also have three cards in their hand of the same suit. Activates ELECTION_MODE. `idxA` is the PM
     * candidate, and `idxB` and `idxC` are the calling cards. No calling cards are actually moved anywhere until the
     * election is over.
     */
    CALL_ELECTION,

    /**
     * In NORMAL_MODE: legal if the player has at least one card in their hand of higher rank than at least one MP.
     * Activates IMPEACH_MODE. `idxA` is the MP being impeached and `idxB` is the card replacing it.
     */
    IMPEACH_MP,

    /**
     * In NORMAL_MODE: legal if there is a PM and the player has at least one card in their hand. If it's the known
     * player's turn, they must also have either a) a king if the PM card is a king, or b) a card higher than the PM.
     * If Cabinet had at least 2 cards before this was played, REPLACE_PM is activated. `idxA` is the card used to
     * impeach.
     */
    IMPEACH_PM,

    /**
     * In NORMAL_MODE: legal if the player has at least three cards in their hand. If it's the known player's turn, all
     * three cards must also be of the same rank, and one of them must be of the tied plurality suit. `idxA`, `idxB`,
     * and `idxC` are the cards played.
     */
    VOTE_NO_CONF,

    /**
     * In NORMAL_MODE mode: legal if the player is the PM, Cabinet isn't empty, and Parliament isn't empty. `idxA` is the
     * Cabinet card and `idxB` is the MP.
     */
    CABINET_RESHUFFLE,

    /**
     * In NORMAL_MODE: legal if the player is the PM and Cabinet isn't empty. `idxA` is the Cabinet member that should
     * be swapped with the PM.
     */
    APPOINT_PM,

    REIMPEACH,

    BLOCK_IMPEACH,

    NO_REIMPEACH,

    NO_BLOCK_IMPEACH,

    CONTEST_ELECTION,

    NO_CONTEST_ELECTION,

    /**
     * In BACKUP_PM_MODE mode: The only legal action. Activates NORMAL_MODE mode and reverts the turn to whoever was going to go
     * next after the PM impeachment.
     */
    APPOINT_BACKUP_PM,

    ENDGAME_PM_FIRST,

    ENDGAME_PM_LAST,

    /**
     * In ENDGAME mode: `idxA` is the PM candidate; if it's the PM's turn then no args.
     */
    ENDGAME_TRY_FORMATION,

    ENDGAME_PASS_FORMATION,

    ENDGAME_BLOCK_COALITION,

    ENDGAME_COUNTER_BLOCK_COALITION,

    ENDGAME_NO_BLOCK_COALITION,

    ENDGAME_NO_COUNTER_BLOCK_COALITION,
} ParlAction;

/**
 * @brief A Parliament game state from one player's perspective, referred to throughout the code as the "known player".
 */
typedef struct ParlGame
{
    /**
     * The number of players in the game.
     */
    ParlPlayer numPlayers : 5;

    /**
     * The number of turns away from the first turn that the known player sits.
     */
    ParlPlayer myPosition : PARL_PLAYER_WIDTH;

    /* Essentials -- required to enforce rules */

    /**
     * The index of the player whose turn it is.
     */
    ParlPlayer turn : PARL_PLAYER_WIDTH;

    /**
     * The number of turns away from the first turn that the PM sits, or `PARL_NO_PM` if there is no PM.
     */
    ParlPlayer pmPosition : PARL_PLAYER_WIDTH;

    /**
     * The PM card, or `PARL_NO_PM` if there is no PM.
     */
    ParlIdx pmCardIdx : PARL_IDX_WIDTH;

    /**
     * The Cabinet.
     */
    ParlStack cabinet;

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
    unsigned int drawDeckSize : 6;

    /**
     * In most cases, this is set to `NORMAL_MODE`. When there is a move awaiting a response, like an impeachment or election
     * call, it will be set to one of the other modes.
     */
    enum ParlGameMode
    {
        /**
         * Active when the last impeachment, election, etc. was resolved and it's now the next player's turn.
         */
        NORMAL_MODE,

        /**
         * Active when a player has drawn their 10th card and must now discard 1.
         */
        DISCARD_AFTER_DRAW_MODE,

        /**
         * Active when all impeachments attempts in the chain have been blocked so far.
         */
        REIMPEACH_MODE,

        /**
         * Active when the next card to be played in an impeachment chain is a blocking card.
         */
        BLOCK_IMPEACH_MODE,

        /**
         * Active when a player called an election earlier and is now waiting for opposition.
         */
        ELECTION_MODE,

        /**
         * Active when the PM card has been impeached and the PM must choose between 2 or more Cabinet members to replace the PM card.
         */
        BACKUP_PM_MODE,

        ENDGAME_MODE,

        PM_CHOOSE_FIRST_LAST_MODE,

        BLOCK_COALITION_MODE,

        COUNTER_BLOCK_COALITION_MODE,

        GAME_OVER
    } mode : 4;

    unsigned int coalitionSize : 6;

    bool endgameSkipPm : 1;

    /**
     * The player that will had the turn before this impeachment or election chain started.
     */
    ParlPlayer currNormalTurn : PARL_PLAYER_WIDTH;

    /**
     * The card to beat in the stack.
     */
    ParlIdx cardToBeatIdx : PARL_IDX_WIDTH;

    /**
     * The MP that was originally impeached.
     */
    ParlIdx impeachedMpIdx: PARL_IDX_WIDTH;

    /**
     * The player who called this election or was the first to play in endgame.
     */
    ParlPlayer cycleStarter : PARL_PLAYER_WIDTH;

    /**
     * A dynamically allocated array of all the candidates in this election.
     */
    struct ParlElectionCand {
        ParlStack callingCards;
        ParlIdx pmIdx : PARL_IDX_WIDTH;
        ParlIdx preCallNumCards : PARL_IDX_WIDTH;
    } *elecCands;

    /* Derived information */

    /**
     * The size of each player's hand.
     */
    int8_t handSizes[PARL_MAX_NUM_PLAYERS];

    /**
     * Cards that are known to be in certain players' hands.
     */
    ParlStack* knownHands;

    /**
     * All cards that are either in the draw pile or someone's hand. These are combined since we can't see either of them.
     */
    ParlStack faceDownCards;
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
 * @param g
 * @param s
 * @return Whether the hand of the player whose turn it is contains `s` completely.
 */
bool parlGame_handContains(const ParlGame* g, ParlStack s);

/**
 * @brief Runs this line of code: g->turn = PARL_NEXT_TURN(g);
 * @note For internal use only. Do not call.
 * @param g
 */
void parlGame_incTurn(ParlGame* const g);

/**
 * Runs parlGame_incTurn and runs it again if the PM needs to be skipped.
 * @param g
 */
void parlGame_incTurnEndgame(ParlGame* const g);

/**
 * @brief Runs this line of code: g->currNormalTurn = g->turn;
 * @note For internal use only. Do not call.
 * @param g
 */
void parlGame_saveNormalTurn(ParlGame *const g);

/**
 * @brief Runs these lines of code:
 * g->mode = NORMAL_MODE;
 * g->turn = g->currNormalTurn;
 * @note For internal use only. Do not call.
 * @param g
 */
void parlGame_revertToNormalModeAndTurn(ParlGame *const g);

/**
 * Moves the game state to endgame.
 * @note For internal use only. Do not call.
 * @param g
 */
void parlGame_moveToEndgame(ParlGame* g);

/**
 * @brief Removes `s` from the hand of the player whose turn it is.
 * @note For internal use only. Do not call.
 * @param g
 * @param s
 * @return Whether the operation was successful.
 */
bool parlGame_removeFromHand(ParlGame* g, ParlStack s);

/**
 * @brief Removes `s` from the hand of the specified player.
 * @note For internal use only. Do not call.
 * @param g
 * @param s
 * @param p
 * @return Whether the operation was successful.
 */
bool parlGame_removeFromHandOf(ParlGame* g, ParlStack s, ParlPlayer p);

/**
 * @brief Moves `s` from the hand of the player whose turn it is to `dest`.
 * @note For internal use only. Do not call.
 * @param g
 * @param dest
 * @param s
 * @return Whether the operation was successful.
 */
bool parlGame_moveFromHandTo(ParlGame* g, ParlStack* dest, ParlStack s);

/**
 * @brief Ends the current impeachment stack.
 * @note For internal use only. Do not call.
 * @param g
 */
void parlGame_confirmImpeachedMp(ParlGame* g);

/**
 * @brief Used in elections to obtain the location of the PM's calling cards that may have to be moved around later.
 * @note For internal use only. Do not call.
 * @param g
 * @param i
 * @return The location of the card `i`.
 */
ParlCallingCardOrigin parlGame_cardOrigin(const ParlGame* g, ParlIdx i);

#endif //PARLIAMENT_GAME_H
