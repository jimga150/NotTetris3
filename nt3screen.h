#ifndef NT3SCREEN_H
#define NT3SCREEN_H

#include <QResizeEvent>
#include <QPainter>
#include <QScreen>

#include "common.h"

class NT3Screen : public QObject
{
    Q_OBJECT
public:
    explicit NT3Screen(QObject *parent = nullptr);
    virtual ~NT3Screen();
    
    virtual void init(QScreen* screen);
    
    virtual void resizeEvent(QResizeEvent* event);    
    
    virtual void render(QPainter& painter);
    
    
    virtual void keyPressEvent(QKeyEvent* ev);
    
    virtual void keyReleaseEvent(QKeyEvent* ev);
    
    virtual void doGameStep();
    
signals:    
    void close();
    
    void stateEnd(NT3_state_enum nextState);
    
    void setGeometry(int x, int y, int w, int h);
    
    void resize(const QSize size);    
};

#endif // NT3SCREEN_H
