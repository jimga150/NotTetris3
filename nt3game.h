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

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>

#include <QPainter>
#include <QResizeEvent>

#include <QElapsedTimer>
#include <QtMath>
#include <QRandomGenerator>

#include "Box2D/Box2D.h"

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

class NT3Game : public QOpenGLWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit NT3Game();
    ~NT3Game() override;


    void setAnimating(bool animating);

    void render(QPainter& painter);


    void gameFrame();

    void drawBodyTo(QPainter* painter, b2Body *body);

    void initializeTetrisPieceDefs();

    void initializeWalls();

    void makeNewTetrisPiece();


    const int millis_per_second = 1000;
    const double rad_to_deg = 180.0/M_PI;

    double fps;
    double timeStep;
    double framerate; //seconds


    int32 velocityIterations = 6;
    int32 positionIterations = 2;


    b2BodyDef tetrisBodyDef;
    std::vector<std::vector<b2FixtureDef>> tetrisFixtures;
    std::vector<std::vector<b2PolygonShape>> tetrisShapes;

    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    std::vector<b2Body*> bodies;

    b2Body* groundBody = nullptr;
    b2Body* leftWall = nullptr;
    b2Body* rightWall = nullptr;

    b2Body* currentPiece = nullptr;


    const double aspect_ratio = 10.0/9.0;
    const int game_field_x_offset_px = 14;

    const int game_field_width = 82;
    const int game_width = 160;
    const int game_height = 144;

    const int tetris_rows = 18;
    const int tetris_cols = 10;

    float32 side_length = static_cast<float32>(game_height*1.0/tetris_rows);

    const QRect game_field = QRect(game_field_x_offset_px, 0, game_field_width, game_height); //TODO: add fudge width
    QRect graphics_field = game_field;

    double graphicsscale = 1;

    int scaled_game_width = game_width;
    int scaled_game_height = game_height;


    const uint32 max_shapes_per_piece = 2;

    QRandomGenerator rng = QRandomGenerator::securelySeeded();


    QString gamebackground_path = ":/resources/graphics/gamebackgroundgamea.png";
    QPixmap gamebackground = QPixmap(gamebackground_path);

    QString gameafield_path = ":/resources/graphics/gameafield.png";
    QPixmap gameafield = QPixmap(gameafield_path);

#ifdef TIME_FRAMES
    QElapsedTimer frameTimer;
    std::vector<long long> frame_times;
#endif

protected:
    bool event(QEvent *event) override;

    void exposeEvent(QExposeEvent *event) override;

    void resizeEvent(QResizeEvent* event) override;

    bool m_animating = false;

    QOpenGLContext *m_context = nullptr;
    QOpenGLPaintDevice *m_device = nullptr;

public slots:
    void renderLater();
    void renderNow();


};

