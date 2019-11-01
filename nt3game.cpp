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

#include "nt3game.h"

NT3Game::NT3Game()
{
    this->setSurfaceType(QWindow::OpenGLSurface);

    if (this->gamebackground.isNull()){
        printf("Couldnt find image at %s!\n", this->gamebackground_path.toUtf8().constData());
        return;
    }

    b2Vec2 gravity(0.0f, 4*9.8f);
    this->world = new b2World(gravity);
    this->contactlistener = new NT3ContactListener;
    this->world->SetContactListener(this->contactlistener);

    this->initializeTetrisPieceDefs();

    this->initializeWalls();

    this->contactlistener->exceptions.push_back(this->leftWall);
    this->contactlistener->exceptions.push_back(this->rightWall);

    this->makeNewTetrisPiece();

#ifdef TIME_FRAMES
    this->frameTimer.start();
#endif
}

NT3Game::~NT3Game()
{
#ifdef TIME_FRAMES
    quint64 numframes = this->frame_times.size();
    qint64 totalTime = 0;
    for (qint64 ft : this->frame_times){
        totalTime += ft;
    }
    double de_facto_rate = totalTime*1.0/numframes;
    printf("Average framerate was %f ms\n", de_facto_rate);
#endif

    if (this->world) delete world;
    if (this->contactlistener) delete contactlistener;
    delete this->m_device;
}


bool NT3Game::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        this->renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void NT3Game::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event)

    if (this->isExposed())
        this->renderNow();
}

void NT3Game::resizeEvent(QResizeEvent* event){
    QSize newSize = event->size();
    int width = newSize.width();
    int height = newSize.height();

    if (width*1.0/height > aspect_ratio){ //screen is relatively wider than the app
        this->graphicsscale = height*1.0/this->ui_field.height();
    } else { //screen is relatively taller than app, or it's the same ratio
        this->graphicsscale = width*1.0/this->ui_field.width();
    }

    this->scaled_ui_field.setX(static_cast<int>(this->ui_field.x()*this->graphicsscale));
    this->scaled_ui_field.setY(static_cast<int>(this->ui_field.y()*this->graphicsscale));
    this->scaled_ui_field.setWidth(static_cast<int>(this->ui_field.width()*this->graphicsscale));
    this->scaled_ui_field.setHeight(static_cast<int>(this->ui_field.height()*this->graphicsscale));

    this->scaled_tetris_field.setX(static_cast<int>(this->tetris_field.x()*this->graphicsscale));
    this->scaled_tetris_field.setY(static_cast<int>(this->tetris_field.y()*this->graphicsscale));
    this->scaled_tetris_field.setWidth(static_cast<int>(this->tetris_field.width()*this->graphicsscale));
    this->scaled_tetris_field.setHeight(static_cast<int>(this->tetris_field.height()*this->graphicsscale));
}


void NT3Game::setAnimating(bool animating)
{
    this->m_animating = animating;

    if (animating)
        this->renderLater();
}

void NT3Game::renderLater()
{
    this->requestUpdate();
}

void NT3Game::renderNow()
{
#ifdef TIME_FRAMES
    long long elapsed = this->frameTimer.elapsed();
    this->frame_times.push_back(elapsed);
    printf("Frame  took %lld ms\n", elapsed);
    this->frameTimer.restart();
#endif

#ifdef TIME_BUFFER
    QElapsedTimer timer;
    timer.start();
#endif

    if (!this->isExposed())
        return;

    bool needsInitialize = false;

    if (!m_context) {
        this->m_context = new QOpenGLContext(this);
        this->m_context->setFormat(this->requestedFormat());
        this->m_context->create();

        needsInitialize = true;
    }

    this->m_context->makeCurrent(this);

    if (needsInitialize) {
        QOpenGLFunctions::initializeOpenGLFunctions();
    }

    if (!this->m_device)
        this->m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    this->m_device->setSize(this->size() * this->devicePixelRatio());
    this->m_device->setDevicePixelRatio(this->devicePixelRatio());

    QPainter painter(this->m_device);

    this->render(painter);

    painter.end();

#ifdef TIME_BUFFER
    printf("Render took %lld ms\n", timer.elapsed());
    timer.restart();
#endif
    this->m_context->swapBuffers(this);

#ifdef TIME_BUFFER
    printf("Buffer took %lld ms\n", timer.elapsed());
#endif

    if (this->m_animating){
        this->gameFrame();
        this->renderLater();
    }
}

void NT3Game::render(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, this->scaled_ui_field.width(), this->scaled_ui_field.height(), this->gamebackground);

    painter.setPen(Qt::SolidLine);
    painter.setBrush(QColor(0, 0, 0));

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        this->drawBodyTo(&painter, b);
    }
}


void NT3Game::gameFrame(){
    world->Step(static_cast<float32>(this->timeStep), this->velocityIterations, this->positionIterations);

    if (this->contactlistener->hasCurrentPieceCollided()){
        this->makeNewTetrisPiece();
    }
}

void NT3Game::drawBodyTo(QPainter* painter, b2Body* body){

    painter->save();
    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(body->GetPosition().x)*this->graphicsscale,
                this->scaled_tetris_field.y() + static_cast<double>(body->GetPosition().y)*this->graphicsscale
                );
    painter->rotate(static_cast<double>(body->GetAngle())*rad_to_deg);

    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *static_cast<b2PolygonShape*>(f->GetShape());
            int numpoints = shape.m_count;
            std::vector<QPointF> points;
            for (int i = 0; i < numpoints; i++){
                points.push_back(
                            QPointF(
                                static_cast<double>(shape.m_vertices[i].x)*this->graphicsscale,
                                static_cast<double>(shape.m_vertices[i].y)*this->graphicsscale
                                )
                            );
                //printf("Point: (%f, %f)\n", points[i].x(), points[i].y());
            }
            painter->drawPolygon(&points[0], numpoints);
        }
            break;
        case b2Shape::e_circle:{
            b2CircleShape shape = *static_cast<b2CircleShape*>(f->GetShape());
            QPointF center(
                        static_cast<double>(shape.m_p.x)*this->graphicsscale,
                        static_cast<double>(shape.m_p.y)*this->graphicsscale
                        );
            double radius = static_cast<double>(shape.m_radius);
            radius *= this->graphicsscale;
            painter->drawEllipse(center, radius, radius);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *static_cast<b2EdgeShape*>(f->GetShape());
            QPointF p1 = QPointF(
                        static_cast<double>(shape.m_vertex1.x)*this->graphicsscale,
                        static_cast<double>(shape.m_vertex1.y)*this->graphicsscale
                        );
            QPointF p2 = QPointF(
                        static_cast<double>(shape.m_vertex2.x)*this->graphicsscale,
                        static_cast<double>(shape.m_vertex2.y)*this->graphicsscale
                        );
            painter->drawLine(p1, p2);
        }
            break;
        case b2Shape::e_chain:{
            b2ChainShape shape = *static_cast<b2ChainShape*>(f->GetShape());
            QPainterPath path;
            path.moveTo(
                        static_cast<double>(shape.m_vertices[0].x),
                        static_cast<double>(shape.m_vertices[0].y)
                        );
            for (int i = 1; i < shape.m_count; i++){
                path.lineTo(
                            static_cast<double>(shape.m_vertices[i].x)*this->graphicsscale,
                            static_cast<double>(shape.m_vertices[i].y)*this->graphicsscale
                            );
            }
            painter->drawPath(path);
        }
            break;
        default:
            fprintf(stderr, "Fixture in body has undefined shape type!\n");
            return;
        }
    }
    painter->restore();
}

void NT3Game::makeNewTetrisPiece(){

    tetris_piece_enum type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));

    this->currentPiece = world->CreateBody(&this->tetrisBodyDef);
    this->currentPiece->ApplyTorque(3000000, true);

    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }
    this->contactlistener->currentPiece = this->currentPiece;
}

void NT3Game::initializeTetrisPieceDefs(){

    this->tetrisBodyDef.type = b2_dynamicBody;
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    this->tetrisBodyDef.position.Set(static_cast<float32>(this->tetris_field.width()/2), -this->side_length*2);

    for (int i = 0; i < num_tetris_pieces; i++){
        std::vector<b2PolygonShape> polyshapevect;
        this->tetrisShapes.push_back(polyshapevect);
    }

    b2PolygonShape shape_template;

    this->tetrisShapes.at(I).push_back(shape_template);
    this->tetrisShapes.at(I).at(0).SetAsBox(this->side_length*2, this->side_length/2);

    this->tetrisShapes.at(O).push_back(shape_template);
    this->tetrisShapes.at(O).at(0).SetAsBox(this->side_length, this->side_length);

    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(G).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(this->side_length, this->side_length), 0);

    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(L).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(-this->side_length, this->side_length), 0);

    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).at(0).SetAsBox(this->side_length, this->side_length/2, b2Vec2(-this->side_length/2, -this->side_length/2), 0);
    this->tetrisShapes.at(Z).at(1).SetAsBox(this->side_length, this->side_length/2, b2Vec2(this->side_length/2, this->side_length/2), 0);

    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).at(0).SetAsBox(this->side_length, this->side_length/2, b2Vec2(-this->side_length/2, this->side_length/2), 0);
    this->tetrisShapes.at(S).at(1).SetAsBox(this->side_length, this->side_length/2, b2Vec2(this->side_length/2, -this->side_length/2), 0);

    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(T).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(0, this->side_length), 0);

    b2FixtureDef fixture_template;
    std::vector<b2FixtureDef> fixture_vector_template;
    for (uint32 t = 0; t < this->tetrisShapes.size(); t++){
        this->tetrisFixtures.push_back(fixture_vector_template);
        for (uint32 s = 0; s < this->max_shapes_per_piece; s++){
            if (this->tetrisShapes.at(t).size() == s) break;
            this->tetrisFixtures.at(t).push_back(fixture_template);
            this->tetrisFixtures.at(t).at(s).shape = &this->tetrisShapes.at(t).at(s);
            this->tetrisFixtures.at(t).at(s).density = 10.0f;
            this->tetrisFixtures.at(t).at(s).friction = 0.3f;
        }
    }
}

void NT3Game::initializeWalls(){
    b2BodyDef edgeBodyDef;
    edgeBodyDef.position.Set(0, 0);

    b2EdgeShape edge;
    edge.Set(b2Vec2(0, tetris_field.height()), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->groundBody = world->CreateBody(&edgeBodyDef);
    this->groundBody->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(0, 0), b2Vec2(0, tetris_field.height()));

    this->leftWall = world->CreateBody(&edgeBodyDef);
    this->leftWall->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(tetris_field.width(), 0), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->rightWall = world->CreateBody(&edgeBodyDef);
    this->rightWall->CreateFixture(&edge, 0.0f);
}
