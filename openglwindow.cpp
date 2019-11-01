#include "openglwindow.h"

OpenGLWindow::OpenGLWindow(){
    this->setSurfaceType(QWindow::OpenGLSurface);

#ifdef TIME_FRAMES
    this->frameTimer.start();
#endif
}

OpenGLWindow::~OpenGLWindow(){
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

void OpenGLWindow::setAnimating(bool animating){
    this->m_animating = animating;

    if (animating)
        this->renderLater();
}

bool OpenGLWindow::event(QEvent *event){
    switch (event->type()) {
    case QEvent::UpdateRequest:
        this->renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event){
    Q_UNUSED(event)

    if (this->isExposed())
        this->renderNow();
}

void OpenGLWindow::renderLater(){
    this->requestUpdate();
}

void OpenGLWindow::renderNow(){
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
        this->doGameStep(); //TODO
        this->renderLater();
    }
}

void OpenGLWindow::render(QPainter& painter){
    Q_UNUSED(painter)
}

void OpenGLWindow::doGameStep(){

}
