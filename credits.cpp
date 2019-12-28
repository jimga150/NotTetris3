#include "credits.h"

Credits::Credits(QObject *parent) : NT3Screen(parent)
{
    this->font_height = this->BOW_font.characters.value('A').height();
}

void Credits::init(QScreen* screen){ //TODO: this and below function is basically all the same code, figure something out
    this->framerate = 1.0/screen->refreshRate();
    this->time_passed = 0;
        
    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();
    
    if (screen_width*1.0/screen_height > this->aspect_ratio){ //screen is relatively wider than the app
        
        int window_width = static_cast<int>(screen_height*this->aspect_ratio);
        emit this->setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);
        
    } else { //screen is relatively taller than app, or it's the same ratio
        
        int window_height = static_cast<int>(screen_width*1.0/this->aspect_ratio);
        emit this->setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }
}

void Credits::resizeEvent(QResizeEvent* event){
    QSize newSize = event->size();
    int width = newSize.width();
    int height = newSize.height();
    
    double ar_error = width*1.0/height - aspect_ratio;
    bool aspect_ratio_respected = qAbs(ar_error) < this->aspect_ratio_epsilon;
    
    if (ar_error > 0){ //screen is relatively wider than the app
        this->ui_scale = height*1.0/this->ui_field.height();
    } else if (ar_error < 0){ //screen is relatively skinnier than app
        this->ui_scale = width*1.0/this->ui_field.width();
    }
    
    this->ui_scale = qMax(this->min_graphics_scale, this->ui_scale);
    
    this->scaled_ui_field = TO_QRECT(this->ui_field, this->ui_scale);
    
    if (!aspect_ratio_respected){
        emit this->resize(this->scaled_ui_field.size());
    }
}    

void Credits::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->scaled_ui_field);
    
    for (int r = 0; r < this->credits_numlines; r++){
        this->BOW_font.print(&painter, QPoint(0, (r+1)*this->font_height)*this->ui_scale, LEFT_ALIGN, 
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
    this->time_passed += this->framerate;
    if (this->time_passed > this->credits_delay){
        emit this->stateEnd(MAINMENU);
    }
}   