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
    //maintain your aspect ratio and set any variables needed
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