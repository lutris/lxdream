/**
 * $Id$
 *
 * Parent for all glx-based display drivers.
 *
 * Copyright (c) 2005 Nathan Keynes.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef lxdream_video_glx_H
#define lxdream_video_glx_H 1

#include "X11/Xlib.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GLX support. Detect capabilities, visuals, etc. 
 * Must be called before any other GLX functions
 */
gboolean video_glx_init( Display *display, int screen );

/**
 * Return the prefered visual to be used for the GL window.
 * (Not using this for the render window may cause init context
 * to fail).
 */
XVisualInfo *video_glx_get_visual();

/**
 * Initialize the GLX context and bind to the specified window.
 * (which should have been created with the visual returned above).
 */
gboolean video_glx_init_context( Display *display, Window window );

/**
 * Initialize the display driver by setting the appropriate methods
 * for GLX support
 */
gboolean video_glx_init_driver( display_driver_t driver );

/**
 * Shutdown GLX support and release all resources.
 */
void video_glx_shutdown();

#ifdef __cplusplus
}
#endif

#endif
