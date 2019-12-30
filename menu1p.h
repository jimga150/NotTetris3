#ifndef MENU1P_H
#define MENU1P_H

#include "nt3screen.h"

enum option_group_enum {
    GAME_TYPE = 0,
    MUSIC_TYPE,
    
    num_option_groups
};

enum game_type_enum {
    NORMAL = 0,
    STACK,
    
    num_game_types
};

enum music_type_enum {
    ATYPE = 0,
    BTYPE,
    CTYPE,
    OFF,
    
    num_music_types
};

class Menu1P : public NT3Screen
{
    Q_OBJECT
public:
    explicit Menu1P(QObject *parent = nullptr);
    
    void init() override;
    
    
    void calcScaleFactors() override;    
    
    void render(QPainter& painter) override;
    
    
    void keyPressEvent(QKeyEvent* ev) override;
        
    void doGameStep() override;
    
    void colorizeResources() override;
    
    
    QPixmap background = QPixmap(":/resources/graphics/gametype.png");
    
    double time_passed;
    
    bool blink_on;
};

#endif // MENU1P_H
