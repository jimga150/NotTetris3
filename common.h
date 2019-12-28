#ifndef COMMON_H
#define COMMON_H

#include <QtMath>

//constants
#define MILLIS_PER_SECOND 1000
#define MICROS_PER_SECOND MILLIS_PER_SECOND*1000
#define NANOS_PER_SECOND MICROS_PER_SECOND*1000

#define RAD_TO_DEG 57.295779513082321

#define TO_QRECT(QRF, SCALE) QRect(static_cast<int>(QRF.x()*SCALE), static_cast<int>(QRF.y()*SCALE), static_cast<int>(QRF.width()*SCALE), static_cast<int>(QRF.height()*SCALE))

enum NT3_state_enum{
    LOGO = 0,
    CREDITS,
    MAINMENU,
    GAMEA,
    
    num_nt3_states
};

#endif // COMMON_H
