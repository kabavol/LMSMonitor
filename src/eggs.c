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

#include <stdio.h>
#include <string.h>

#include <math.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#ifdef __arm__
#include "display.h"
#include "ArduiPi_OLED.h"

inching_t *initInching(const point_t pin, const limits_t lw, const limits_t lh,
                       const limits_t lx, const limits_t ly) {

    static inching_t ii = {.drinkme = 2,
                           .currseq = 0,
                           .playcnt = 0,
                           .pin = {0, 0},
                           .limitw = {0, 0},
                           .limith = {0, 0},
                           .limitx = {0, 0},
                           .limity = {0, 0},
                           .incher = {{.dimention = {{0, 0}, 0, 0, 3},
                                       .direction = IW_WIDTH,
                                       .hm = IM_GROW,
                                       .sliding = ID_GRIGHT},
                                      {.dimention = {{0, 0}, 0, 0, 3},
                                       .direction = IW_HEIGHT,
                                       .hm = IM_GROW,
                                       .sliding = ID_GDOWN},
                                      {.dimention = {{0, 0}, 0, 0, 3},
                                       .direction = IW_WIDTH,
                                       .hm = IM_SHRINK,
                                       .sliding = ID_SLEFT}},
                           .playseq = {
                               {0, IW_WIDTH, IM_SHRINK, ID_SLEFT},
                               {-1, IW_HEIGHT, IM_GROW, ID_GDOWN},
                               {0, IW_HEIGHT, IM_SHRINK, ID_SDOWN},
                               {-1, IW_WIDTH, IM_GROW, ID_GRIGHT},
                               {0, IW_WIDTH, IM_SHRINK, ID_SRIGHT},
                               {-1, IW_HEIGHT, IM_GROW, ID_GUP},
                               {0, IW_HEIGHT, IM_SHRINK, ID_SUP},
                               {-1, IW_WIDTH, IM_GROW, ID_GLEFT},
                           }};

    // fix limits
    ii.limitw.min = lw.min;
    ii.limitw.max = lw.max;
    ii.limith.min = lh.min;
    ii.limith.max = lh.max;
    ii.limitx.min = lx.min;
    ii.limitx.max = lx.max;
    ii.limity.min = ly.min;
    ii.limity.max = ly.max;
    // fix init limit on the active incher
    ii.pin = pin;
    ii.incher[0].dimention.pos = pin;
    ii.incher[0].dimention.w = lh.min;
    ii.incher[0].dimention.h = lw.min;
    ii.incher[1].dimention.pos = pin;
    ii.incher[1].dimention.pos.x = pin.x + 2 + lh.min;
    ii.incher[1].dimention.w = lh.min;
    ii.incher[1].dimention.h = lw.min;
    ii.incher[2].dimention.pos = pin;
    ii.incher[2].dimention.pos.y = pin.y + 2 + lw.min;
    ii.incher[2].dimention.w = lh.max;
    ii.incher[2].dimention.h = lw.min;
    ii.playcnt = 7;

    return (inching_t *)&ii;
}

void animateInching(inching_t *b) {

    bool trip = false;
    int ii = b->drinkme;

    if (IW_WIDTH == b->incher[ii].direction) {
        if (IM_SHRINK == b->incher[ii].hm) {
            if ((ID_SLEFT == b->incher[ii].sliding) &&
                (b->incher[ii].dimention.w > b->limitw.min)) {
                b->incher[ii].dimention.w--;
            } else if ((ID_SRIGHT == b->incher[ii].sliding) &&
                       (b->incher[ii].dimention.w > b->limitw.min)) {
                b->incher[ii].dimention.w--;
                b->incher[ii].dimention.pos.x++;
            }
            trip = (b->incher[ii].dimention.w == b->limitw.min);
        } else if (IM_GROW == b->incher[ii].hm) {
            if ((ID_GRIGHT == b->incher[ii].sliding) &&
                (b->incher[ii].dimention.w < b->limitw.max)) {
                b->incher[ii].dimention.w++;
            } else if ((ID_GLEFT == b->incher[ii].sliding) &&
                       (b->incher[ii].dimention.w < b->limitw.max)) {
                b->incher[ii].dimention.w++;
                b->incher[ii].dimention.pos.x--;
            }
            trip = (b->incher[ii].dimention.w == b->limitw.max);
        }
    } else if (IW_HEIGHT == b->incher[ii].direction) {
        if (IM_SHRINK == b->incher[ii].hm) {
            if ((ID_SDOWN == b->incher[ii].sliding) &&
                (b->incher[ii].dimention.h > b->limith.min)) {
                b->incher[ii].dimention.pos.y++;
                b->incher[ii].dimention.h--;
            } else if (ID_SUP == b->incher[ii].sliding) {
                b->incher[ii].dimention.h--;
            }
            trip = (b->incher[ii].dimention.h == b->limith.min);
        } else if (IM_GROW == b->incher[ii].hm) {
            if ((ID_GDOWN == b->incher[ii].sliding) &&
                (b->incher[ii].dimention.h < b->limith.max)) {
                b->incher[ii].dimention.h++;
                trip = (b->incher[ii].dimention.h == b->limith.max);
            } else if (ID_GUP == b->incher[ii].sliding) {
                b->incher[ii].dimention.h++;
                b->incher[ii].dimention.pos.y--;
            }
            trip = (b->incher[ii].dimention.h == b->limith.max);
        }
    }
    // trip sequence
    if (trip) {

        b->currseq++;
        if (b->currseq > b->playcnt)
            b->currseq = 0;
        b->drinkme += b->playseq[b->currseq].adj;
        if (b->drinkme < 0)
            b->drinkme = 2;
        if (b->drinkme > 2)
            b->drinkme = 0;
        ii = b->drinkme;

        b->incher[ii].direction = b->playseq[b->currseq].direction;
        b->incher[ii].hm = b->playseq[b->currseq].hm;
        b->incher[ii].sliding = b->playseq[b->currseq].sliding;
    }

    // and paint
    fillRectangle(b->pin.x, b->pin.y, b->limitw.max, b->limith.max, BLACK);
    for (int i = 0; i < 3; i++) {
        drawRoundRectangle(b->incher[i].dimention.pos.x,
                           b->incher[i].dimention.pos.y,
                           b->incher[i].dimention.w, b->incher[i].dimention.h,
                           b->incher[i].dimention.radius, WHITE);
    }
}

int lrand(int l, int u) {
    int r = (rand() % (u - l + 1)) + l;
    if (r = 0)
        r = lrand(l, u);
    return r;
}

void centerBall(grect_t *b, grect_t c) {
    b->pos.x = c.pos.x + (c.w / 2);
    b->pos.y = c.pos.y + (c.h / 2);
    b->phys.accel.x = lrand(-3, 3);
    b->phys.accel.y = lrand(-3, 3);
}

pongem_t *initPongPlay(const point_t pin) {

    static pongem_t pp = {
        .plscore = 0,
        .prscore = 0,
        .court = {.pos = {0, 0}, .w = 40, .h = 19,.phys={{0,0},0}},
        .left = {.pos = {1, 8}, .w = 2, .h = 6,.phys={{0,2},2}},
        .right = {.pos = {37, 8}, .w = 2, .h = 6,.phys={{0,-2},2}},
        .ball = {.pos = {0, 0}, .w = 2, .h = 2,.phys={{3,3},3}},
    };

    pp.court.pos.x = (int)pin.x;
    pp.court.pos.y = (int)pin.y;

    pp.left.pos.x += pin.x;
    pp.left.pos.y += pin.y;
    pp.left.phys.accel.y = lrand(-2, 2);
    
    pp.right.pos.x += pin.x;
    pp.right.pos.y += pin.y;
    pp.right.phys.accel.y = lrand(-2, 2);

    // center and randomize travel
    centerBall(&pp.ball, pp.court);

    return (pongem_t *)&pp;
}

void animatePong(pongem_t *p) {

    // clear the court
    fillRectangle(p->court.pos.x, p->court.pos.y, p->court.w, p->court.h,
                  BLACK);

    // draw the net
    int cx = p->court.pos.x + (int)(p->court.w / 2);
    int my = p->court.pos.y + p->court.h;
    drawALine(cx, p->court.pos.y, cx, my, WHITE);
    for (int yy = p->court.pos.y; yy < my; yy += 5) {
        putPixel(cx, yy, BLACK);
        putPixel(cx, yy + 1, BLACK);
    }

    // position actors
    p->ball.pos.x += p->ball.phys.accel.x;
    p->ball.pos.y += p->ball.phys.accel.y;
    p->left.pos.y += p->left.phys.accel.y;
    p->right.pos.y += p->right.phys.accel.y;
    fillRectangle(p->left.pos.x, p->left.pos.y, p->left.w, p->left.h, WHITE);
    fillRectangle(p->right.pos.x, p->right.pos.y, p->right.w, p->right.h,
                  WHITE);
    fillRectangle(p->ball.pos.x, p->ball.pos.y, p->ball.w, p->ball.h, WHITE);

    // test left loss
    if (p->ball.pos.x < p->court.pos.x) {
        p->prscore++;
        centerBall(&p->ball, p->court);
    } // test right loss
    else if (p->ball.pos.x < (p->court.pos.x + p->court.w)) {
        p->plscore++;
        centerBall(&p->ball, p->court);
    }

    // collision logic top and bottom
    if ((p->ball.pos.y + p->ball.h > p->court.pos.y + p->court.h) ||
        (p->ball.pos.y < p->court.pos.y)) {
        p->ball.phys.accel.y *= -1;
    }

    // left limits
    if (p->left.pos.y + p->left.h > p->court.pos.y + p->court.h) {
        p->left.pos.y -= p->left.h / 2;
        p->left.phys.accel.y = lrand(-2, 2);
    }
    else if (p->left.pos.y < p->court.pos.y) {
        p->left.pos.y += p->left.h / 2;
        p->left.phys.accel.y = lrand(-2, 2);
    }

    // right limits
    if (p->right.pos.y + p->right.h > p->court.pos.y + p->court.h) {
        p->right.pos.y -= p->right.h / 2;
        p->right.phys.accel.y = lrand(-2, 2);
    }
    else if (p->right.pos.y < p->court.pos.y) {
        p->right.pos.y += p->right.h / 2;
        p->right.phys.accel.y = lrand(-2, 2);
    }
}

#endif