#ifndef NT3WINDOW_H
#define NT3WINDOW_H

#include "common.h"

#include "opengl2dwindow.h"

#include "nt3screen.h"

#include "logo.h"
#include "gamea.h"
#include "credits.h"
#include "mainmenu.h"
#include "globaloptions.h"
#include "menu1p.h"

class NT3Window : public OpenGL2DWindow
{
public:
    NT3Window();
    ~NT3Window() override;
    
    void render(QPainter &painter) override;
    
    void doGameStep() override;
    
    void setupWindow();
    
    
    NT3_state_enum NT3state = LOGO;
    
    NT3Screen* screens[num_nt3_states];
    
    QPoint fullscreen_offset;
    
    double oldHue;
    int oldVolume;
    
    QMediaPlayer music_player;
    
public slots:
    void stateEnd(NT3_state_enum next);
    
    void restartMusic(QMediaPlayer::State newstate);
    
    void musicChange(QUrl new_url);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
};

#endif // NT3WINDOW_H
