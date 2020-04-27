/*
 *	oledimg.h
 *
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

#include "oledimg.h"
#include <stdio.h>

const uint8_t * getOledFont(int font)
{
    switch(font)
    {
        case MON_FONT_CLASSIC:
        case MON_FONT_LCD2544:
            return lcd25x44;
            break;
        case MON_FONT_DECOSOL:
            return soldeco25x44;
            break;
        case MON_FONT_DECOHOL:
            return holdeco25x44;
            break;
        case MON_FONT_FESTUS:
            return festus25x44;
            break;
        case MON_FONT_HOLFESTUS:
            return holfestus25x44;
            break;
        case MON_FONT_NB1999:
            return nb1999s25x44;
            break;
        case MON_FONT_LCD1521:
            return lcd15x21;
            break;
        case MON_FONT_LCD1217:
            return lcd12x17;
            break;
        case MON_FONT_LCD2348:
            return lcd23x48;
            break;
        default:
            return lcd25x44;
    }

    return lcd25x44;

}

void printOledFontTypes(void)
{
    for (int font=0; font < MON_FONT_MAX; font++)
        printf("%s\n",oled_font_str[font]);
}