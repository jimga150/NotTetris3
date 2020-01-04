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
    this->resetBlinkTimer();
    this->options.reset();
}

void GlobalOptions::calcScaleFactors(){
    
}    

void GlobalOptions::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    
    if (this->blink_on){
        this->BOW_font.print(&painter, 
                             QPoint(
                                 this->option_labels_x, 
                                 this->option_labels_y_start + 
                                 static_cast<int>(this->options.current_opt)*this->option_labels_y_separation
                                 )*this->ui_scale, 
                             LEFT_ALIGN, this->option_strings[this->options.current_opt], this->ui_scale);
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
    switch (ev->key()) {
    case Qt::Key_Up:
        this->options.decrement();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Down:
        this->options.increment();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Left:
        switch(this->options.current_opt){
        case VOLUME:
            volume -= this->volume_increment;
            if (volume < this->volume_increment) volume = MIN_VOLUME;
            break;
        case COLOR:
            hue -= this->hue_time_factor*framerate;
            if (hue < MIN_HUE) hue = MIN_HUE;
            break;
        case SCALE:
            //TODO: we really just don't need this, but how to cleanly get it out of the UI?
            break;
        case FULLSCREEN:
            fullscreen = true;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Right:
        switch(this->options.current_opt){
        case VOLUME:
            volume += this->volume_increment;
            if (volume > MAX_VOLUME) volume = MAX_VOLUME;
            break;
        case COLOR:
            hue += this->hue_time_factor*framerate;
            if (hue > MAX_HUE) hue = MAX_HUE;
            break;
        case SCALE:
            
            break;
        case FULLSCREEN:
            fullscreen = false;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Return:
        switch(this->options.current_opt){
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
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Escape:
        emit this->stateEnd(MAINMENU);
        break;
    }
}

void GlobalOptions::colorizeResources(){
    this->background = this->colorize(this->background);
    this->volume_slider = this->colorize(this->volume_slider);
}