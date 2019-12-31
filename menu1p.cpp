#include "menu1p.h"

Menu1P::Menu1P(QObject *parent) : NT3Screen(parent)
{
    for (uint og = 0; og < num_option_groups; ++og){
        switch (og) {
        case GAME_TYPE:
            this->option_groups[og] = optionTracker(num_game_types, NORMAL);
            for (uint option = 0; option < num_game_types; ++option){
                switch (option) {
                case NORMAL:
                    this->option_names[og].push_back("normal");
                    break;
                case STACK:
                    this->option_names[og].push_back("stack");
                    break;
                default:
                    this->option_names[og].push_back(this->default_option_name);
                    break;
                }
            }
            break;
        case MUSIC_TYPE:
            this->option_groups[og] = optionTracker(num_music_types, ATYPE);
            for (uint option = 0; option < num_music_types; ++option){
                switch (option) {
                case ATYPE:
                    this->option_names[og].push_back("a-type");
                    break;
                case BTYPE:
                    this->option_names[og].push_back("b-type");
                    break;
                case CTYPE:
                    this->option_names[og].push_back("c-type");
                    break;
                case OFF:
                    this->option_names[og].push_back(" off");
                    break;
                default:
                    this->option_names[og].push_back(this->default_option_name);
                    break;
                }
            }
            break;
        default:
            fprintf(stderr, "Undefined option group! %u\n", og);
            break;
        }
    }
}

void Menu1P::init(){
    this->resetBlinkTimer();
    this->option_group.reset();
    for (uint og = 0; og < num_option_groups; ++og){
        this->option_groups[og].reset();
    }
}

void Menu1P::calcScaleFactors(){
    
}    

void Menu1P::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    
    for (uint opt_group = 0; opt_group < num_option_groups; ++opt_group){
        if (opt_group == this->option_group.current_opt && !this->blink_on){
            continue;
        }
        
        uint opt_col = this->option_groups[opt_group].current_opt;
        
        int x, y;
        switch(opt_group){
        case GAME_TYPE:
            y = this->option_gt_y;
            x = this->option_groups[GAME_TYPE].current_opt == NORMAL ? this->option_left_x : this->option_right_x;
            break;
        case MUSIC_TYPE:
            if (this->option_groups[MUSIC_TYPE].current_opt == ATYPE || 
                this->option_groups[MUSIC_TYPE].current_opt == BTYPE){
                y = this->option_mt_top_y;
            } else {
                y = this->option_mt_bot_y;
            }
            if (this->option_groups[MUSIC_TYPE].current_opt == ATYPE || 
                this->option_groups[MUSIC_TYPE].current_opt == CTYPE){
                x = this->option_left_x;
            } else {
                x = this->option_right_x;
            }
            break;
        default:
            fprintf(stderr, "Undefined option group! %u\n", opt_group);
            y = 0;
            x = 0;
            break;
        }
        
        this->BOW_font.print(&painter, QPoint(x, y)*this->ui_scale, LEFT_ALIGN, 
                             this->option_names[opt_group][opt_col], this->ui_scale);
    }
}

void Menu1P::keyPressEvent(QKeyEvent* ev){
    
    music_type_enum currMT = static_cast<music_type_enum>(this->option_groups[MUSIC_TYPE].current_opt);
    
    switch(ev->key()){
    case Qt::Key_Left:
        this->option_groups[this->option_group.current_opt].decrement();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Right:
        this->option_groups[this->option_group.current_opt].increment();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Up:
        //check for music type upness
        if (this->option_group.current_opt == MUSIC_TYPE && (currMT == CTYPE || currMT == OFF)){
            if (currMT == CTYPE) this->option_groups[MUSIC_TYPE].current_opt = ATYPE;
            else this->option_groups[MUSIC_TYPE].current_opt = BTYPE;
            break;
        }
        //otherwise, change the current option group
        this->option_group.decrement();
        
        this->resetBlinkTimer();
        
        break;
    case Qt::Key_Down:
        //check for music type downness
        if (this->option_group.current_opt == MUSIC_TYPE && (currMT == ATYPE || currMT == BTYPE)){
            if (currMT == ATYPE) this->option_groups[MUSIC_TYPE].current_opt = CTYPE;
            else this->option_groups[MUSIC_TYPE].current_opt = OFF;
            break;
        }
        //otherwise, change the current option group
        this->option_group.increment();
        this->resetBlinkTimer();
        
        break;        
    }
}

void Menu1P::colorizeResources(){
    this->background = this->colorize(this->background);
}

void Menu1P::resetBlinkTimer(){
    this->blink_timer = 0;
    this->blink_on = this->prevOG == this->option_group.current_opt;
    this->prevOG = this->option_group.current_opt;
}