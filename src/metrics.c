/*
 *	metrics.h
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

#include <stdio.h>
#include <stdlib.h>

double cpuLoad(void)
{
    float loadavg;
    FILE *load;
    load = fopen("/proc/loadavg", "r");
    fscanf(load, "%f", &loadavg);
    fclose(load);
    return (double)(100 * loadavg);
} 

double cpuTemp(void)
{
    float millideg;
    FILE *thermo;
    thermo = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    fscanf(thermo, "%f", &millideg);
    fclose(thermo);
    return (int)(millideg / 10.0f) / 100.0f;
}
