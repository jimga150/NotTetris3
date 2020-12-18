#include "menu1p.h"

music_type_enum music_type;

Menu1P::Menu1P(QObject *parent) : NT3Screen(parent)
{
    for (uint og = 0; og < num_option_groups; ++og){
        switch (og) {
        case GAME_TYPE:
            this->option_groups[og] = optionTracker(num_game_types, NORMAL);
            for (uint option = 0; option < num_game_types; ++option){
                switch (option) {
                case NORMAL:
                    this->option_names[og].push_back("normal");
                    break;
                case STACK:
                    this->option_names[og].push_back("stack");
                    break;
                default:
                    this->option_names[og].push_back(this->default_option_name);
                    break;
                }
            }
            break;
        case MUSIC_TYPE:
            this->option_groups[og] = optionTracker(num_music_types, ATYPE);
            for (uint option = 0; option < num_music_types; ++option){
                switch (option) {
                case ATYPE:
                    this->option_names[og].push_back("a-type");
                    break;
                case BTYPE:
                    this->option_names[og].push_back("b-type");
                    break;
                case CTYPE:
                    this->option_names[og].push_back("c-type");
                    break;
                case OFF:
                    this->option_names[og].push_back(" off");
                    break;
                default:
                    this->option_names[og].push_back(this->default_option_name);
                    break;
                }
            }
            break;
        default:
            fprintf(stderr, "Undefined option group! %u\n", og);
            break;
        }
    }
}

void Menu1P::init(){
    this->resetBlinkTimer();
    this->option_group.reset();
    for (uint og = 0; og < num_option_groups; ++og){
        this->option_groups[og].reset();
    }
    emit this->changeMusic(music_urls[this->option_groups[MUSIC_TYPE].current_opt]);
    
    uint score = ((NT3Window*)(this->parent()))->gameA_score;
    
    this->appdata_dir = QDir(this->appdata_dir_str);
    if (!this->appdata_dir.exists()){
        this->appdata_dir.mkpath(".");
    }
        
    if (this->high_scores_file){
        if (this->high_scores_file->isOpen()){
            this->high_scores_file->flush();
            this->high_scores_file->close();
        }
        this->high_scores_file->deleteLater();
    }
    this->high_scores_file = new QFile(this->appdata_dir_str + QString("/") + this->high_scores_filename);
    if (!this->high_scores_file->open(QIODevice::ReadWrite | QIODevice::Text)){
        fprintf(stderr, "Cannot open file at %s\n%s\n", 
                this->high_scores_file->fileName().toUtf8().constData(), this->high_scores_file->errorString().toUtf8().constData());
        return;
    } else {
        printf("Opened high scores file at %s\n", this->high_scores_file->fileName().toUtf8().constData());
    }
    
    QTextStream stream(this->high_scores_file);
    
    QString highscores_file_str = stream.readAll();
    printf("The file had this:\n%s\n", highscores_file_str.toUtf8().constData());
    
    if (highscores_file_str.length() == 0){ // TODO: remove
        highscores_file_str = "test1,100;test2,500;jimmy,69";
    }
    // Add the current score as an unknown name
    highscores_file_str += QString(";###,") + QString::number(score);
    
    //parse into high score entries: name and score
    QStringList hs_pairs = highscores_file_str.split(QChar(this->pair_separator), Qt::SkipEmptyParts);
    
    //read into array of structs
    high_score_struct high_scores_unsorted[highscores_list_length+1];
    int ind = 0;
    for (QString s : hs_pairs){
        QStringList pair = s.split(QChar(this->name_score_separator), Qt::SkipEmptyParts);
        if (pair.length() != 2) continue;
        
        high_score_struct high_score;
        high_score.name = pair.at(0);
        high_score.score = pair.at(1).toUInt();
        
        high_scores_unsorted[ind++] = high_score;
    }
    
//    for (high_score_struct hs : high_scores_unsorted){
//        if (hs.name == "") continue;
//        printf("%s : %u\n", hs.name.toUtf8().constData(), hs.score);
//    }
    
    // insertion sort
    for(uint i = 1; i < highscores_list_length+1; ++i){
        high_score_struct key = high_scores_unsorted[i];
        int j = i - 1;
        
        while (j >= 0 && high_scores_unsorted[j].score < key.score){
            high_scores_unsorted[j + 1] = high_scores_unsorted[j];
            --j;
        }
        high_scores_unsorted[j + 1] = key;
    }
    
//    for (high_score_struct hs : high_scores_unsorted){
//        printf("%s : %u\n", hs.name.toUtf8().constData(), hs.score);
//    }
    
    // copy into global array
    for(uint i = 0; i < highscores_list_length; ++i){
        this->high_scores[i] = high_scores_unsorted[i];
    }
    
    bool spot_found = false;
    for (int i = 0; i < this->highscores_list_length; ++i){
        if (this->high_scores[i].name == "###"){
            spot_found = true;
            this->high_scores[i].name = "";
            this->high_score_entering = i;
        }
    }
    this->high_score_entry_mode = spot_found;
} 

void Menu1P::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    
    for (uint opt_group = 0; opt_group < num_option_groups; ++opt_group){
        
        if (opt_group == this->option_group.current_opt 
            && !this->blink_on && !this->high_score_entry_mode){
            continue;
        }
        
        uint opt_col = this->option_groups[opt_group].current_opt;
        
        int x, y;
        switch(opt_group){
        case GAME_TYPE:
            y = this->option_gt_y;
            x = this->option_groups[GAME_TYPE].current_opt == NORMAL ? this->option_left_x : this->option_right_x;
            break;
        case MUSIC_TYPE:
            if (this->option_groups[MUSIC_TYPE].current_opt == ATYPE || 
                this->option_groups[MUSIC_TYPE].current_opt == BTYPE){
                y = this->option_mt_top_y;
            } else {
                y = this->option_mt_bot_y;
            }
            if (this->option_groups[MUSIC_TYPE].current_opt == ATYPE || 
                this->option_groups[MUSIC_TYPE].current_opt == CTYPE){
                x = this->option_left_x;
            } else {
                x = this->option_right_x;
            }
            break;
        default:
            fprintf(stderr, "Undefined option group! %u\n", opt_group);
            y = 0;
            x = 0;
            break;
        }
        
        this->BOW_font.print(&painter, QPoint(x, y)*this->ui_scale, LEFT_ALIGN, 
                             this->option_names[opt_group][opt_col], this->ui_scale);
    }
    
    for (int i = 0; i < this->highscores_list_length; ++i){
        
        QString suffix = ""; // TODO: make blinker over 6th character when 6 chars are reached
        
        if (this->high_score_entry_mode && i == this->high_score_entering && this->blink_on && 
            this->high_scores[this->high_score_entering].name.length() < 6){
            
            suffix = "_";
        }
        
        QPoint name_start = this->top_score_name_left + QPoint(0, 8*i);
        name_start *= this->ui_scale;
        this->BOW_font.print(&painter, name_start, LEFT_ALIGN, this->high_scores[i].name + suffix, this->ui_scale);
        
        QPoint score_end = this->top_score_val_right + QPoint(0, 8*i);
        score_end *= this->ui_scale;
        this->BOW_font.print(&painter, score_end, RIGHT_ALIGN, QString::number(this->high_scores[i].score), this->ui_scale);
    }
}

void Menu1P::keyPressEvent(QKeyEvent* ev){
    
    int key = ev->key();
    QString text = ev->text();
    
    if (this->high_score_entry_mode){
        if (key == Qt::Key_Return){
            this->save_high_scores();
            this->high_score_entry_mode = false;
        } else if (text.length() > 0 && this->high_scores[this->high_score_entering].name.length() < 6){
            this->high_scores[this->high_score_entering].name += text;
        }
    } else {
        music_type_enum currMT = static_cast<music_type_enum>(this->option_groups[MUSIC_TYPE].current_opt);
        
        switch(key){
        case Qt::Key_Left:
            this->option_groups[this->option_group.current_opt].decrement();
            this->resetBlinkTimer();
            break;
        case Qt::Key_Right:
            this->option_groups[this->option_group.current_opt].increment();
            this->resetBlinkTimer();
            break;
        case Qt::Key_Up:
            //check for music type upness
            if (this->option_group.current_opt == MUSIC_TYPE && (currMT == CTYPE || currMT == OFF)){
                if (currMT == CTYPE) this->option_groups[MUSIC_TYPE].current_opt = ATYPE;
                else this->option_groups[MUSIC_TYPE].current_opt = BTYPE;
            } else {
                //otherwise, change the current option group
                this->option_group.decrement();
            }
            this->resetBlinkTimer();
            
            break;
        case Qt::Key_Down:
            //check for music type downness
            if (this->option_group.current_opt == MUSIC_TYPE && (currMT == ATYPE || currMT == BTYPE)){
                if (currMT == ATYPE) this->option_groups[MUSIC_TYPE].current_opt = CTYPE;
                else this->option_groups[MUSIC_TYPE].current_opt = OFF;
            } else {
                //otherwise, change the current option group
                this->option_group.increment();
            }
            this->resetBlinkTimer();
            
            break;
        case Qt::Key_Escape:
            emit this->stateEnd(MAINMENU);
            break;
        case Qt::Key_Return:
            music_type = static_cast<music_type_enum>(this->option_groups[MUSIC_TYPE].current_opt);
            switch (this->option_groups[GAME_TYPE].current_opt) {
            case NORMAL:
                emit this->stateEnd(GAMEA, false);
                break;
            case STACK:
                emit this->stateEnd(GAMEB, false);
                break;
            default:
                fprintf(stderr, "Undefined game type! %u\n", this->option_groups[GAME_TYPE].current_opt);
                break;
            }
            break;
        }
        
        if (this->option_groups[MUSIC_TYPE].current_opt != currMT){
            emit this->changeMusic(music_urls[this->option_groups[MUSIC_TYPE].current_opt]);
        }
    }
}

void Menu1P::colorizeResources(){
    this->background = this->colorize(this->background);
}

void Menu1P::resetBlinkTimer(){
    this->blink_timer = 0;
    this->blink_on = this->prevOG == this->option_group.current_opt;
    this->prevOG = this->option_group.current_opt;
}

void Menu1P::save_high_scores(){
    this->high_scores_file->remove();
    if (!this->high_scores_file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)){
        fprintf(stderr, "Cannot open file at %s\n", 
                this->high_scores_file->fileName().toUtf8().constData());
        return;
    }
    QString toWrite;
    for (int i = 0; i < this->highscores_list_length; ++i){
        high_score_struct hs = this->high_scores[i];
        toWrite += hs.name + QString(this->name_score_separator) + QString::number(hs.score) + QString(this->pair_separator);
    }
    this->high_scores_file->write(toWrite.toUtf8().constData());
    this->high_scores_file->flush();
    this->high_scores_file->close();
}