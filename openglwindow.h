#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>

#include <QPainter>

class OpenGLWindow : public QOpenGLWindow, protected QOpenGLFunctions
{
public:
    OpenGLWindow();
    ~OpenGLWindow() override;

    void setAnimating(bool animating);

    virtual void render(QPainter& painter);

    virtual void doGameStep();

protected:
    bool event(QEvent *event) override;

    void exposeEvent(QExposeEvent *event) override;

    bool m_animating = false;

    QOpenGLContext *m_context = nullptr;
    QOpenGLPaintDevice *m_device = nullptr;

#ifdef TIME_FRAMES
    QElapsedTimer frameTimer;
    std::vector<long long> frame_times;
#endif

public slots:
    void renderLater();
    void renderNow();
};

#endif // OPENGLWINDOW_H
