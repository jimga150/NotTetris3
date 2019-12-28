#include "nt3screen.h"

NT3Screen::NT3Screen(QObject *parent) : QObject(parent)
{
    //one-time initialization
}

NT3Screen::~NT3Screen(){
    //one-time destruction at end of application's life
}

void NT3Screen::init(QScreen* screen){
    Q_UNUSED(screen)
    //init is called every time this screen becomes active, so this should also reset any dirty variables
}

void NT3Screen::resizeEvent(QResizeEvent* event){
    Q_UNUSED(event)
    //set any variables needed
    //Dont override this if you don't need to do anything not already done in lockAR()
}    

void NT3Screen::render(QPainter& painter){
    Q_UNUSED(painter)
}   

void NT3Screen::keyPressEvent(QKeyEvent* ev){
    Q_UNUSED(ev)
}   

void NT3Screen::keyReleaseEvent(QKeyEvent* ev){
    Q_UNUSED(ev)
}   

void NT3Screen::doGameStep(){
    
}   

bool NT3Screen::lockAR(QSize newSize){
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
        return false;
    }
    return true;
}