#ifndef MENU1P_H
#define MENU1P_H

#include "nt3screen.h"
#include "nt3window.h"

enum option_group_enum {
    GAME_TYPE = 0,
    MUSIC_TYPE,
    
    num_option_groups
};

enum game_type_enum {
    NORMAL = 0,
    STACK,
    
    num_game_types
};

struct high_score_struct{
    
    QString name = "";
    uint score = 0;
    
};

class Menu1P : public NT3Screen
{
    Q_OBJECT
public:
    explicit Menu1P(QObject *parent = nullptr);
    
    void init() override;
    
    
    void render(QPainter& painter) override;
    
    
    void keyPressEvent(QKeyEvent* ev) override;
        
    void colorizeResources() override;
    
    void resetBlinkTimer() override;
    
    void save_high_scores();
    
    high_score_struct* high_score_array_for(uint game_type);
    
    
    QPixmap background = QPixmap(":/resources/graphics/gametype.png");
    
    int option_left_x = 24;
    int option_right_x = 88;
    
    int option_gt_y = 26;
    int option_mt_top_y = 60;
    int option_mt_bot_y = 76;
            
    
    optionTracker option_group = optionTracker(num_option_groups, GAME_TYPE);
    optionTracker option_groups[num_option_groups];
    
    uint prevOG = option_group.current_opt;
    
    std::vector<QString> option_names[num_option_groups];
    
    bool high_score_entry_mode;
    
    // high score stuff
    QString appdata_dir_str = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
    QDir appdata_dir;
    const QString high_scores_filename = "hs.txt";
    
    const int max_highscore_name_length = 6;
    
    const QPoint top_score_name_left = QPoint(33, 110);
    const QPoint top_score_val_right = QPoint(144, 110);
    
    static const int highscores_list_length = 3;
    
    const char list_separator = '\n';
    const char pair_separator = ';';
    const char name_score_separator = ',';
    
    QFile* high_scores_file = nullptr;
    
    high_score_struct gamea_high_scores[highscores_list_length];
    high_score_struct gameb_high_scores[highscores_list_length];
    
    game_type_enum last_game;
    
    int high_score_entering = -1;
    
    QString highscore_music_path = "qrc:/resources/sounds/music/highscoremusic.mp3";
};

#endif // MENU1P_H
