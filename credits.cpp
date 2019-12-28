#include "credits.h"

Credits::Credits(QObject *parent) : NT3Screen(parent)
{
    
}

void Credits::init(QScreen* screen){
    
}

void Credits::resizeEvent(QResizeEvent* event){
    
}    

void Credits::render(QPainter& painter){
    
}   

void Credits::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter){
        emit this->stateEnd(MAINMENU);
    }
}  

void Credits::doGameStep(){
    
}   