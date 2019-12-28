#ifndef NT3WINDOW_H
#define NT3WINDOW_H

#include "common.h"

#include "opengl2dwindow.h"

#include "nt3screen.h"

#include "logo.h"
#include "nt3game.h"
#include "credits.h"

class NT3Window : public OpenGL2DWindow
{
public:
    NT3Window();
    ~NT3Window() override;
    
    void render(QPainter &painter) override;
    
    void doGameStep() override;
    
    
    NT3_state_enum NT3state = LOGO;
    
    NT3Screen* screens[num_nt3_states];
    
public slots:
    void setExpectedFrameTime(int eft);
    
    void stateEnd(NT3_state_enum next);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
};

#endif // NT3WINDOW_H