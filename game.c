//
// Created by Weiju Wang on 8/17/24.
//

#include "game.h"

#include <stdlib.h>

bool parlGame_init(ParlGame* const game,
                   const int numJokers,
                   const int numPlayers,
                   const ParlPlayer myPosition,
                   const ParlCardIdx myFirstCard)
{
    game->numPlayers = numPlayers;
    game->myPosition = myPosition;

    game->pmPosition = PARL_NO_PM;
    game->pmCard = PARL_NO_PM;
    game->cabinet = PARL_EMPTY_STACK;

    game->turn = 0;
    game->parliament = PARL_EMPTY_STACK;
    game->drawDeckSize = PARL_JOKER_IDX + numJokers;

    game->handSizes = calloc(numPlayers, sizeof(int));
    game->knownHands = calloc(numPlayers, sizeof(ParlStack));

    game->knownHands[game->myPosition] = myFirstCard;

    game->faceDownCards = numJokers * PARL_JOKER_CARD
            + PARL_COMPLETE_STACK_NO_JOKERS;

    // Memory allocation errors will set one of these to null.
    return game->handSizes && game->knownHands;
}

void parlGame_free(const ParlGame* const game)
{
    free(game->handSizes);
    free(game->knownHands);
}