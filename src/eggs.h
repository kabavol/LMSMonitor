/*
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	See <http://www.gnu.org/licenses/> to get a copy of the GNU General
 *	Public License.
 *
 */

#ifndef GAMEMULATE_H
#define GAMEMULATE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

enum inching { IW_WIDTH, IW_HEIGHT };
enum honeymethod { IM_SHRINK, IM_GROW };
enum inchdir {
    ID_GLEFT,
    ID_GRIGHT,
    ID_GDOWN,
    ID_GUP,
    ID_SLEFT,
    ID_SRIGHT,
    ID_SDOWN,
    ID_SUP
};

typedef struct point_t {
    int16_t x;
    int16_t y;
} point_t;

typedef struct fpoint_t {
    float x;
    float y;
} fpoint_t;

typedef struct speed_t {
    fpoint_t accel;
    float delta;
} speed_t;

typedef struct grect_t {
    fpoint_t pos;
    int16_t w;
    int16_t h;
    speed_t phys;
} grect_t;

typedef struct limits_t {
    int16_t min;
    int16_t max;
} limits_t;

typedef struct dimention_t {
    point_t pos;
    int16_t w;
    int16_t h;
    int16_t radius;
} dimention_t;

// drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color)
typedef struct inchingball_t {
    dimention_t dimention;
    enum inching direction; // modifying width/height
    enum honeymethod hm;    // -1/+1 drives shrink/grow
    enum inchdir sliding;   // modifying direction
} inchingball_t;

typedef struct plays_t {
    int adj;
    enum inching direction; // modifying width/height
    enum honeymethod hm;    // -1/+1 drives shrink/grow
    enum inchdir sliding;   // modifying direction
} plays_t;

typedef struct inching_t {
    int drinkme; // active shrink/grow object
    int currseq; // current play
    int playcnt;
    point_t pin;
    limits_t limitw;
    limits_t limith;
    limits_t limitx;
    limits_t limity;
    inchingball_t incher[3];
    plays_t playseq[9];
} inching_t;

typedef struct pongem_t {
    int plscore;
    int prscore;
    grect_t court;
    grect_t left;
    grect_t right;
    grect_t ball;
} pongem_t;

inching_t *initInching(const point_t pin, const limits_t lw, const limits_t lh,
                       const limits_t lx, const limits_t ly);
void animateInching(inching_t *b);

pongem_t *initPongPlay(const point_t pin);
void animatePong(pongem_t *p);

#endif
