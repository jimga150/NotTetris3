#ifndef MAINMENU_H
#define MAINMENU_H

#include "nt3screen.h"

//Order matters here, this should be the order from left to right as the options appear
enum mm_selection_enum {
    P = 0,
    PP,
    OPTIONS,
    
    num_mm_selections
};

class MainMenu : public NT3Screen
{
    Q_OBJECT
    
public:
    explicit MainMenu(QObject* parent = nullptr);
    
    void init() override;
    
    void calcScaleFactors() override;    
    
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
        
    void doGameStep() override;
    
    
    const QString background_path = ":/resources/graphics/title.png";
    const QPixmap background = QPixmap(background_path);
    
    QPoint selectionPoints[num_mm_selections];
    
    mm_selection_enum currentSelection = P;
};

#endif // MAINMENU_H
