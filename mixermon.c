/*
 *	(c) 2015 László TÓTH
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
 */

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <alsa/asoundlib.h>

#include "common.h"

#define SLEEP_TIME	(25000/25)
#define CNLENGTH    64

char        device_name[CNLENGTH];
char        card[CNLENGTH];
snd_mixer_t *handle;
pthread_t   seventsThread;
char        stbm[BSIZE];
long actVolume  = 0;

long getActVolume(void) {
    return actVolume;
}

static void sevents_value(snd_mixer_selem_id_t *sid) {
	long min, max, currentVolume;
	const char *selem_name;

	selem_name = snd_mixer_selem_id_get_name(sid);
//	printf("event value: '%s',%i\n", selem_name, snd_mixer_selem_id_get_index(sid));

	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
//	printf("Returned card VOLUME range - min: %ld, max: %ld\n", min, max);

	// Minimum given is mute, we need the first real value
	min++;

    if (max-min == 0) {return;} // prevent to on/off devices cause dividing zero

	// Get current volume
	if (snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &currentVolume) == 0) {
		actVolume = (currentVolume*100)/(max-min);
//		printf("actVolume = %ld (currentVolume = %ld)\n", actVolume, currentVolume);
   	}
}
/*
static void sevents_info(snd_mixer_selem_id_t *sid) {
	printf("event info: '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
}

static void sevents_remove(snd_mixer_selem_id_t *sid) {
	printf("event remove: '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
}
*/
static int melem_event(snd_mixer_elem_t *elem, unsigned int mask) {
	snd_mixer_selem_id_t *sid;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_get_id(elem, sid);
/*
	if (mask == SND_CTL_EVENT_MASK_REMOVE) {
		sevents_remove(sid);
		return 0;
	}

	if (mask & SND_CTL_EVENT_MASK_INFO) {
		sevents_info(sid);
	}
*/
	if (mask & SND_CTL_EVENT_MASK_VALUE) {
		sevents_value(sid);
	}

	return 0;
}

/*
static void sevents_add(snd_mixer_elem_t *elem) {
	snd_mixer_selem_id_t *sid;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_get_id(elem, sid);
//	printf("event add: '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
	snd_mixer_elem_set_callback(elem, melem_event);
}
*/
static int mixer_event(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem) {
	snd_mixer_selem_id_t *sid;

	if (mask & SND_CTL_EVENT_MASK_ADD) {
		snd_mixer_selem_id_alloca(&sid);
		snd_mixer_selem_get_id(elem, sid);
//	printf("event add: '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
		snd_mixer_elem_set_callback(elem, melem_event);
//		sevents_add(elem);
	}
	return 0;
}

void *sevents(void *x_voidptr){
	int err;

	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		perror("Mixer open error.\n");
		return NULL;
	}

	if ((err = snd_mixer_attach(handle, card)) < 0) {
		perror("Mixer attach error.\n");
		snd_mixer_close(handle);
		return NULL;
	}

	if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
		perror("Mixer register error.\n");
		snd_mixer_close(handle);
		return NULL;
	}

	snd_mixer_set_callback(handle, mixer_event);
	if ((err = snd_mixer_load(handle)) < 0) {
		perror("Mixer load error.\n");
		snd_mixer_close(handle);
		return NULL;;
	}

//	printf("Ready to listen...\n");
	while (1) {
		int res;
		res = snd_mixer_wait(handle, -1);
		if (res >= 0) {
			// printf("Poll ok: %i\n", res);
			res = snd_mixer_handle_events(handle);
			assert(res >= 0);
		}
	}

	snd_mixer_close(handle);
	return NULL;
}


int startMimo(char *cName, char *dName) {
	int x = 0;

    if (cName != NULL) {
        strncpy(card, cName, CNLENGTH);
    } else {
        strncpy(card, "default", CNLENGTH);
    }

    if (dName != NULL) {
        strncpy(device_name, dName, CNLENGTH);
    } else {
        strncpy(device_name, "default", CNLENGTH);
    }

    sprintf(stbm, "Init ALSA wint CARD:%s and DEVICE:%s\n", card, device_name);
    putMSG(stbm, LL_INFO);

	if (pthread_create(&seventsThread, NULL, sevents, &x) != 0) {
		abort("Failed to create ALSA mixer monitoring thread!");
	}

	return EXIT_SUCCESS;
}
