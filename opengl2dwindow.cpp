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

#include "opengl2dwindow.h"

OpenGL2DWindow::OpenGL2DWindow(){
    this->setSurfaceType(QWindow::OpenGLSurface);

#ifdef TIME_FRAMES
    this->frameTimer.start();
#endif
}

OpenGL2DWindow::~OpenGL2DWindow(){
#ifdef TIME_FRAMES
    quint64 numframes = this->frame_times_vect.size();
    qint64 totalTime = 0;
    for (qint64 ft : this->frame_times_vect){
        totalTime += ft;
    }
    double de_facto_rate = totalTime*1.0/numframes;
    printf("Average framerate was %f ms\n", de_facto_rate);
#endif
    delete this->m_device;
}

void OpenGL2DWindow::setAnimating(bool animating){
    this->m_animating = animating;

    if (animating)
        this->renderLater();
}

bool OpenGL2DWindow::event(QEvent *event){
    switch (event->type()) {
    case QEvent::UpdateRequest:
        this->renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGL2DWindow::exposeEvent(QExposeEvent *event){
    Q_UNUSED(event)

    if (this->isExposed())
        this->renderNow();
}

void OpenGL2DWindow::renderLater(){
    this->requestUpdate();
}

void OpenGL2DWindow::renderNow(){

#ifdef TIME_FRAME_COMPS
    printf("OpenGL init: %lld ms\tRender: %lld ms\tGame frame: %lld ms\tBuffer: %lld ms\t",
           this->frame_times.openGL_init_time, this->frame_times.render_time,
           this->frame_times.game_frame_time, this->frame_times.buffer_time);
#endif

#ifdef TIME_FRAMES
    long long elapsed = this->frameTimer.elapsed();
    this->frame_times_vect.push_back(elapsed);
    const char* suffix = elapsed > this->expected_frame_time ? this->frame_toolong_suffix.data() : this->frame_normal_suffix.data();
    printf("Frame took %lld ms %s\n", elapsed, suffix);
    this->frameTimer.restart();
#elif defined(TIME_FRAME_COMPS)
    printf("\n");
#endif

    if (!this->isExposed())
        return;

#ifdef TIME_FRAME_COMPS
    QElapsedTimer timer;
    timer.start();
#endif

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

#ifdef TIME_FRAME_COMPS
    this->frame_times.openGL_init_time = timer.elapsed();
    timer.restart();
#endif

    QPainter painter(this->m_device);

    this->render(painter);

    painter.end();

#ifdef TIME_FRAME_COMPS
    this->frame_times.render_time = timer.elapsed();
    timer.restart();
#endif

    this->doGameStep();

#ifdef TIME_FRAME_COMPS
    this->frame_times.game_frame_time = timer.elapsed();
    timer.restart();
#endif

    this->m_context->swapBuffers(this);

#ifdef TIME_FRAME_COMPS
    this->frame_times.buffer_time = timer.elapsed();
#endif

    if (this->m_animating) this->renderLater();

}

void OpenGL2DWindow::render(QPainter& painter){
    Q_UNUSED(painter)
    //Override this!
}

void OpenGL2DWindow::doGameStep(){
    //Override this!
}
