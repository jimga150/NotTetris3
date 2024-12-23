#ifndef NT3WINDOW_H
#define NT3WINDOW_H

#include "common.h"

#include "opengl2dwindow.h"

#include "nt3screen.h"

#include "Screens/logo.h"
#include "Screens/gamea.h"
#include "Screens/gameb.h"
#include "Screens/credits.h"
#include "Screens/mainmenu.h"
#include "Screens/globaloptions.h"
#include "Screens/menu1p.h"
#include "Screens/gameover.h"

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
    QAudioOutput audio_output;
    
    int expected_frame_time = 0;
    std::string frame_toolong_suffix = "!!!";
    
    int gameA_score = 0;
    int gameB_score = 0;
    
    QPixmap game_lastframe;
    
public slots:
    void stateEnd(NT3_state_enum next, bool stopMusic = true);
    
    void restartMusic(QMediaPlayer::PlaybackState newstate);
    
    void musicChange(QUrl new_url);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
};

#endif // NT3WINDOW_H
