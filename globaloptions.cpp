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
        break;
    case Qt::Key_Right:
        //TODO: Increment current option
        break;
    case Qt::Key_Return:
        //TODO: set current option to default
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