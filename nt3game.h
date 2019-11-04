/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QScreen>
#include <QResizeEvent>

#include <QRandomGenerator>

#include "Box2D/Box2D.h"

#include "opengl2dwindow.h"
#include "nt3contactlistener.h"

enum tetris_piece_enum{
    I = 0, //Long skinny piece
    O, //2x2 square
    G, //¬ piece, or a backwards L. G stands for capital Gamma.
    L,
    Z,
    S,
    T,

    num_tetris_pieces
};

enum wall_enum{
    GROUND = 0,
    LEFTWALL,
    RIGHTWALL,

    num_walls
};

enum rotate_state_enum{
    NO_ROTATION = 0,
    ROTATECCW,
    ROTATECW,
    BOTH_ROTATIONS,

    num_rotate_states
};

enum lateral_movement_state_enum{
    NO_LATERAL_MOVEMENT = 0,
    MOVELEFT,
    MOVERIGHT,
    BOTH_DIRECTIONS,

    num_lateral_movement_states
};

class NT3Game : public OpenGL2DWindow
{
public:
    explicit NT3Game();
    ~NT3Game() override;


    void render(QPainter& painter) override;


    void doGameStep() override;

    void drawBodyTo(QPainter* painter, b2Body *body);

    void drawTetrisPiece(QPainter* painter, b2Body *piece_body);

    void initializeTetrisPieceDefs();

    void initializeTetrisPieceImages();

    void initializeWalls();

    void makeNewTetrisPiece();


    //constants
    const int millis_per_second = 1000;
    const double rad_to_deg = 180.0/M_PI;


    //calculated timings
    double fps;
    double timeStep;
    double framerate; //seconds


    //input states/params
    bool accelDownState = false;
    rotate_state_enum rotateState = NO_ROTATION;
    lateral_movement_state_enum lateralMovementState = NO_LATERAL_MOVEMENT;

    int accelDownKey;
    QHash<int, rotate_state_enum> rotateStateTable;
    QHash<int, lateral_movement_state_enum> lateralMovementStateTable;


    //Box2d params
    const int32 default_velocityIterations = 10;
    const int32 default_positionIterations = 10;

    int32 velocityIterations = default_velocityIterations;
    int32 positionIterations = default_positionIterations;


    //Box2d data
    b2BodyDef tetrisBodyDef;
    std::vector<std::vector<b2FixtureDef>> tetrisFixtures;
    std::vector<std::vector<b2PolygonShape>> tetrisShapes;

    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    std::vector<b2Body*> bodies;

    b2Body* walls[num_walls];

    b2Body* currentPiece = nullptr;
    QHash<b2Body*, tetris_piece_enum> bodytypes;


    //physical properties of graphics and world
    const double aspect_ratio = 10.0/9.0;

    const int tetris_rows = 18;
    const int tetris_cols = 10;

    const QRect tetris_field = QRect(14, 0, 82, 144);
    QRect scaled_tetris_field = tetris_field;

    float32 side_length = static_cast<float32>(tetris_field.height()*1.0/tetris_rows);

    const QRect ui_field = QRect(0, tetris_field.y(), 160, tetris_field.height());
    QRect scaled_ui_field = ui_field;

    const double min_graphics_scale = 1;
    double graphicsscale = 1;

    float32 piece_start_y = -side_length/2;


    //physics constants

    const float32 old_g = 500;
    const float32 old_start_y = -64;
    const float32 old_game_height = 640;
    float32 gravity_g = old_g*(-piece_start_y+tetris_field.height())/(-old_start_y+old_game_height);

    float32 density = 1.0f/900.0f;

    float32 wmax = 3.0f;
    float32 torque = 35*side_length;

    float32 lateral_force = 4.375f*side_length;

    float32 downward_force = 2.5f*side_length;
    float32 upward_correcting_force = 8*side_length;

    float32 downward_velocity_max = 15.625f*side_length;
    float32 downward_velocity_regular = 3.125f*side_length;

    float32 piece_friction_k = 0.5f; //Box2D uses the same k for static and dynamic friction, unfortunately
    float32 ground_friction_k = 0.5f;

    float32 restitution = 0.001f;


    //piece params
    const uint32 max_shapes_per_piece = 2;

    QRandomGenerator rng = QRandomGenerator::securelySeeded();


    //resources
    QColor debug_line_color = QColor(255, 0, 0);

    QString gamebackground_path = ":/resources/graphics/gamebackgroundgamea.png";
    QPixmap gamebackground = QPixmap(gamebackground_path);

    QString gameafield_path = ":/resources/graphics/gameafield.png";
    QPixmap gameafield = QPixmap(gameafield_path);

    std::vector<QPixmap> piece_images;
    std::vector<QRect> piece_rects;

protected:
    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* ev) override;

    void keyReleaseEvent(QKeyEvent* ev) override;

};

