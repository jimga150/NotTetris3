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
        
    void colorizeResources() override;
    
    void resetBlinkTimer() override;
    
    
    QPixmap background = QPixmap(":/resources/graphics/gametype.png");
    
    int option_left_x = 24;
    int option_right_x = 88;
    
    int option_gt_y = 26;
    int option_mt_top_y = 60;
    int option_mt_bot_y = 76;
            
    
    optionTracker option_group = optionTracker(num_option_groups, GAME_TYPE);
    optionTracker option_groups[num_option_groups];
    
    uint prevOG = option_group.current_opt;
    
    std::vector<QString> option_names[num_option_groups];
};

#endif // MENU1P_H
