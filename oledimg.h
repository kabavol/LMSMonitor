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

#ifndef OLEDIMG_H
#define OLEDIMG_H

#include <stdint.h>

const uint8_t lcd23x48[]  = {
// Digit 0  
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x19, 0xff, 0x9e, 0x1e, 0x00, 0x1e, 0x1e, 
0x00, 0x1e, 0x1e, 0x00, 0x1e, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 
0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 
0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x38, 0x00, 0x3c, 0x30, 
0x00, 0x1c, 0x00, 0x00, 0x08, 0x60, 0x00, 0x00, 0x70, 0x00, 0x08, 0x78, 0x00, 0x38, 0x78, 0x00, 
0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 
0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 
0x00, 0x78, 0x78, 0x00, 0x78, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 
0xf0, 0xef, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 1
0x00, 0x00, 0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 
0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 
0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 
0x00, 0x1c, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x70, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// Digit 2
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x01, 0xff, 0x9e, 0x00, 0x00, 0x1e, 0x00, 
0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 
0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x07, 
0xff, 0xdc, 0x0f, 0xff, 0xe8, 0x6f, 0xff, 0xe0, 0x77, 0xff, 0xc0, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 
0x00, 0xef, 0xfe, 0x00, 0x1f, 0xff, 0x00, 0xff, 0xff, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 3
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x01, 0xff, 0x9e, 0x00, 0x00, 0x1e, 0x00, 
0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 
0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x07, 
0xff, 0xdc, 0x0f, 0xff, 0xe8, 0x0f, 0xff, 0xe0, 0x07, 0xff, 0xc8, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x0f, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 4
0x00, 0x00, 0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x1e, 0x18, 0x00, 0x1e, 0x1e, 0x00, 0x1e, 0x1e, 
0x00, 0x1e, 0x1e, 0x00, 0x1e, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 
0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 
0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x38, 0x00, 0x3c, 0x37, 
0xff, 0xdc, 0x0f, 0xff, 0xe8, 0x0f, 0xff, 0xe0, 0x07, 0xff, 0xc8, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x70, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// Digit 5
0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xe0, 0x07, 0xff, 0xc0, 0x19, 0xff, 0x80, 0x1e, 0x00, 0x00, 0x1e, 
0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 
0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x38, 0x00, 0x00, 0x37, 
0xff, 0xc0, 0x0f, 0xff, 0xe0, 0x0f, 0xff, 0xe0, 0x07, 0xff, 0xc8, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x0f, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 6
0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xe0, 0x07, 0xff, 0xc0, 0x19, 0xff, 0x80, 0x1e, 0x00, 0x00, 0x1e, 
0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 
0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x38, 0x00, 0x00, 0x37, 
0xff, 0xc0, 0x0f, 0xff, 0xe0, 0x6f, 0xff, 0xe0, 0x77, 0xff, 0xc8, 0x78, 0x00, 0x38, 0x78, 0x00, 
0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 
0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 
0x00, 0x78, 0x78, 0x00, 0x78, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 
0xf0, 0xef, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 7
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x01, 0xff, 0x9e, 0x00, 0x00, 0x1e, 0x00, 
0x00, 0x1e, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 
0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 
0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 
0x00, 0x1c, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x70, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// Digit 8 
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x19, 0xff, 0x9e, 0x1e, 0x00, 0x1e, 0x1e, 
0x00, 0x1e, 0x1e, 0x00, 0x1e, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 
0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 
0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x38, 0x00, 0x3c, 0x37, 
0xff, 0xdc, 0x0f, 0xff, 0xe8, 0x6f, 0xff, 0xe0, 0x77, 0xff, 0xc8, 0x78, 0x00, 0x38, 0x78, 0x00, 
0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 
0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 0x00, 0x78, 0x78, 
0x00, 0x78, 0x78, 0x00, 0x78, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 
0xf0, 0xef, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// Digit 9
0x0f, 0xff, 0xf4, 0x1f, 0xff, 0xee, 0x07, 0xff, 0xde, 0x19, 0xff, 0x9e, 0x1e, 0x00, 0x1e, 0x1e, 
0x00, 0x1e, 0x1e, 0x00, 0x1e, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 
0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 
0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x38, 0x00, 0x3c, 0x37, 
0xff, 0xdc, 0x0f, 0xff, 0xe8, 0x0f, 0xff, 0xe0, 0x07, 0xff, 0xc8, 0x00, 0x00, 0x38, 0x00, 0x00, 
0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 
0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 
0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 
0xf0, 0x0f, 0xfe, 0xf0, 0x1f, 0xff, 0x70, 0xff, 0xff, 0x70, 0x7f, 0xff, 0x80, 0x00, 0x00, 0x00,
// semi-colon
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x3e, 0x00, 
0x00, 0x3e, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x3e, 0x00, 
0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  Blank Digit  
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// minus - sign
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 
0xff, 0xc0, 0x0f, 0xff, 0xe0, 0x0f, 0xff, 0xe0, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t taNum[]  = {
  0x00,0x00,0x00,0x00, 0x00,0xc0,0xde,0x00, // space ! 
  0x03,0x00,0x03,0x00, 0x74,0x2f,0x74,0x2f, // " #
  0x2c,0x49,0xff,0x39, 0x43,0x33,0x6c,0x63, // $ %
  0x6e,0x59,0x19,0x26, 0x00,0x05,0x03,0x00, // & '
  0x1c,0x22,0x41,0x00, 0x00,0x41,0x22,0x1c, // ( )
  0x2a,0x1c,0x1c,0x2a, 0x08,0x3e,0x08,0x08, // * +
  0x00,0xa0,0x60,0x00, 0x08,0x08,0x08,0x08, // , -
  0x00,0xc0,0xc0,0x00, 0x03,0x0c,0x30,0xc0, // . / 
  0x3e,0x41,0x41,0x3e, 0x04,0x02,0x7f,0x00, // 0 1
  0x62,0x51,0x49,0x46, 0x22,0x41,0x49,0x36, // 2 3
  0x1c,0x12,0x79,0x10, 0x2f,0x49,0x49,0x31, // 4 5
  0x36,0x49,0x49,0x31, 0x41,0x31,0x0d,0x03, // 6 7
  0x36,0x49,0x49,0x36, 0x46,0x49,0x49,0x3e, // 8 9
  0x00,0x36,0x36,0x00, 0x00,0xd6,0x76,0x00, // : ;
  0x08,0x14,0x22,0x41, 0x24,0x24,0x24,0x24, // < = 
  0x41,0x22,0x14,0x08, 0x02,0xc1,0xd9,0x06}; // > ?
const uint8_t taMaj[]  = {
  0x39,0x48,0x71,0x41,0x3e, 0x7e,0x11,0x11,0x11,0x7e, // @ A   
  0x7f,0x49,0x49,0x49,0x36, 0x3e,0x41,0x41,0x41,0x22, // B C
  0x7f,0x41,0x41,0x41,0x3e, 0x7f,0x49,0x49,0x41,0x41, // D,E
  0x7f,0x09,0x09,0x01,0x01, // F
  0x3e,0x41,0x49,0x49,0x3a, 0x7f,0x08,0x08,0x08,0x7f, // G,H
  0x00,0x00,0x7f,0x00,0x00, 0x21,0x41,0x41,0x41,0x3f, // I,J
  0x7f,0x08,0x14,0x22,0x41, 0x7f,0x40,0x40,0x40,0x40, // K,L
  0x7f,0x02,0x04,0x02,0x7f, 0x7f,0x04,0x08,0x10,0x7f, // M,N
  0x3e,0x41,0x41,0x41,0x3e, 0x7f,0x11,0x11,0x11,0x0e, // O,P
  0x3e,0x41,0x49,0x51,0x7e, 0x7f,0x09,0x11,0x29,0x46, // Q,R
  0x46,0x49,0x49,0x49,0x32, 0x01,0x01,0x7f,0x01,0x01, // S,T
  0x3f,0x40,0x40,0x40,0x3f, 0x07,0x18,0x60,0x18,0x07, // U,V
  0x1f,0x60,0x1e,0x60,0x1f, 0x63,0x14,0x08,0x14,0x63, // W,X
  0x07,0x04,0x78,0x04,0x07, 0x61,0x51,0x49,0x45,0x43, // Y,Z
  0x00,0x7f,0x41,0x41,0x00, 0x03,0x06,0x1c,0x30,0x60, // [ forward slash
  0x00,0x41,0x41,0x7f,0x00, 0x04,0x02,0x01,0x02,0x04, // ] ^
  0x80,0x80,0x80,0x80,0x80}; // _
  
const uint8_t taMin []  = {
  0x00,0x03,0x06,0x00, 0x20,0x54,0x54,0x78, // ` a 0x38,0x44,0x44,0x7C,
  0x7f,0x44,0x44,0x38, 0x38,0x44,0x44,0x28, // b c
  0x38,0x44,0x44,0x7f, 0x38,0x54,0x54,0x58, // d e
  0x00,0xfe,0x09,0x08, 0x98,0xa4,0xa4,0x78, // f g
  0x7f,0x04,0x04,0x78, 0x00,0x7d,0x00,0x00, // h i
  0x40,0x80,0x80,0x7d, 0x7e,0x10,0x28,0x44, // j k
  0x00,0x3f,0x40,0x00, 0x7c,0x08,0x08,0x7c, // l m
  0x7c,0x04,0x04,0x78, 0x38,0x44,0x44,0x38, // n o
  0xfc,0x24,0x24,0x18, 0x38,0x44,0x44,0xf8, // p q
  0x7c,0x04,0x04,0x08, 0x48,0x54,0x54,0x24, // r s
  0x00,0x3f,0x44,0x04, 0x3c,0x40,0x40,0x7c, // t u
  0x1c,0x60,0x60,0x1c, 0x5c,0x20,0x20,0x5c, // v w
  0x64,0x18,0x18,0x64, 0x04,0x08,0x78,0x04, // x y
  0x64,0x54,0x4C,0x44, 0x08,0x08,0x36,0x41, // z {
  0x00,0x00,0xff,0x00, 0x41,0x36,0x08,0x08, // | }
  0x10,0x20,0x10,0x20, 0xff,0xff,0xff,0xff}; // ~ del
const uint8_t squar[]  ={0xff,0x81,0x81,0x81,0x81,0x81,0x81,0xff};
const uint8_t smile[]  = {0x3c,0x42,0x95,0xa5,0xa1,0xa1, 0xa5,0x95,0x42,0x3c};
const uint8_t sad  []  = {0x3c,0x42,0xc5,0xa5,0xa1,0xa1, 0xa5,0xc5,0x42,0x3c};
const uint8_t didel[]  = {0x82,0xf2,0xfe,0xfe,0x8e,0x82,0xc2,0x3c,0x00, \
                              0xc0,0xf8,0xfd,0x3d,0x0d,0x00, \
                              0x82,0xf2,0xfe,0xfe,0x8e,0x82,0xc2,0x3c,0x00, \
                              0xc0,0xf8,0xfe,0xfe,0x96,0x92,0x82,0x02,0x00, \
                              0xc0,0xf8,0xfe,0xfe,0x86,0x80,0x80,0x80,0x80 };

#endif