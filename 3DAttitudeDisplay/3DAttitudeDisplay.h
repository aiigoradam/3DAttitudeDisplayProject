/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  CONFIG                           1
#define  CONFIG_CONFIRM                   2       /* control type: command, callback function: ConfigCallback */
#define  CONFIG_STOPBITS                  3       /* control type: slide, callback function: (none) */
#define  CONFIG_DATABITS                  4       /* control type: slide, callback function: (none) */
#define  CONFIG_PARITY                    5       /* control type: slide, callback function: (none) */
#define  CONFIG_BAUDRATE                  6       /* control type: slide, callback function: (none) */

#define  PANEL                            2       /* callback function: PanelCallback */
#define  PANEL_TEXTBOX                    2       /* control type: textBox, callback function: TextBoxCallback */
#define  PANEL_ROLL                       3       /* control type: numeric, callback function: (none) */
#define  PANEL_YAW                        4       /* control type: numeric, callback function: (none) */
#define  PANEL_INQUE                      5       /* control type: numeric, callback function: (none) */
#define  PANEL_PITCH                      6       /* control type: numeric, callback function: (none) */
#define  PANEL_DISCONNECT                 7       /* control type: command, callback function: DisconnectButtonCallback */
#define  PANEL_CONNECT                    8       /* control type: command, callback function: ConnectButtonCallback */
#define  PANEL_HIDELOG                    9       /* control type: pictRing, callback function: ViewLogCallback */
#define  PANEL_VIEWLOG                    10      /* control type: pictRing, callback function: ViewLogCallback */
#define  PANEL_3D_IMAGE                   11      /* control type: picture, callback function: OGLCallback */
#define  PANEL_SCROLLDOWN                 12      /* control type: pictRing, callback function: ScrollDownCallback */
#define  PANEL_CANVAS                     13      /* control type: canvas, callback function: ViewLogCallback */
#define  PANEL_RING                       14      /* control type: ring, callback function: RingCallback */
#define  PANEL_VIEWPLANE                  15      /* control type: slide, callback function: ViewPlaneCallback */
#define  PANEL_CONFIG                     16      /* control type: command, callback function: ConfigCallback */
#define  PANEL_DECORATION                 17      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_2               18      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_3               19      /* control type: deco, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK ConfigCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ConnectButtonCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK DisconnectButtonCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK OGLCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RingCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ScrollDownCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TextBoxCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ViewLogCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ViewPlaneCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif