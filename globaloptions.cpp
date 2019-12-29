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
        this->BOW_font.print(&painter, QPoint(19, 18+(this->currentSelection)*16)*this->ui_scale, 
                             LEFT_ALIGN, this->option_strings[this->currentSelection], this->ui_scale);
    }
    
    painter.drawPixmap(
                QRect(
                    QPoint(71 + static_cast<int>(76*volume), 15)*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    painter.drawPixmap(QRect(QPoint(73, 33)*this->ui_scale, this->gradient.size()*this->ui_scale), this->gradient);
    
    painter.drawPixmap(
                QRect(
                    QPoint(71 + static_cast<int>(76*hue), 31)*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    if (fullscreen){
        this->BOW_font.print(&painter, QPoint(96, 66)*this->ui_scale, LEFT_ALIGN, "yes", this->ui_scale);
    } else {
        this->BOW_font.print(&painter, QPoint(133, 66)*this->ui_scale, LEFT_ALIGN, "no", this->ui_scale);
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
        //TODO: Decrement current option
        switch(this->currentSelection){
        case VOLUME:
            volume -= 0.1;
            if (volume < 0.1) volume = 0;
            break;
        case COLOR:
            hue -= 0.5*framerate;
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
        //TODO: Increment current option
        switch(this->currentSelection){
        case VOLUME:
            volume += 0.1;
            if (volume > 1) volume = 1;
            break;
        case COLOR:
            hue += 0.5*framerate;
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
        //TODO: set current option to default
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