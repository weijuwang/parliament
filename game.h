//
// Created by Weiju Wang on 8/17/24.
//

#ifndef PARLIAMENT_GAME_H
#define PARLIAMENT_GAME_H

#include "cards.h"

#define PARL_NO_PM -1

/**
 * Obtains the known player's hand.
 */
#define PARL_MY_HAND(g) (g->knownHands[g->myPosition])

/**
 * The index of a player.
 */
typedef int ParlPlayer;

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

    /* PM-related information */

    /**
     * The number of turns away from the first turn that the PM sits, or `PARL_NO_PM` if there is no PM.
     */
    ParlPlayer pmPosition;

    /**
     * The PM card, or `PARL_NO_PM` if there is no PM.
     */
    ParlCardIdx pmCard;

    /**
     * The Cabinet.
     */
    ParlStack cabinet;

    /* Essentials -- required to enforce rules */

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
     * The current stage of the game
     */
    enum { PREGAME, MIDGAME, ENDGAME } stage;

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
} ParlGame;

/**
 * Initialize the game from midgame.
 * @param game The game to initialize.
 * @param numJokers The number of jokers in the deck.
 * @param numPlayers Same as the field.
 * @param myPosition Same as the field.
 * @param myFirstCard The first card in the known player's hand, obtained from pregame.
 * @return Whether the initialization was successful.
 */
bool parlGame_init(ParlGame* game,
                   int numJokers,
                   int numPlayers,
                   ParlPlayer myPosition,
                   ParlCardIdx myFirstCard);

#endif //PARLIAMENT_GAME_H
