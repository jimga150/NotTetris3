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

#include <QGuiApplication>
#include <QScreen>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QSurfaceFormat format;
    format.setSamples(16);

    NT3Game window;
    window.setFormat(format);

    QScreen* screen = QGuiApplication::primaryScreen();
    window.fps = screen->refreshRate();
    window.framerate = 1.0/window.fps;
    window.timeStep = window.framerate; //seconds
    //printf("Using time step of %f ms\n", window.timeStep*window.millis_per_second);

    QRect screenRect = screen->availableGeometry();
    //TODO: use availableGeometryChanged() signal to resize application if needed
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();

    if (screen_width*1.0/screen_height > window.aspect_ratio){ //screen is relatively wider than the app
        //this->setFixedSize(h*aspect_ratio, h);
        int window_width = static_cast<int>(screen_height*window.aspect_ratio);
        window.setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);
    } else { //screen is relatively taller than app, or it's the same ratio
        int window_height = static_cast<int>(screen_width*1.0/window.aspect_ratio);
        window.setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }

    window.show();

    window.setAnimating(true);

    return app.exec();
}
