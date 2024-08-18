cmake_minimum_required(VERSION 3.28)
project(parliament C)

set(CMAKE_C_STANDARD 11)

add_executable(parliament main.c
        cards.c
        cards.h
        game.c
        game.h)
