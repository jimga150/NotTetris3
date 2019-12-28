#include "credits.h"

Credits::Credits(QObject *parent) : NT3Screen(parent)
{
    
}

void Credits::init(){
    this->time_passed = 0;
}

void Credits::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->scaled_ui_field);
    
    for (int r = 0; r < this->credits_numlines; r++){
        this->BOW_font.print(&painter, QPoint(0, (r+1)*this->BOW_font.height_px)*this->ui_scale, LEFT_ALIGN, 
                             this->credits_text[r], this->ui_scale);
    }
    painter.drawPixmap(QRect(QPoint(32, 80)*this->ui_scale, this->logo.size()*this->ui_scale), this->logo);
}   

void Credits::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter){
        emit this->stateEnd(MAINMENU);
    }
}  

void Credits::doGameStep(){
    this->time_passed += framerate;
    if (this->time_passed > this->credits_delay){
        emit this->stateEnd(MAINMENU);
    }
}   