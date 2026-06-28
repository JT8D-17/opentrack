/* Copyright (c) 2013, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>

#include <XPLMPlugin.h>
#include <XPLMDataAccess.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

#ifndef PLUGIN_API
#define PLUGIN_API
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"

/* using Wine name to ease things */
#define WINE_SHM_NAME "facetracknoir-wine-shm"
#define WINE_MTX_NAME "facetracknoir-wine-mtx"

#include "compat/linkage-macros.hpp"

#ifndef MAP_FAILED
#   define MAP_FAILED ((void*)-1)
#endif

#ifdef __GNUC__
#   pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#   pragma GCC diagnostic ignored "-Wdouble-promotion"
#   pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif

enum Axis {
    TX = 0, TY, TZ, Yaw, Pitch, Roll
};

typedef struct shm_wrapper
{
    void* mem;
    int fd, size;
} shm_wrapper;

typedef struct WineSHM
{
    double data[6];
    int gameid, gameid2;
    unsigned char table[8];
    bool stop;
} volatile WineSHM;

static int VERSION = 2021-10-15;
static int debug_strings = 0; //Enable debug strings
static shm_wrapper* lck_posix = NULL;
static WineSHM* shm_posix = NULL;
static void *view_x, *view_y, *view_z, *view_heading, *view_pitch, *view_roll;
static float offset_x, offset_y, offset_z;
static void *xcam_mode, *xcam_ht_on, *xcam_offset_h, *xcam_offset_p, *xcam_offset_r, *xcam_offset_x, *xcam_offset_y, *xcam_offset_z;
static XPLMCommandRef track_toggle = NULL, track_on = NULL, track_off = NULL, translation_disable_toggle = NULL;
static XPLMDataRef StatusDataRef = NULL;
static int track_status = 0;
static int translation_disabled = 0;
static int XCameraStatus = 0;

static void get_head_xyz() {
    offset_x = XPLMGetDataf(view_x);
    offset_y = XPLMGetDataf(view_y);
    offset_z = XPLMGetDataf(view_z);
}

shm_wrapper* shm_wrapper_init(const char *shm_name, const char *mutex_name, int mapSize)
{
    (void)mutex_name;
    shm_wrapper* self = malloc(sizeof(shm_wrapper));
    char shm_filename[NAME_MAX];
    shm_filename[0] = '/';
    strncpy(shm_filename+1, shm_name, NAME_MAX-2);
    shm_filename[NAME_MAX-1] = '\0';
    /* (void) shm_unlink(shm_filename); */

    self->fd = shm_open(shm_filename, O_RDWR | O_CREAT, 0600);
    (void) ftruncate(self->fd, mapSize);
    self->mem = mmap(NULL, mapSize, PROT_READ|PROT_WRITE, MAP_SHARED, self->fd, (off_t)0);
    return self;
}

void shm_wrapper_free(shm_wrapper* self)
{
    /*(void) shm_unlink(shm_filename);*/
    (void) munmap(self->mem, self->size);
    (void) close(self->fd);
    free(self);
}

void shm_wrapper_lock(shm_wrapper* self)
{
    flock(self->fd, LOCK_SH);
}

void shm_wrapper_unlock(shm_wrapper* self)
{
    flock(self->fd, LOCK_UN);
}

float write_head_position(float inElapsedSinceLastCall,
                          float inElapsedTimeSinceLastFlightLoop,
                          int   inCounter,
                          void* inRefcon)
{
    if (lck_posix != NULL && shm_posix != NULL) {
        shm_wrapper_lock(lck_posix);
        if (translation_disabled == 0)
        {
			if (XCameraStatus < 2) {
				XPLMSetDataf(view_x, shm_posix->data[TX] * 1e-3 + offset_x);
				XPLMSetDataf(view_y, shm_posix->data[TY] * 1e-3 + offset_y);
				XPLMSetDataf(view_z, shm_posix->data[TZ] * 1e-3 + offset_z);
			} else {
				if (XPLMGetDatai(xcam_mode) == 2) {
					XPLMSetDataf(xcam_offset_x, shm_posix->data[TX] * 1e-3);
					XPLMSetDataf(xcam_offset_y, shm_posix->data[TY] * 1e-3);
					XPLMSetDataf(xcam_offset_z, shm_posix->data[TZ] * 1e-3);
				}
			}
        }
        if (XCameraStatus < 2) {
			XPLMSetDataf(view_heading, shm_posix->data[Yaw] * 180 / M_PI);
			XPLMSetDataf(view_pitch, shm_posix->data[Pitch] * 180 / M_PI);
			XPLMSetDataf(view_roll, shm_posix->data[Roll] * 180 / M_PI);

		} else {
			if (XPLMGetDatai(xcam_mode) == 2) {
				XPLMSetDataf(xcam_offset_h, shm_posix->data[Yaw] * 180 / M_PI);
				XPLMSetDataf(xcam_offset_p, shm_posix->data[Pitch] * 180 / M_PI);
				XPLMSetDataf(xcam_offset_r, shm_posix->data[Roll] * 180 / M_PI);
			}
		}
		shm_wrapper_unlock(lck_posix);
    }
    return -1.0;
}

/*
 * Debug strings
 */
static void debug_status_output()
{
    char message[128];
    if (track_status == 1)
        sprintf(message,"Opentrack: Enabled tracking\n");
    if (track_status == 0)
        sprintf(message,"Opentrack: Disabled tracking\n");
    XPLMDebugString(message);
}

static void debug_keyrelease_output()
{
    char message[128];
    sprintf(message,"Opentrack: Registered a key release\n");
    XPLMDebugString(message);
}

static void debug_translation_output()
{
    char message[128];
    if (translation_disabled == 0)
        sprintf(message,"Opentrack: Enabled translation\n");
    if (translation_disabled == 1)
        sprintf(message,"Opentrack: Disbled translation\n");
    XPLMDebugString(message);
}

static void debug_xcam_status_output()
{
	char message[128];
	if (XCameraStatus == 2)
		sprintf(message,"Opentrack: X-Camera output started\n");
	if (XCameraStatus == 1)
		sprintf(message,"Opentrack: X-Camera output stopped\n");
	XPLMDebugString(message);
}
/*
 * X-Camera initialization
 */
 static void Xcam_init()
 {
	xcam_mode = XPLMFindDataRef("SRS/X-Camera/integration/X-Camera_enabled"); //2 = tir mode 1 = enabled
	xcam_ht_on = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_present");
	xcam_offset_h = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_heading_offset");
	xcam_offset_p = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_pitch_offset");
	xcam_offset_r = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_roll_offset");
	xcam_offset_x = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_x_offset");
	xcam_offset_y = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_y_offset");
	xcam_offset_z = XPLMFindDataRef("SRS/X-Camera/integration/headtracking_z_offset");

	if (xcam_mode && xcam_ht_on && xcam_offset_h && xcam_offset_p && xcam_offset_r && xcam_offset_x && xcam_offset_y && xcam_offset_z) {
		XCameraStatus = 2;
		XPLMSetDatai(xcam_ht_on,1);
		if (debug_strings == 1) { debug_xcam_status_output(); }
	}
 }

 static void Xcam_deinit()
 {
	 XCameraStatus = 1;
	 XPLMSetDatai(xcam_ht_on,0);
	 if (debug_strings == 1) { debug_xcam_status_output(); }
 }
/*
 *Flight loop callback handling
 */
static void flightloop_handler()
{
    if (debug_strings == 1) { debug_keyrelease_output(); }
    if (track_status == 1) {
		/* */
		if (XCameraStatus == 1) { Xcam_init(); }
		else{ get_head_xyz(); }
        XPLMRegisterFlightLoopCallback(write_head_position, -1, NULL);
    }
    if (track_status == 0) {
		if (XCameraStatus > 1) { Xcam_deinit(); }
        XPLMUnregisterFlightLoopCallback(write_head_position, NULL);
    }
    if (debug_strings == 1) { debug_status_output(); }
}

/*
 *Command Handlers
 */

static int TrackToggleHandler(XPLMCommandRef inCommand,
                              XPLMCommandPhase inPhase,
                              void* inRefCon)
{
    /* Only perform this action upon command release */
    if (inPhase == 2) {
        if (track_status == 0) {
            track_status = 1;
            flightloop_handler();
        }
        else if (track_status == 1) {
            track_status = 0;
            flightloop_handler();
        }
    }
    return 0;
}


static int TrackOnHandler(XPLMCommandRef inCommand,
                            XPLMCommandPhase inPhase,
                            void* inRefCon)
{
    /* Only perform this action upon command release */
    if (inPhase == 2) {
        track_status = 1;
        flightloop_handler();
    }
    return 0;
}

static int TrackOffHandler(XPLMCommandRef inCommand,
                            XPLMCommandPhase inPhase,
                            void* inRefCon)
{
    /* Only perform this action upon command release */
    if (inPhase == 2) {
        track_status = 0;
        flightloop_handler();
    }
    return 0;
}

static int TranslationToggleHandler(XPLMCommandRef inCommand,
                                    XPLMCommandPhase inPhase,
                                    void* inRefCon)
{
    /* Only perform this action upon command release */
    if (inPhase == 2) {
        if (debug_strings == 1) { debug_keyrelease_output(); }
        if (translation_disabled == 1) {
            translation_disabled = 0;
            if (debug_strings == 1) { debug_translation_output(); }
            //get_head_xyz();
        }
        else if (translation_disabled == 0) {
            translation_disabled = 1;
            if (debug_strings == 1) { debug_translation_output(); }
        }
    }
    return 0;
}

/* Dataref Handlers */

static int GetDataiCallback(void * inRefcon)
{
    return track_status;
}

void SetDataiCallback(void * inRefcon, int inValue)
{
    track_status = inValue;
    flightloop_handler();
}

static inline
void volatile_explicit_bzero(void volatile* restrict ptr, size_t len)
{
    for (size_t i = 0; i < len; i++)
        *((char volatile* restrict)ptr + i) = 0;

    asm volatile("" ::: "memory");
}

PLUGIN_API OTR_GENERIC_EXPORT
int XPluginStart (char* outName, char* outSignature, char* outDescription) {

    view_x = XPLMFindDataRef("sim/graphics/view/pilots_head_x");
    view_y = XPLMFindDataRef("sim/graphics/view/pilots_head_y");
    view_z = XPLMFindDataRef("sim/graphics/view/pilots_head_z");
    view_heading = XPLMFindDataRef("sim/graphics/view/pilots_head_psi");
    view_pitch = XPLMFindDataRef("sim/graphics/view/pilots_head_the");
    view_roll = XPLMFindDataRef("sim/graphics/view/pilots_head_phi");

    track_toggle = XPLMCreateCommand("Opentrack/Toggle", "Toggle Head Tracking");
    track_on = XPLMCreateCommand("Opentrack/On", "Enable Head Tracking");
	track_off = XPLMCreateCommand("Opentrack/Off", "Disable Head Tracking");
	translation_disable_toggle = XPLMCreateCommand("Opentrack/Toggle_Translation", "Toggle Input Translation");


    XPLMRegisterCommandHandler(track_toggle,TrackToggleHandler,1,NULL);
    XPLMRegisterCommandHandler(track_on,TrackOnHandler,1,NULL);
    XPLMRegisterCommandHandler(track_off,TrackOffHandler,1,NULL);
    XPLMRegisterCommandHandler(translation_disable_toggle,TranslationToggleHandler,1,NULL);

    StatusDataRef = XPLMRegisterDataAccessor(
								"Opentrack/Tracking",
								xplmType_Int,								/* The types we support */
								1,											/* Writable */
								GetDataiCallback, SetDataiCallback,			/* No accessors for ints */
								NULL, NULL,									/* No accessors for floats */
								NULL, NULL,									/* No accessors for doubles */
								NULL, NULL,									/* No accessors for int arrays */
								NULL, NULL,									/* No accessors for float arrays */
								NULL, NULL,									/* No accessors for raw data */
								NULL, NULL);								/* Refcons not used */

    if (view_x && view_y && view_z && view_heading && view_pitch && track_toggle && track_on && track_off && translation_disable_toggle) {
        lck_posix = shm_wrapper_init(WINE_SHM_NAME, WINE_MTX_NAME, sizeof(WineSHM));
        if (lck_posix->mem == MAP_FAILED) {
            fprintf(stderr, "Opentrack: Failed to init SHM!\n");
            return 0;
        }
        shm_posix = lck_posix->mem;
        volatile_explicit_bzero(shm_posix, sizeof(WineSHM));
        strcpy(outName, "Opentrack");
        strcpy(outSignature, "bk_sthalik.Opentrack");
        strcpy(outDescription, "Head tracking plugin for use with Opentrack");
        //Debug
        if (debug_strings == 1) {
            char message[128];
            sprintf(message,"Opentrack: Plugin v%.2f loaded\n", VERSION);
            XPLMDebugString(message);
        }
        fprintf(stderr, "Opentrack: Init complete\n");

        return 1;
    }
    return 0;
}

PLUGIN_API OTR_GENERIC_EXPORT
void XPluginStop (void) {
    if (lck_posix)
    {
        shm_wrapper_free(lck_posix);
        lck_posix = NULL;
        shm_posix = NULL;
    }
    if (StatusDataRef)
		XPLMUnregisterDataAccessor(StatusDataRef);
}

PLUGIN_API OTR_GENERIC_EXPORT
int XPluginEnable (void) {

	XPLMPluginID XCam = XPLMFindPluginBySignature("SRS.X-Camera");
	if (XCam != XPLM_NO_PLUGIN_ID) {
		if (debug_strings == 1) {
			char message[128];
			sprintf(message,"Opentrack: X-Camera plugin found (ID %d); enabling output mode\n",XCam);
			XPLMDebugString(message);
		}
		XCameraStatus = 1;
	}
    return 1;
}

PLUGIN_API OTR_GENERIC_EXPORT
void XPluginDisable (void) {
	if (XCameraStatus != 0) {
		XCameraStatus = 0;
		if (debug_strings == 1) {
			char message[128];
			sprintf(message,"Opentrack: Disabling X-Camera output mode\n");
			XPLMDebugString(message);
		}
	}
    track_status = 0;
    flightloop_handler();


}

PLUGIN_API OTR_GENERIC_EXPORT
void XPluginReceiveMessage(XPLMPluginID    inFromWho,
                           int             inMessage,
                           void *          inParam)
{
    /*if (inMessage == XPLM_MSG_AIRPORT_LOADED) {
        get_head_xyz();
    }*/
}
