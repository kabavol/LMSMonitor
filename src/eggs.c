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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

#ifdef __arm__
#include "oledimg.h"
#include "visualize.h"
#include "ArduiPi_OLED.h"
#include "display.h"
#include "eggs.h"

int lrand(int l, int u) {
    int r = (rand() % (u - l + 1)) + l;
    return r;
}

void putTapeType(audio_t audio) {

    int szw = 6;
    int szh = 4;
    int w = elementLength(szh, szw);

    int start = 0;
    uint8_t dest[w];
    start = audio.audioIcon * w;
    int x = 99;
    int y = 29;
    memcpy(dest, cmedia + start, sizeof dest);
    fillRectangle(x, y, szw, szh, BLACK);
    drawBitmap(x, y, dest, szw, szh, WHITE);
}

void compactCassette(void) {
    drawBitmap(0, 0, cassette, 128, 64, WHITE);
}

void cassetteEffects(int xpos, int frame, int mxframe, int direction) {

    int szw = 10;
    int szh = 10;
    int w = elementLength(szh, szw);
    int prev = frame + direction;
    if (prev < 0)
        prev = mxframe;
    if (prev > mxframe)
        prev = 0;
    uint8_t dest[w];
    int ypos = 23;
    int start = prev * w;
    memcpy(dest, hub10x10 + start, sizeof dest);
    drawRectangle(xpos, ypos, szw, szh, BLACK);
    drawBitmap(xpos, ypos, dest, szw, szh, BLACK);
    start = frame * w;
    memcpy(dest, hub10x10 + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, WHITE);
}

void putSL1200Btn(audio_t audio) {
    int w = 7;
    int h = 6;
    int x = 71;
    int yy[] = {35, 44, 50};
    int y = yy[audio.audioIcon];
    // draw slider position based on audio fidelity
    fillRectangle(x + 1, 32, 5, 23, BLACK);
    fillRectangle(x + 2, 33, 3, 23, WHITE);
    drawLine(x + 3, 34, x + 3, 54, BLACK);

    fillRectangle(x, y - 1, w, h, BLACK);
    fillRectangle(x + 1, y, w - 2, h - 2, WHITE);
    fillRectangle(x + 2, y + 1, w - 4, h - 4, BLACK);
}

void toneArm(double pct, bool init) {
    int w = 270;
    int h = 4;
    int start = 0;
    uint8_t dest[w];
    if (init)
        start = w * (1 + (int)(pct / 10));
    memcpy(dest, tonearm + start, sizeof dest);
    drawBitmap(39, 4, dest, 40, 54, BLACK); // shadow line
    drawBitmap(38, 3, dest, 40, 54, WHITE);
}

void technicsSL1200(bool blank) {

    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    } else {
        int x = 38;
        uint16_t c = BLACK;
        fillRectangle(x, 40, 32, 20, c);
        drawLine(x, 42, 50, 42, c);
        drawLine(x, 41, 50, 41, c);
        drawLine(x + 1, 40, 50, 40, c);
        drawLine(x + 2, 39, 50, 39, c);
        drawLine(x + 3, 38, 50, 38, c);
        drawLine(x + 4, 37, 50, 37, c);
        drawLine(x + 5, 36, 50, 36, c);
        fillRectangle(47, 27, 30, 24, c);
        fillRectangle(60, 4, 17, 24, c);
    }
    drawBitmap(0, 0, sl1200t, 83, 64, WHITE);
}

void vinylEffects(int xpos, int lpos, int frame, int mxframe) {

    int z = 50;
    int erase = xpos - 2;
    int place = xpos - 1;
    drawBitmap(erase, erase, vinfx, z, z, BLACK);
    drawBitmap(place, place, vinfx, z, z, WHITE);

    z = 57;
    int h = 19;
    int start = 0;
    uint8_t dest[z];
    int blank = frame - 1;
    if (blank < 0)
        blank = mxframe;
    // last frame
    start = blank * z;
    memcpy(dest, vinlbl + start, sizeof dest);
    drawBitmap(lpos, lpos, dest, h, h, BLACK);
    // this frame
    start = frame * z;
    memcpy(dest, vinlbl + start, sizeof dest);
    drawBitmap(lpos, lpos, dest, h, h, WHITE);
}

void putReelToReel(audio_t audio) {
    // manipulate "switches" visualize fidelity
}

void reelToReel(bool blank) {
    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    }
    drawBitmap(4, 17, reel2reel, 62, 54, WHITE);
}

void reelEffects(int xpos, int ypos, int frame, int mxframe, int direction) {

    int szw = 30;
    int szh = 30;
    int w = elementLength(szh, szw);

    int prev = frame + direction;
    if (prev < 0)
        prev = mxframe;
    if (prev > mxframe)
        prev = 0;
    uint8_t dest[w];
    int start = prev * w;
    memcpy(dest, smr2rc + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, BLACK);
    start = frame * w;
    memcpy(dest, smr2rc + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, WHITE);
}

void putVcr(audio_t audio) {
    // manipulate "switches" visualize fidelity
}

void vcrPlayer(bool blank) {
    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    }
    drawBitmap(3, 25, vcr, 118, 35, WHITE);
}

void vcrEffects(int xpos, int ypos, int frame, int mxframe) {

    int szw = 25;
    int szh = 6;
    int w = elementLength(szh, szw);
    int prev = frame + 1;
    if (prev < 0)
        prev = mxframe;
    if (prev > mxframe)
        prev = 0;
    uint8_t dest[w];
    int start = prev * w;
    memcpy(dest, vcrclock + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, BLACK);
    start = frame * w;
    memcpy(dest, vcrclock + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, WHITE);
}

void putRadio(audio_t audio) {
    // manipulate "switches" visualize fidelity
}

void radio50(bool blank) {
    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    }
    drawBitmap(3, 8, radio50s, 67, 50, WHITE);
}

void radioEffects(int xpos, int ypos, int frame, int mxframe) {
    int szw = 19;
    int szh = 6;
    int w = elementLength(szh, szw);
    int prev = frame + 1;
    if (prev < 0)
        prev = mxframe;
    if (prev > mxframe)
        prev = 0;
    uint8_t dest[w];
    int start = prev * w;
    memcpy(dest, radani19x6 + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, BLACK);
    start = frame * w;
    memcpy(dest, radani19x6 + start, sizeof dest);
    drawBitmap(xpos, ypos, dest, szw, szh, WHITE);
}

void putTVTime(audio_t audio) {
    // manipulate "switches" visualize fidelity
}

void TVTime(bool blank) {
    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    }
    drawBitmap(0, 0, tvtime88x64, 88, 64, WHITE);
}

void putPCTime(audio_t audio) {
    // manipulate "switches" visualize fidelity
}

void PCTime(bool blank) {
    if (blank) {
        fillRectangle(0, 0, 128, 64, BLACK);
    }
    drawBitmap(0, 0, pctime67x64, 67, 64, WHITE);
}


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

void centerBall(grect_t *b, grect_t c) {
    b->pos.x = c.pos.x + (c.w / 2);
    b->pos.y = c.pos.y + (c.h / 2);
    b->phys.accel.x = 3;
    b->phys.accel.y = (float)lrand(-3, 3);
}

double getDistance(int x1, int y1, int x2, int y2) {
    int xd = x2 - x1;
    int yd = y2 - y1;
    return sqrt((xd ^ 2) + (yd ^ 2));
}

double getDistance(fpoint_t x1y1, fpoint_t x2y2) {
    int xd = x2y2.x - x1y1.x;
    int yd = x2y2.y - x1y1.y;
    return sqrt((xd ^ 2) + (yd ^ 2));
}

pongem_t *initPongPlay(const point_t pin) {

    static pongem_t pp = {
        .plscore = 0,
        .prscore = 0,
        .court = {.pos = {0, 0}, .w = 42, .h = 32, .phys = {{0, 0}, 0}},
        .left = {.pos = {1, 14}, .w = 2, .h = 6, .phys = {{0, -2}, 2}},
        .right = {.pos = {39, 14}, .w = 2, .h = 6, .phys = {{0, -2}, 2}},
        .ball = {.pos = {0, 0}, .w = 2, .h = 2, .phys = {{3, 1}, 3}},
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

void drawNet(pongem_t *p) {
    // draw the net
    int zx = p->court.pos.x + (int)(p->court.w / 2);
    int zy = p->court.pos.y + p->court.h;

    drawLine(zx, p->court.pos.y, zx, zy, WHITE);
    for (int yy = p->court.pos.y; yy < zy; yy += 5) {
        putPixel(zx, yy, BLACK);
        putPixel(zx, yy + 1, BLACK);
    }
}
void animatePong(pongem_t *p) {

    // clear the court
    fillRectangle(p->court.pos.x, p->court.pos.y, p->court.w, p->court.h,
                  BLACK);

    drawNet(p);

    if (0 == p->ball.phys.accel.x) {
        p->ball.phys.accel.x = p->ball.phys.delta;
    }

    // position actors
    p->ball.pos.x += p->ball.phys.accel.x;
    p->ball.pos.y += p->ball.phys.accel.y;

    fillRectangle(p->ball.pos.x, p->ball.pos.y, p->ball.w, p->ball.h, WHITE);

    bool inplay = true;
    // test left loss
    if (p->ball.pos.x <= p->court.pos.x) {
        p->prscore++;
        centerBall(&p->ball, p->court);
        //inplay = false;
    } // test right loss
    else if (p->ball.pos.x >= (p->court.pos.x + p->court.w)) {
        p->plscore++;
        centerBall(&p->ball, p->court);
        //inplay = false;
    }

    // collision logic top and bottom
    if ((p->ball.pos.y + (p->ball.h / 2) > p->court.pos.y + p->court.h) ||
        (p->ball.pos.y < p->court.pos.y)) {
        p->ball.phys.accel.y *= -1;
    }

int follow = 4;
    if (inplay) {

        p->left.pos.y += p->left.phys.accel.y;
        p->right.pos.y += p->right.phys.accel.y;

        // left paddle limits
        if (p->left.pos.y + p->left.h >= p->court.pos.y + p->court.h) {
            p->left.pos.y = (p->court.pos.y + p->court.h) - p->left.h;
            p->left.phys.accel.y = lrand(-3, 1);
        } else if (p->left.pos.y < p->court.pos.y) {
            p->left.pos.y = p->court.pos.y;
            p->left.phys.accel.y = 2; //lrand(-1, 3);
        }

        // ball following
        if (p->left.pos.y > p->ball.pos.y)
            p->left.phys.accel.y = follow;
        if (p->left.pos.y < p->ball.pos.y)
            p->left.phys.accel.y = -follow;
        else
            p->left.phys.accel.y = 0;

        // right paddle limits
        if (p->right.pos.y + p->right.h > p->court.pos.y + p->court.h) {
            p->right.pos.y = (p->court.pos.y + p->court.h) - p->right.h;
            p->right.phys.accel.y = lrand(-3, 1);
        } else if (p->right.pos.y < p->court.pos.y) {
            p->right.pos.y = p->court.pos.y;
            p->right.phys.accel.y = 2; //lrand(-1, 3);
        }

        // ball following
        if (p->right.pos.y > p->ball.pos.y)
            p->right.phys.accel.y = follow;
        else if (p->left.pos.y < p->ball.pos.y)
            p->right.phys.accel.y = -follow;
        else
            p->right.phys.accel.y = 0;
    }

    fillRectangle(p->left.pos.x, p->left.pos.y, p->left.w, p->left.h, WHITE);
    fillRectangle(p->right.pos.x, p->right.pos.y, p->right.w, p->right.h,
                  WHITE);

    // collision right paddle
    if (p->ball.pos.x >= p->right.pos.x && p->ball.pos.y >= p->right.pos.y &&
        p->ball.pos.y <= (p->right.pos.y + p->right.h)) {
        p->ball.phys.accel.x *= -1;
        p->ball.phys.accel.y *= -1;
    }

    // collision left paddle
    if (p->ball.pos.x <= (p->left.pos.x + p->left.w) &&
        p->ball.pos.y >= p->left.pos.y &&
        p->ball.pos.y <= (p->left.pos.y + p->left.h)) {
        p->ball.phys.accel.x *= -1;
        p->ball.phys.accel.y *= -1;
    }

    // scores
    char buf[4];
    int zy = p->court.pos.y + 8;
    int zx = (int)(p->court.w / 4);
    sprintf(buf, "%02d", p->plscore);
    putTinyTextMaxWidth(p->court.pos.x + zx - 3, zy, 2, buf);
    sprintf(buf, "%02d", p->prscore);
    putTinyTextMaxWidth(p->court.pos.x + (3 * zx) - 5, zy, 2, buf);

    drawNet(p);

    //printf("L: %f\n", getDistance(p->left.pos, p->ball.pos));
    //printf("R: %f\n", getDistance(p->ball.pos, p->right.pos));

    // roll on 20
    if (p->plscore > 20 || p->prscore > 20) {
        p->plscore = 0;
        p->prscore = 0;
    }
}

#endif