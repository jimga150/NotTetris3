#ifndef GAMEA_H
#define GAMEA_H

#include <cmath>

#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <QSoundEffect>
#include <QPainterPath>

#include "box2d/box2d.h"

#include "tetrisgamestuff.h"
#include "nt3screen.h"
#include "nt3contactlistener.h"

//#define TIME_GAME_FRAME 1
//#define TIME_RENDER_STEPS 1

#define NUM_FRAMES_TO_SAVE 120

enum gamea_state_enum{
    gameA = 0,
    start_row_clear_blinking,
    row_clear_blinking,
    shake_field_state,
    flush_blocks,
    
    num_gamea_states
};

enum ray_cast_enum{
    TOPLEFT = 0,
    TOPRIGHT,
    BOTTOMLEFT,
    BOTTOMRIGHT,
    
    num_ray_casts
};

enum line_cut_side_enum{
    BOTTOM = 0,
    TOP,
    
    num_line_cut_sides
};

struct row_sides_struct{
    float top_m = 0;
    float bottom_m = 0;
    
    row_sides_struct(){}
    
    row_sides_struct(uint row, float side_length_m){
        this->top_m = (row+1)*side_length_m + std::numeric_limits<float>::epsilon();
        this->bottom_m = row*side_length_m - std::numeric_limits<float>::epsilon();
    }
};

struct rayCastComplete{
    b2RayCastInput input;
    b2RayCastOutput output;
    bool hit;
    
    void doRayCast(b2Shape* s, b2Body* b){
        b2Transform t;
        t.Set(b->GetPosition(), b->GetAngle());
        this->hit = s->RayCast(&this->output, this->input, t, 0);
    }
};

class GameA : public NT3Screen
{    
    Q_OBJECT
    
public:

    //constructor/destructor
    explicit GameA(QObject* parent = nullptr);
    ~GameA() override;
    
    
    //initialization
    void init() override;
    
    
    //Destructor utilities
    void destroyWorld();
    
    void freeUserDataOn(b2Body* b);
    
    
    //graphics
    void calcScaleFactors() override;    
    
    void render(QPainter& painter) override;
    
    void colorizeResources() override;
    
    void drawBodyTo(QPainter* painter, b2Body *body);
    
    void drawTetrisPiece(QPainter* painter, b2Body *piece_body);
    
    void drawScore(QPainter* painter);
    
    
    //game logic    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
    
    void doGameStep() override;

    void pieceLanded();

    void processControlInput();
    
    
    //calculating/removing rows
    void clearRows();
    
    float getRowArea_m2(uint row, bool use_current_piece);

    void clearDiagRange(float top_y_m, float bottom_y_m, float angle_rad);
    
    void clearYRange(float top_y_m, float bottom_y_m);
    
    QImage maskImage(b2Body* b, QImage orig_image, QRectF region_m);
    
    bool TestPointRadius(b2PolygonShape* s, const b2Transform& xf, const b2Vec2& p) const;
    
    std::vector<rayCastComplete> getRayCasts(float top_m, float bot_m, float slope);
    
    b2Vec2 hit_point(rayCastComplete ray_cast);
    
    float poly_area_m2(b2Vec2* vertices, int count);
    
    
    //adding new pieces
    void makeNewTetrisPiece(bool is_powerup);
    
    void makeNewNextPiece();
    
    
    //utilities
    bool isAWall(b2Body* b);
    
    QString b2Vec2String(b2Vec2 vec);
    
    tetrisPieceData getTetrisPieceData(b2Body* b);
    
    void setTetrisPieceData(b2Body* b, tetrisPieceData tpd);

    bool hasTetrisPieceData(b2Body *b);
    
    QPixmap enableAlphaChannel(QPixmap pixmap);
        
    void destroyTetrisPiece(b2Body* b);

    QPointF physPtToScrnPt(b2Vec2 worldPoint_m);

    QRectF physRectToScrnRect(b2Vec2 topLeft_m, b2Vec2 size_m);

    b2Vec2 scrnPtToPhysPt(QPointF screenPoint_px);

    
    //initialization functions
    void initializeTetrisPieceDefs();
    
    void initializeTetrisPieceImages();
    
    void initializeWalls();
    
    void init_BDC();
    //end functions
    
    
    
    //debug
    const bool debug_framerate = false;
    
    const bool debug_box2d = false;
    
    // not const cause of the way its used but trust me, dont change it willy nilly. I'll know.
    bool frame_review = false;
    
    
    //calculated timings    
    float timeStep_s;
        
    
    //input states/params
    bool accelDownState;
    rotate_state_enum rotateState;
    lateral_movement_state_enum lateralMovementState;
    
    int freeze_key;
    int accelDownKey;
    QHash<int, rotate_state_enum> rotateStateTable;
    QHash<int, lateral_movement_state_enum> lateralMovementStateTable;
    
    
    //Box2d params
    const int32 default_velocityIterations = 8;
    const int32 default_positionIterations = 3;
    
    int32 velocityIterations = default_velocityIterations;
    int32 positionIterations = default_positionIterations;
    
    
    //Box2d data
    b2BodyDef tetrisBodyDef;
    std::vector<std::vector<b2FixtureDef>> tetrisFixtures;
    std::vector<std::vector<b2PolygonShape>> tetrisShapes;
    std::vector<b2Vec2> center_of_mass_offsets;
    
    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    
    QHash<b2Body*, tetrisPieceData> userData;
    
    b2Body* walls[num_walls];
    
    b2Body* currentPiece = nullptr;
    
    std::vector<b2Body*> bodies_to_destroy;
    
    //Index: Row box # (up to tetris_rows-1)
    std::vector<float> row_areas_m2;
    std::vector<QHash<b2Body*, float>> body_area_contributions_m2;
    
    
    //physical properties of graphics and world
    
    const uint tetris_rows = 18;
    const uint tetris_cols = 10;
    
    const uint row_fill_density_col_width_in = 6;
    
    //must be an even number greater than 1 and less than 7.6175362031343
    //so basically 2, 4, or 6 meters is it right now
    float side_length_m = 2.0f;
    double side_length_dbl_m = static_cast<double>(side_length_m);
    
    const QRectF tetris_field_m = QRectF(
                                    1.75*side_length_dbl_m,
                                    0, 
                                    (tetris_cols + 0.25)*side_length_dbl_m,
                                    tetris_rows*side_length_dbl_m
                                    );
    QRectF tetris_field_px = SCALE_QRECTF(tetris_field_m, 1);
    
    const float square_area_m2 = side_length_m*side_length_m;
    
    const float min_poly_area_m2 = (1.0f/40.0f)*square_area_m2;
    
    //TODO: come up with a better way of signifying that a unit is x/y other than "x_y"
    const double physics_to_ui_scale_in_m = ui_field_in.height()/tetris_field_m.height();
    double physics_to_screen_scale_px_m = physics_to_ui_scale_in_m*ui_to_screen_scale_px_in;

    const float raycast_left_m = -this->side_length_m;
    const float raycast_right_m = static_cast<float>(this->tetris_field_m.width()) + this->side_length_m;
        
    //scale used to make tetris piece cutting smooth (set later)
    double piece_image_scale = 0;
        
    b2Vec2 piece_start_m = b2Vec2(static_cast<float>(this->tetris_field_m.width()/2), -this->side_length_m*2);
    
    QPoint score_display_right_in = QPoint(151, 24);
    
    QRect score_add_display_in = QRect(QPoint(109, 35), QSize(46, 10));
    
    QPoint sc_add_right_in_disp_in = QPoint(42, 1);
    
    int score_add_disp_offset_in = 0;
    
    QPoint lines_cleared_disp_offset_in = QPoint(144, 80);
    
    QPoint level_disp_offset_in = QPoint(144, 56);
    
    
    //physics constants
    
    const float old_g_m_s2 = 500;
    const float old_start_y_m = -64;
    const float old_game_height_m = 640;
    const float gravity_g_m_s2 = old_g_m_s2*(static_cast<float>(tetris_field_m.height()) - piece_start_m.y)/(old_game_height_m - old_start_y_m);
    
    //TODO: unit? unit necessary?
    const float density = 1;//1.0f/900.0f;
    
    const float wmax_rad_s = 3.0f;
    const float angular_accel_rad_s2 = 9.55342974715892f;
    
    const float lateral_accel_m_s2 = 11.25f*side_length_m;
    
    const float downward_accel_m_s2 = 14.441875f*side_length_m;
    
    const float upward_correcting_accel_m_s2 = 50.87875f*side_length_m;
    
    const float downward_velocity_max_m_s = 15.753125f*side_length_m;
    const float downward_velocity_regular_m_s = 2.86853125f*side_length_m;
    const float downward_velocity_level_increment_m_s = 0.2007971875f*side_length_m;
    
    const float piece_friction_k = 0.5f; //Box2D uses the same k for static and dynamic friction, unfortunately
    const float ground_friction_k = 0.5f;
    
    const float restitution = 0.01f;
    
    const float linear_damping_1_s = 0.5f;
    const float angular_damping_1_s = 0.1f;
    
    const float line_clear_threshold = 8.1f*square_area_m2;
    
    
    //next piece stuff
    float next_piece_w_rad_s = 1.0;
    
    tetris_piece_enum next_piece_type = default_tetris_piece;
    
    b2BodyDef next_piece_bodydef;
    
    b2Body* next_piece_for_display = nullptr;
    
    QPoint next_piece_display_center_in = QPoint(136, 120);
    
    
    //Game state
    gamea_state_enum game_state;
    
    bool paused;
    QPixmap pause_frame;

    bool skip_falling;
    
    const float avgarea_divisor = square_area_m2*10;
    
    int current_score;
    
    int score_to_add;
    
    int lines_cleared;
    
    int current_level;
    
    bool new_level_reached;
    
    const int lines_per_level = 10;
    
    
    //piece params
    const uint32 max_shapes_per_piece = 4;
    
    QRandomGenerator rng;
    
    
    //resources
    QColor debug_line_color = QColor(255, 0, 0);
    
    QString gamebackground_path = ":/resources/graphics/gamebackgroundgamea.png";
    QPixmap gamebackground = QPixmap(gamebackground_path);
    
    QString pause_overlay_path = ":/resources/graphics/pausegamea.png";
    QPixmap pause_overlay = QPixmap(pause_overlay_path);
    
    std::vector<QPixmap> piece_images;
    std::vector<QPixmap> pwu_piece_images;
    std::vector<QRectF> piece_rects_m;
    
    QPixmap default_piece_image;
    QRectF default_piece_rect_m;
    tetrisPieceData default_data;
    
    QSoundEffect sfx[num_sound_effects];
    
    
    //Line clear stuff
    QColor line_clear_color = QColor(0xd2af8f);
    
    const double lc_blink_toggle_time_s = 0.17142857142857;
    
    const uint num_blinks = 4; //number of blinks to do on row clear
    
    uint num_blinks_so_far; //blink count accumulator
    
    std::vector<bool> rows_to_clear;

    bool clear_diag_cut;
    float diag_top_m;
    float diag_bot_m;
    float diag_slope;

    bool shake_field;
    double shake_time_acc_s;
    const int shake_time_max_s = 3;
    
    double row_blink_accumulator_s;
    
    bool row_blink_on; //blink state
    
    QFuture<void> line_clearing_thread;
    
    QPixmap line_clear_freezeframe;
    
    
    //freeze frame data
    bool freeze_frame = false;
    
    QPixmap saved_frames[NUM_FRAMES_TO_SAVE];
    
    int last_frame = 0;
};

#endif
