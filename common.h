#ifndef COMMON_H
#define COMMON_H

#define TO_QRECT(QRF, SCALE) QRect(static_cast<int>(QRF.x()*SCALE), static_cast<int>(QRF.y()*SCALE), static_cast<int>(QRF.width()*SCALE), static_cast<int>(QRF.height()*SCALE))

enum NT3_state_enum{
    LOGO = 0,
    CREDITS,
    MAINMENU,
    GAMEA,
    
    num_nt3_states
};

#endif // COMMON_H
