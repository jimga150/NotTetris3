#include "globaloptions.h"

GlobalOptions::GlobalOptions(QObject* parent) : NT3Screen(parent)
{
    for (uint o = 0; o < num_glob_options; ++o){
        switch(o){
        case VOLUME:
            this->option_strings[o] = "volume";
            break;
        case COLOR:
            this->option_strings[o] = "color";
            break;
        case SCALE:
            this->option_strings[o] = "scale";
            break;
        case FULLSCREEN:
            this->option_strings[o] = "fullscrn";
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", o);
            this->option_strings[o] = this->default_option_name;
            break;
        }
    }
}

void GlobalOptions::init(){
    this->time_passed = 0;
    this->blink_on = true;
    this->currentSelection = VOLUME;
}

void GlobalOptions::calcScaleFactors(){
    
}    

void GlobalOptions::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    
    if (this->blink_on){
        this->BOW_font.print(&painter, 
                             QPoint(
                                 this->option_labels_x, 
                                 this->option_labels_y_start+(this->currentSelection)*this->option_labels_y_separation
                                 )*this->ui_scale, 
                             LEFT_ALIGN, this->option_strings[this->currentSelection], this->ui_scale);
    }
    
    painter.drawPixmap(
                QRect(
                    QPoint(
                        this->slider_x + static_cast<int>(this->slider_length*volume), 
                        this->volume_slider_y
                        )*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    painter.drawPixmap(QRect(this->gradient_pos*this->ui_scale, this->gradient.size()*this->ui_scale), this->gradient);
    
    painter.drawPixmap(
                QRect(
                    QPoint(
                        this->slider_x + static_cast<int>(this->slider_length*hue), 
                        this->hue_slider_y
                        )*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    if (fullscreen){
        this->BOW_font.print(&painter, QPoint(this->fullscreen_yes_x, this->fullscreen_options_y)*this->ui_scale, 
                             LEFT_ALIGN, "yes", this->ui_scale);
    } else {
        this->BOW_font.print(&painter, QPoint(this->fullscreen_no_x, this->fullscreen_options_y)*this->ui_scale, 
                             LEFT_ALIGN, "no", this->ui_scale);
    }
}

void GlobalOptions::keyPressEvent(QKeyEvent* ev){
    glob_option_enum last_option = static_cast<glob_option_enum>(num_glob_options - 1);
    
    switch (ev->key()) {
    case Qt::Key_Up:
        if (!this->currentSelection){
            this->currentSelection = last_option;
        } else {
            this->currentSelection = static_cast<glob_option_enum>(this->currentSelection - 1);
        }
        this->time_passed = 0;
        this->blink_on = true;
        break;
    case Qt::Key_Down:
        if (this->currentSelection == last_option){
            this->currentSelection = static_cast<glob_option_enum>(0);
        } else {
            this->currentSelection = static_cast<glob_option_enum>(this->currentSelection + 1);
        }
        this->time_passed = 0;
        this->blink_on = true;
        break;
    case Qt::Key_Left:
        switch(this->currentSelection){
        case VOLUME:
            volume -= this->volume_increment;
            if (volume < this->volume_increment) volume = 0;
            break;
        case COLOR:
            hue -= this->hue_time_factor*framerate;
            if (hue < 0) hue = 0;
            break;
        case SCALE:
            //TODO: we really just don't need this, but how to cleanly get it out of the UI?
            break;
        case FULLSCREEN:
            fullscreen = true;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->currentSelection);
            break;
        }
        break;
    case Qt::Key_Right:
        switch(this->currentSelection){
        case VOLUME:
            volume += this->volume_increment;
            if (volume > 1) volume = 1;
            break;
        case COLOR:
            hue += this->hue_time_factor*framerate;
            if (hue > 1) hue = 1;
            break;
        case SCALE:
            
            break;
        case FULLSCREEN:
            fullscreen = false;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->currentSelection);
            break;
        }
        break;
    case Qt::Key_Return:
        switch(this->currentSelection){
        case VOLUME:
            volume = DEFAULT_VOLUME;
            break;
        case COLOR:
            hue = DEFAULT_HUE;
            break;
        case SCALE:
            
            break;
        case FULLSCREEN:
            fullscreen = DEFAULT_FULLSCREEN;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->currentSelection);
            break;
        }
        break;
    case Qt::Key_Escape:
        emit this->stateEnd(MAINMENU);
        break;
    }
}

void GlobalOptions::doGameStep(){
    this->time_passed += framerate;
    if (this->time_passed > this->select_blink_rate){
        this->blink_on = !this->blink_on; //TODO: generalize blinking and the resetting of the state and timer
        this->time_passed = 0;
    }
}

void GlobalOptions::colorizeResources(){
    this->background = this->colorize(this->background);
    this->volume_slider = this->colorize(this->volume_slider);
}