#ifndef GAMEOVER_H
#define GAMEOVER_H

#include "nt3screen.h"
#include "nt3window.h"

class GameOver : public NT3Screen
{
    Q_OBJECT
public:
    
    explicit GameOver(QObject *parent = nullptr);
    
    void init() override;
    
    void colorizeResources() override;
            
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;    
    
    const QString game_a_over_overlay_path = ":/resources/graphics/gameovergamea.png";
    QPixmap game_a_overlay = QPixmap(game_a_over_overlay_path);
    
    const QString game_b_over_overlay_path = ":/resources/graphics/gameovergameb.png";
    QPixmap game_b_overlay = QPixmap(game_b_over_overlay_path);
    
    QString gameover_sound_path = "qrc:/resources/sounds/effects/gameover2.wav";
    QSoundEffect gameover_sound;
    
    NT3_state_enum last_state;
    
};

#endif // GAMEOVER_H
