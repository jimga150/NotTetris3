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
    
    void colorizeResources() override;     
    
    void keyPressEvent(QKeyEvent* ev) override;    
    
    
    const double volume_increment = 0.1;
    
    const double hue_time_factor = 0.5; //don't ask me why this is.
    
    const int option_labels_x = 19;
    const int option_labels_y_start = 18;
    const int option_labels_y_separation = 16;
    
    const int slider_x = 71;
    const int slider_length = 76;
    
    const int volume_slider_y = 15;
    const int hue_slider_y = 31;
    
    const int fullscreen_options_y = 66;
    const int fullscreen_yes_x = 96;
    const int fullscreen_no_x = 133;
    
    const QPoint gradient_pos = QPoint(73, 33);
    
    optionTracker options = optionTracker(num_glob_options, VOLUME);
    
    QString option_strings[num_glob_options];
    
    QPixmap background = QPixmap(":/resources/graphics/options.png");
    QPixmap volume_slider = QPixmap(":/resources/graphics/volumeslider.png");
    QPixmap gradient = QPixmap(":/resources/graphics/rainbow.png");
};

#endif // GLOBALOPTIONS_H
