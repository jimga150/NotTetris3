#include "opengl2dwindow.h"

OpenGL2DWindow::OpenGL2DWindow(){
    this->setSurfaceType(QWindow::OpenGLSurface);

#ifdef TIME_FRAMES
    this->frameTimer.start();
#endif
}

OpenGL2DWindow::~OpenGL2DWindow(){
#ifdef TIME_FRAMES
    quint64 numframes = this->frame_times.size();
    qint64 totalTime = 0;
    for (qint64 ft : this->frame_times){
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
#ifdef TIME_FRAMES
    long long elapsed = this->frameTimer.elapsed();
    this->frame_times.push_back(elapsed);
    const char* suffix = elapsed > this->expected_frame_time ? this->frame_toolong_suffix.data() : this->frame_normal_suffix.data();
    printf("Frame took %lld ms %s\n", elapsed, suffix);
    this->frameTimer.restart();
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
    printf("OpenGL init: %lld ms\t", timer.elapsed());
    timer.restart();
#endif

    QPainter painter(this->m_device);

    this->render(painter);

    painter.end();

#ifdef TIME_FRAME_COMPS
    printf("Render: %lld ms\t", timer.elapsed());
    timer.restart();
#endif

    this->doGameStep();

#ifdef TIME_FRAME_COMPS
    printf("Game frame: %lld ms\t", timer.elapsed());
    timer.restart();
#endif

    this->m_context->swapBuffers(this);

#ifdef TIME_FRAME_COMPS
    printf("Buffer: %lld ms\t", timer.elapsed());
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
