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
    }

    b2Vec2 gravity(0.0f, 4*9.8f);
    this->world = new b2World(gravity);
    this->contactlistener = new NT3ContactListener;
    this->world->SetContactListener(this->contactlistener);

    this->initializeTetrisPieceDefs();

    this->initializeWalls();

    this->thingy = this->makeTetrisPiece(T);

    this->frameTimer.start();
}

NT3Game::~NT3Game()
{
    quint64 numframes = this->frame_times.size();
    qint64 totalTime = 0;
    for (qint64 ft : this->frame_times){
        totalTime += ft;
    }
    double de_facto_rate = totalTime*1.0/numframes;
    printf("Average framerate was %f ms\n", de_facto_rate);

    if (this->world) delete world;
    //if (this->contactlistener) delete contactlistener;
    delete this->m_device;
}

void NT3Game::render()
{
    if (!this->m_device)
        this->m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    this->m_device->setSize(this->size() * this->devicePixelRatio());
    this->m_device->setDevicePixelRatio(this->devicePixelRatio());

    QPainter painter(this->m_device);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::SolidLine);
    //painter.setBrush(QColor(255*qAbs(qSin((i++)/100.0)), 255, 255));
    //painter.fillRect(100*qAbs(qSin((i*1.0)/100.0)), 200*qAbs(qSin((i)/100.0)), 30, 40, QColor(255*qAbs(qSin((i)/100.0)), 255*qAbs(qCos((i++)/50.0)), 255));
    //WORKING! Just copy the tetris code in....

    painter.drawPixmap(
                0,
                0,
                this->game_width*this->graphicsscale,
                this->game_height*this->graphicsscale,
                this->gamebackground
                );

    painter.setPen(Qt::SolidLine);
    painter.setBrush(QColor(0, 0, 0));

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        this->drawBodyTo(&painter, b);
    }

    painter.end();
}

void NT3Game::renderLater()
{
    this->requestUpdate();
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
        this->graphicsscale = height*1.0/this->game_height;
    } else { //screen is relatively taller than app, or it's the same ratio
        this->graphicsscale = width*1.0/this->game_width;
    }
    this->graphics_field.setX(this->game_field.x()*this->graphicsscale);
    this->graphics_field.setY(this->game_field.y()*this->graphicsscale);
    this->graphics_field.setWidth(this->game_field.width()*this->graphicsscale);
    this->graphics_field.setHeight(this->game_field.height()*this->graphicsscale);
}

void NT3Game::renderNow()
{
    long long elapsed = this->frameTimer.elapsed();
    this->frame_times.push_back(elapsed);
    printf("Frame  took %lld ms\n", elapsed);
    this->frameTimer.restart();
    QElapsedTimer timer;
    timer.start();

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

    this->render();

    printf("Render took %lld ms\n", timer.elapsed());
    timer.restart();

    this->m_context->swapBuffers(this);

    printf("Buffer took %lld ms\n", timer.elapsed());

    if (this->m_animating)
        world->Step(this->timeStep, this->velocityIterations, this->positionIterations);
        this->renderLater();
}

void NT3Game::setAnimating(bool animating)
{
    this->m_animating = animating;

    if (animating)
        this->renderLater();
}

void NT3Game::drawBodyTo(QPainter* painter, b2Body* body){

    painter->save();
    painter->translate(
                this->graphics_field.x() + body->GetPosition().x*this->graphicsscale,
                this->graphics_field.y() + body->GetPosition().y*this->graphicsscale
                );
    painter->rotate(body->GetAngle()*180.0/M_PI);

    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *(b2PolygonShape*)f->GetShape();
            int numpoints = shape.m_count;
            QPointF points[numpoints];
            for (int i = 0; i < numpoints; i++){
                points[i] = QPointF(shape.m_vertices[i].x*this->graphicsscale, shape.m_vertices[i].y*this->graphicsscale);
                //printf("Point: (%f, %f)\n", points[i].x(), points[i].y());
            }
            painter->drawPolygon(points, numpoints);
        }
            break;
        case b2Shape::e_circle:{
            b2CircleShape shape = *(b2CircleShape*)f->GetShape();
            QPointF center(shape.m_p.x*this->graphicsscale, shape.m_p.y*this->graphicsscale);
            painter->drawEllipse(center, shape.m_radius*this->graphicsscale, shape.m_radius*this->graphicsscale);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *(b2EdgeShape*)f->GetShape();
            painter->drawLine(
                        shape.m_vertex1.x*this->graphicsscale,
                        shape.m_vertex1.y*this->graphicsscale,
                        shape.m_vertex2.x*this->graphicsscale,
                        shape.m_vertex2.y*this->graphicsscale
                        );
        }
            break;
        case b2Shape::e_chain:{
            b2ChainShape shape = *(b2ChainShape*)f->GetShape();
            QPainterPath path;
            path.moveTo(shape.m_vertices[0].x, shape.m_vertices[0].y);
            for (int i = 1; i < shape.m_count; i++){
                path.lineTo(shape.m_vertices[i].x*this->graphicsscale, shape.m_vertices[i].y*this->graphicsscale);
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

b2Body* NT3Game::makeTetrisPiece(tetris_piece_enum type){
    b2Body* ans = world->CreateBody(&this->tetrisBodyDef);
    ans->ApplyTorque(3000000, true);
    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        ans->CreateFixture(&f);
    }
    return ans;
}

void NT3Game::initializeTetrisPieceDefs(){

    this->tetrisBodyDef.type = b2_dynamicBody;
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    this->tetrisBodyDef.position.Set(this->game_field.width()/2, 64);

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
    for (int t = 0; t < this->tetrisShapes.size(); t++){
        this->tetrisFixtures.push_back(fixture_vector_template);
        for (int s = 0; s < this->max_shapes_per_piece; s++){
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
    edge.Set(b2Vec2(0, side_length*tetris_rows), b2Vec2(side_length*tetris_cols, side_length*tetris_rows));

    this->groundBody = world->CreateBody(&edgeBodyDef);
    this->groundBody->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(0, 0), b2Vec2(0, side_length*tetris_rows));

    b2Body* leftEdge = world->CreateBody(&edgeBodyDef);
    leftEdge->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(side_length*tetris_cols, 0), b2Vec2(side_length*tetris_cols, side_length*tetris_rows));

    b2Body* rightEdge = world->CreateBody(&edgeBodyDef);
    rightEdge->CreateFixture(&edge, 0.0f);
}
