/**/
/*--- object dispatcher*/
/**/
#pragma libcall AwebPluginBase AmethodA 1e 9802
#pragma libcall AwebPluginBase AmethodasA 24 98003
/* */
/*--- object methods*/
/* */
#pragma libcall AwebPluginBase AnewobjectA 2a 8002
#pragma libcall AwebPluginBase Adisposeobject 30 801
#pragma libcall AwebPluginBase AsetattrsA 36 9802
#pragma libcall AwebPluginBase AgetattrsA 3c 9802
#pragma libcall AwebPluginBase Agetattr 42 0802
#pragma libcall AwebPluginBase AupdateattrsA 48 A9803
#pragma libcall AwebPluginBase Arender 4e A432109808
#pragma libcall AwebPluginBase Aaddchild 54 09803
#pragma libcall AwebPluginBase Aremchild 5a 09803
#pragma libcall AwebPluginBase Anotify 60 9802
/* */
/*--- object support*/
/* */
#pragma libcall AwebPluginBase Allocobject 66 81003
#pragma libcall AwebPluginBase AnotifysetA 6c 9802
/* */
/*--- memory*/
/* */
#pragma libcall AwebPluginBase Allocmem 72 1002
#pragma libcall AwebPluginBase Dupstr 78 0802
#pragma libcall AwebPluginBase Freemem 7e 801
/* */
/*--- render*/
/* */
#pragma libcall AwebPluginBase Clipcoords 84 9802
#pragma libcall AwebPluginBase Unclipcoords 8a 801
#pragma libcall AwebPluginBase Erasebg 90 32109806
/* */
/*--- application*/
/* */
#pragma libcall AwebPluginBase Aweb 96 0
/* */
/*--- task*/
/* */
#pragma libcall AwebPluginBase AsetattrsasyncA 9c 9802
#pragma libcall AwebPluginBase Waittask a2 001
#pragma libcall AwebPluginBase Gettaskmsg a8 0
#pragma libcall AwebPluginBase Replytaskmsg ae 801
#pragma libcall AwebPluginBase Checktaskbreak b4 0
#pragma libcall AwebPluginBase Updatetask ba 801
#pragma libcall AwebPluginBase UpdatetaskattrsA c0 801
#pragma libcall AwebPluginBase Obtaintasksemaphore c6 801
/**/
/*--- debug*/
/**/
#pragma libcall AwebPluginBase Avprintf cc 9802
/**/
/*pragma libcall AwebPluginBase Awebprivate1 d2 0*/
/*pragma libcall AwebPluginBase Awebprivate2 d8 0*/
/*pragma libcall AwebPluginBase Awebprivate3 de 0*/
/*pragma libcall AwebPluginBase Awebprivate4 e4 0*/
/*pragma libcall AwebPluginBase Awebprivate5 ea 0*/
/*pragma libcall AwebPluginBase Awebprivate6 f0 0*/
/*pragma libcall AwebPluginBase Awebprivate7 f6 0*/
/*pragma libcall AwebPluginBase Awebprivate8 fc 0*/
/*pragma libcall AwebPluginBase Awebprivate9 102 0*/
/*pragma libcall AwebPluginBase Awebprivate10 108 0*/
/*pragma libcall AwebPluginBase Awebprivate11 10e 0*/
/*pragma libcall AwebPluginBase Awebprivate12 114 0*/
/**/
/*--- render*/
/**/
#pragma libcall AwebPluginBase Obtainbgrp 11a 32109806
#pragma libcall AwebPluginBase Releasebgrp 120 801
/**/
/*pragma libcall AwebPluginBase AWebprivate13 126 0*/
/*pragma libcall AwebPluginBase AWebprivate14 12c 0*/
/*pragma libcall AwebPluginBase AWebprivate15 132 0*/
/*pragma libcall AwebPluginBase AWebprivate16 138 0*/
/*pragma libcall AwebPluginBase AWebprivate17 13e 0*/
/*pragma libcall AwebPluginBase AWebprivate18 144 0*/
/*pragma libcall AwebPluginBase AWebprivate19 14a 0*/
/*pragma libcall AwebPluginBase AWebprivate20 150 0*/
/*pragma libcall AwebPluginBase AWebprivate21 156 0*/
/*pragma libcall AwebPluginBase AWebprivate22 15c 0*/
/*pragma libcall AwebPluginBase AWebprivate23 162 0*/
/*pragma libcall AwebPluginBase AWebprivate24 168 0*/
/*pragma libcall AwebPluginBase AWebprivate25 16e 0*/
/*pragma libcall AwebPluginBase AWebprivate26 174 0*/
/*pragma libcall AwebPluginBase AWebprivate27 17a 0*/
/*pragma libcall AwebPluginBase AWebprivate28 180 0*/
/*pragma libcall AwebPluginBase AWebprivate29 186 0*/
/*pragma libcall AwebPluginBase AWebprivate30 18c 0*/
/*pragma libcall AwebPluginBase AWebprivate31 192 0*/
/*pragma libcall AwebPluginBase AWebprivate32 198 0*/
/*pragma libcall AwebPluginBase AWebprivate33 19e 0*/
/*pragma libcall AwebPluginBase AWebprivate34 1a4 0*/
/*pragma libcall AwebPluginBase AWebprivate35 1aa 0*/
/**/
/* Library version 2.0*/
/**/
/*--- filter type plugins*/
/**/
#pragma libcall AwebPluginBase Setfiltertype 1b0 9802
#pragma libcall AwebPluginBase Writefilter 1b6 09803
/**/
/*--- commands*/
/**/
#pragma libcall AwebPluginBase Awebcommand 1bc 198004
/**/

#ifdef __SASC
#pragma tagcall AwebPluginBase Amethod 1e 9802
#pragma tagcall AwebPluginBase Amethodas 24 98003
#pragma tagcall AwebPluginBase Anewobject 2a 8002
#pragma tagcall AwebPluginBase Asetattrs 36 9802
#pragma tagcall AwebPluginBase Agetattrs 3c 9802
#pragma tagcall AwebPluginBase Aupdateattrs 48 A9803
#pragma tagcall AwebPluginBase Anotifyset 6c 9802
#pragma tagcall AwebPluginBase Asetattrsasync 9c 9802
#pragma tagcall AwebPluginBase Updatetaskattrs c0 801
#pragma tagcall AwebPluginBase Aprintf cc 9802
#endif
