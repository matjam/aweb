/**********************************************************************
 * 
 * This file is part of the AWeb-II distribution
 *
 * Copyright (C) 2002 Yvon Rozijn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the AWeb Public License as included in this
 * distribution.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * AWeb Public License for more details.
 *
 **********************************************************************/

/* gst.c - aweb gst */

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <datatypes/soundclass.h>
#include <devices/audio.h>
#include <devices/clipboard.h>
#include <devices/printer.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/stdio.h>
#include <dos/rdargs.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/displayinfo.h>
#include <graphics/scale.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <intuition/pointerclass.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <utility/date.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <cybergraphics/cybergraphics.h>
#include <classact.h>
#include <classact_author.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include <ezlists.h>

#include <clib/alib_protos.h>
#include <clib/asl_protos.h>
#include <clib/colorwheel_protos.h>
#include <clib/datatypes_protos.h>
#include <clib/diskfont_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/icon_protos.h>
#include <clib/iffparse_protos.h>
#include <clib/intuition_protos.h>
#include <clib/keymap_protos.h>
#include <clib/layers_protos.h>
#include <clib/locale_protos.h>
#include <clib/timer_protos.h>
#include <clib/utility_protos.h>
#include <clib/wb_protos.h>
#include <clib/cybergraphics_protos.h>

#include <pragmas/asl_pragmas.h>
#include <pragmas/colorwheel_pragmas.h>
#include <pragmas/datatypes_pragmas.h>
#include <pragmas/diskfont_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_sysbase_pragmas.h>
#include <pragmas/gadtools_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/icon_pragmas.h>
#include <pragmas/iffparse_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/keymap_pragmas.h>
#include <pragmas/layers_pragmas.h>
#include <pragmas/locale_pragmas.h>
#include <pragmas/timer_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/wb_pragmas.h>
#include <pragmas/cybergraphics_pragmas.h>

