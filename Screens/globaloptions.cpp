#include "globaloptions.h"

GlobalOptions::GlobalOptions(QObject* parent) : NT3Screen(parent)
{
    for (uint o = 0; o < num_glob_options; ++o){
        switch(o){
        case VOLUME:
            this->option_strings[o] = "volume";
            break;
        case COLOR:
            this->option_strings[o] = "color";
            break;
        case SCALE:
            this->option_strings[o] = "scale";
            break;
        case FULLSCREEN:
            this->option_strings[o] = "fullscrn";
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", o);
            this->option_strings[o] = this->default_option_name;
            break;
        }
    }
}

GlobalOptions::~GlobalOptions(){
    if (this->options_file){
        
        this->save_options();
        
        if (this->options_file->isOpen()){
            this->options_file->close();
        }
        
        delete this->options_file;
    }
}

void GlobalOptions::init(){
    this->resetBlinkTimer();
    
    //this is done when the program starts
    //this->load_options();
    
    emit this->changeMusic(QUrl("qrc:/resources/sounds/music/musicoptions.mp3"));
}   

void GlobalOptions::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    
    if (this->blink_on){
        this->BOW_font.print(&painter, 
                             QPoint(
                                 this->option_labels_x, 
                                 this->option_labels_y_start + 
                                 static_cast<int>(this->options.current_opt)*this->option_labels_y_separation
                                 )*this->ui_scale, 
                             LEFT_ALIGN, this->option_strings[this->options.current_opt], this->ui_scale);
    }
    
    painter.drawPixmap(
                QRect(
                    QPoint(
                        this->slider_x + static_cast<int>(this->slider_length*volume*0.01), 
                        this->volume_slider_y
                        )*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    painter.drawPixmap(QRect(this->gradient_pos*this->ui_scale, this->gradient.size()*this->ui_scale), this->gradient);
    
    painter.drawPixmap(
                QRect(
                    QPoint(
                        this->slider_x + static_cast<int>(this->slider_length*hue), 
                        this->hue_slider_y
                        )*this->ui_scale, 
                    this->volume_slider.size()*this->ui_scale
                    ), 
                this->volume_slider
                );
    
    if (fullscreen){
        this->BOW_font.print(&painter, QPoint(this->fullscreen_yes_x, this->fullscreen_options_y)*this->ui_scale, 
                             LEFT_ALIGN, "yes", this->ui_scale);
    } else {
        this->BOW_font.print(&painter, QPoint(this->fullscreen_no_x, this->fullscreen_options_y)*this->ui_scale, 
                             LEFT_ALIGN, "no", this->ui_scale);
    }
}

void GlobalOptions::keyPressEvent(QKeyEvent* ev){    
    switch (ev->key()) {
    case Qt::Key_Up:
        this->options.decrement();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Down:
        this->options.increment();
        this->resetBlinkTimer();
        break;
    case Qt::Key_Left:
        switch(this->options.current_opt){
        case VOLUME:
            volume -= this->volume_increment;
            if (volume < this->volume_increment) volume = MIN_VOLUME;
            break;
        case COLOR:
            hue -= this->hue_time_factor*framerate;
            if (hue < MIN_HUE) hue = MIN_HUE;
            break;
        case SCALE:
            //TODO: we really just don't need this, but how to cleanly get it out of the UI?
            break;
        case FULLSCREEN:
            fullscreen = true;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Right:
        switch(this->options.current_opt){
        case VOLUME:
            volume += this->volume_increment;
            if (volume > MAX_VOLUME) volume = MAX_VOLUME;
            break;
        case COLOR:
            hue += this->hue_time_factor*framerate;
            if (hue > MAX_HUE) hue = MAX_HUE;
            break;
        case SCALE:
            
            break;
        case FULLSCREEN:
            fullscreen = false;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        switch(this->options.current_opt){
        case VOLUME:
            volume = DEFAULT_VOLUME;
            break;
        case COLOR:
            hue = DEFAULT_HUE;
            break;
        case SCALE:
            
            break;
        case FULLSCREEN:
            fullscreen = DEFAULT_FULLSCREEN;
            break;
        default:
            fprintf(stderr, "Unknown option selection type: %u\n", this->options.current_opt);
            break;
        }
        break;
    case Qt::Key_Escape:
        //this is done in the destructor
        //this->save_options();
        emit this->stateEnd(MAINMENU);
        break;
    }
}

void GlobalOptions::colorizeResources(){
    this->background = this->colorize(this->background);
    this->volume_slider = this->colorize(this->volume_slider);
}

void GlobalOptions::save_options(){
    
    Q_ASSERT(this->options_file);
    
    this->options_file->close();
    if (!this->options_file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)){
        fprintf(stderr, "Cannot open file at %s\n", 
                this->options_file->fileName().toUtf8().constData());
        return;
    }
    
    QString to_save = "";
    
    for (uint o = 0; o < num_glob_options; ++o){
        to_save += this->option_strings[o] + QString(this->key_val_separator);
        switch(o){
        case VOLUME:
            to_save += QString::number(volume);
            break;
        case COLOR:
            to_save += QString::number(hue);
            break;
        case SCALE:
            // pass
            break;
        case FULLSCREEN:
            to_save += fullscreen ? QString("1") : QString("0");
            break;
        default:
            fprintf(stderr, "Unknown option type: %u\n", o);
            break;
        }
        to_save += QString(this->options_separator);
    }
    
    this->options_file->write(to_save.toUtf8().constData());
    this->options_file->close();
}

void GlobalOptions::load_options(){
    this->options.reset();
    
    this->appdata_dir = QDir(this->appdata_dir_str);
    if (!this->appdata_dir.exists()){
        this->appdata_dir.mkpath(".");
    }
        
    if (this->options_file){
        if (this->options_file->isOpen()){
            this->options_file->close();
        }
        this->options_file->deleteLater();
    }
    this->options_file = new QFile(this->appdata_dir_str + QString("/") + this->options_filename);
    if (!this->options_file->open(QIODevice::ReadWrite | QIODevice::Text)){
        fprintf(stderr, "Cannot open file at %s\n%s\n", 
                this->options_file->fileName().toUtf8().constData(), this->options_file->errorString().toUtf8().constData());
        return;
    } /*else {
        printf("Opened high scores file at %s\n", this->high_scores_file->fileName().toUtf8().constData());
    }*/
    
    QTextStream stream(this->options_file);
    
    QString options_file_str = stream.readAll();
    
    QStringList option_strings = options_file_str.split(QChar(this->options_separator));
    for (QString& option_string : option_strings){
        QStringList keyvalpair = option_string.split(this->key_val_separator);
        if (keyvalpair.size() != 2) continue;
        
        QString key = keyvalpair.at(0);
        QString val = keyvalpair.at(1);
        // iterate through options and find option string that matches the key
        for (uint o = 0; o < num_glob_options; ++o){
            if (QString(this->option_strings[o]) != key) continue;
            switch(o){
            case VOLUME:
                volume = val.toInt();
                break;
            case COLOR:
                hue = val.toDouble();
                break;
            case SCALE:
                // pass
                break;
            case FULLSCREEN:
                fullscreen = val != QString("0");
                break;
            default:
                fprintf(stderr, "Unknown option type: %u\n", o);
                break;
            }
        }
    }
}
