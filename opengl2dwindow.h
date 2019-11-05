#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>

#include <QPainter>

#if defined(TIME_FRAMES) || defined(TIME_BUFFER)
#include <QElapsedTimer>
#endif

class OpenGL2DWindow : public QOpenGLWindow, protected QOpenGLFunctions
{
public:
    OpenGL2DWindow();
    ~OpenGL2DWindow() override;

    void setAnimating(bool animating);

    virtual void render(QPainter& painter);

    virtual void doGameStep();

    int expected_frame_time = 0;

protected:
    bool event(QEvent *event) override;

    void exposeEvent(QExposeEvent *event) override;

    bool m_animating = false;

    QOpenGLContext *m_context = nullptr;
    QOpenGLPaintDevice *m_device = nullptr;

#ifdef TIME_FRAMES
    QElapsedTimer frameTimer;
    std::vector<long long> frame_times;
    std::string frame_toolong_suffix = "!!!";
    std::string frame_normal_suffix = "";
#endif

public slots:
    void renderLater();
    void renderNow();
};

#endif // OPENGLWINDOW_H
