#ifndef GLOBALOPTIONS_H
#define GLOBALOPTIONS_H

#include "nt3screen.h"

enum glob_option_enum{
    VOLUME = 0,
    COLOR,
    SCALE,
    FULLSCREEN,
    
    num_glob_options
};

class GlobalOptions : public NT3Screen
{
    Q_OBJECT
public:
    explicit GlobalOptions(QObject* parent = nullptr);
    
    void init() override;
    
    void calcScaleFactors() override;    
    
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
        
    void doGameStep() override;
    
    
    glob_option_enum currentSelection = VOLUME;
    
    QString option_strings[num_glob_options];
    
    QPixmap background = QPixmap(":/resources/graphics/options.png");
    
    double time_passed;
    
    bool blink_on;
};

#endif // GLOBALOPTIONS_H
