#ifndef COMMON_H
#define COMMON_H

#include <QtMath>

//constants
#define MILLIS_PER_SECOND (1000)
#define MICROS_PER_SECOND (1000000)
#define NANOS_PER_SECOND (10000000000)

#define RAD_TO_DEG (57.295779513082321)

#define TO_QRECT(QRF, SCALE) QRect(static_cast<int>(QRF.x()*SCALE), static_cast<int>(QRF.y()*SCALE), static_cast<int>(QRF.width()*SCALE), static_cast<int>(QRF.height()*SCALE))

enum NT3_state_enum{
    LOGO = 0,
    CREDITS,
    MAINMENU,
    P_GAMEOPTIONS,
    PP_GAMEOPTIONS,
    GLOBAL_OPTIONS,
    GAMEA,
    
    num_nt3_states
};

extern double framerate;

//global options
extern double volume; //{0->1}
extern double hue; //{0->1}, 0.08 is default
extern bool fullscreen;

#define DEFAULT_VOLUME (1.0)
#define DEFAULT_HUE (0.08)
#define DEFAULT_FULLSCREEN false

#endif // COMMON_H
