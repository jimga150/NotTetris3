#ifndef COMMON_H
#define COMMON_H

#include <QtMath>
#include <QString>

//constants
#define MILLIS_PER_SECOND (1000)
#define MICROS_PER_SECOND (1000000)
#define NANOS_PER_SECOND  (1000000000)

#define DEG_PER_RAD (57.295779513082321)

#define TO_QRECT(QRF, SCALE) QRect( \
    static_cast<int>(QRF.x()*SCALE), \
    static_cast<int>(QRF.y()*SCALE), \
    static_cast<int>(QRF.width()*SCALE), \
    static_cast<int>(QRF.height()*SCALE))


#define DEFAULT_VOLUME (10)
#define DEFAULT_HUE (0.08)
#define DEFAULT_FULLSCREEN false

#define MAX_VOLUME (100)
#define MAX_HUE (1.0)

#define MIN_VOLUME (0.0)
#define MIN_HUE (0.0)

//global options
extern int volume; //{0->100}
const double volume_sfx_multiplier = 1.0/100.0;
extern double hue; //{0->1}, 0.08 is default
extern bool fullscreen;

extern double framerate;

enum NT3_state_enum{
    LOGO = 0,
    CREDITS,
    MAINMENU,
    P_GAMEOPTIONS,
    PP_GAMEOPTIONS,
    GLOBAL_OPTIONS,
    GAMEA,
    GAMEB,
    GAME_LOST,
    
    num_nt3_states
};

enum music_type_enum {
    ATYPE = 0,
    BTYPE,
    CTYPE,
    OFF,
    
    num_music_types
};
extern music_type_enum music_type;
extern QString* music_urls;

struct optionTracker{
    uint default_opt;
    uint current_opt;
    uint first = 0;
    uint last;
    
    optionTracker(){}
    
    optionTracker(uint num_options, uint default_opt){
        this->first = 0;
        this->last = num_options - 1;
        this->current_opt = default_opt;
        this->default_opt = default_opt;
    }
    
    void increment(){
        if (this->current_opt == this->last) this->current_opt = this->first;
        else this->current_opt++;
    }
    
    void decrement(){
        if (this->current_opt == this->first) this->current_opt = this->last;
        else this->current_opt--;
    }
    
    void reset(){
        this->current_opt = this->default_opt;
    }
};

#endif // COMMON_H
