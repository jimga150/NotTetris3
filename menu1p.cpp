#include "menu1p.h"

Menu1P::Menu1P(QObject *parent) : NT3Screen(parent)
{
    
}

void Menu1P::init(){
    
}

void Menu1P::calcScaleFactors(){
    
}    

void Menu1P::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
}

void Menu1P::keyPressEvent(QKeyEvent* ev){
    
}
    
void Menu1P::doGameStep(){
    this->time_passed += framerate;
    if (this->time_passed > this->select_blink_rate){
        this->blink_on = !this->blink_on;
        this->time_passed = 0;
    }
}

void Menu1P::colorizeResources(){
    
}