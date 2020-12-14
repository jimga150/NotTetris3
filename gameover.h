#ifndef GAMEOVER_H
#define GAMEOVER_H

#include "nt3screen.h"
#include "nt3window.h"

class GameOver : public NT3Screen
{
    Q_OBJECT
public:
    
    explicit GameOver(QObject *parent = nullptr);
    
    void colorizeResources() override;
            
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;    
    
    const QString gameover_overlay_path = ":/resources/graphics/gameovergamea.png";
    QPixmap gameover_overlay = QPixmap(gameover_overlay_path);
    
};

#endif // GAMEOVER_H
