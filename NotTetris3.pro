QT       += core gui concurrent multimedia opengl

CONFIG += c++11

macx:ICON = NotTetris3.icns
win32:RC_ICONS = NotTetris3.ico

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# build universal binary on macos
QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64

# This code compares floating point numbers with equality a lot,
# and all of them are justified, contrary to the folks over at C++ HQ.
!win32-msvc*:QMAKE_CXXFLAGS += -Wno-float-equal
!win32:QMAKE_CXXFLAGS += -Wno-weak-vtables

SOURCES += \
    box2d/src/collision/b2_broad_phase.cpp \
    box2d/src/collision/b2_chain_shape.cpp \
    box2d/src/collision/b2_circle_shape.cpp \
    box2d/src/collision/b2_collide_circle.cpp \
    box2d/src/collision/b2_collide_edge.cpp \
    box2d/src/collision/b2_collide_polygon.cpp \
    box2d/src/collision/b2_collision.cpp \
    box2d/src/collision/b2_distance.cpp \
    box2d/src/collision/b2_dynamic_tree.cpp \
    box2d/src/collision/b2_edge_shape.cpp \
    box2d/src/collision/b2_polygon_shape.cpp \
    box2d/src/collision/b2_time_of_impact.cpp \
    box2d/src/common/b2_block_allocator.cpp \
    box2d/src/common/b2_draw.cpp \
    box2d/src/common/b2_math.cpp \
    box2d/src/common/b2_settings.cpp \
    box2d/src/common/b2_stack_allocator.cpp \
    box2d/src/common/b2_timer.cpp \
    box2d/src/dynamics/b2_body.cpp \
    box2d/src/dynamics/b2_chain_circle_contact.cpp \
    box2d/src/dynamics/b2_chain_polygon_contact.cpp \
    box2d/src/dynamics/b2_circle_contact.cpp \
    box2d/src/dynamics/b2_contact.cpp \
    box2d/src/dynamics/b2_contact_manager.cpp \
    box2d/src/dynamics/b2_contact_solver.cpp \
    box2d/src/dynamics/b2_distance_joint.cpp \
    box2d/src/dynamics/b2_edge_circle_contact.cpp \
    box2d/src/dynamics/b2_edge_polygon_contact.cpp \
    box2d/src/dynamics/b2_fixture.cpp \
    box2d/src/dynamics/b2_friction_joint.cpp \
    box2d/src/dynamics/b2_gear_joint.cpp \
    box2d/src/dynamics/b2_island.cpp \
    box2d/src/dynamics/b2_joint.cpp \
    box2d/src/dynamics/b2_motor_joint.cpp \
    box2d/src/dynamics/b2_mouse_joint.cpp \
    box2d/src/dynamics/b2_polygon_circle_contact.cpp \
    box2d/src/dynamics/b2_polygon_contact.cpp \
    box2d/src/dynamics/b2_prismatic_joint.cpp \
    box2d/src/dynamics/b2_pulley_joint.cpp \
    box2d/src/dynamics/b2_revolute_joint.cpp \
    box2d/src/dynamics/b2_weld_joint.cpp \
    box2d/src/dynamics/b2_wheel_joint.cpp \
    box2d/src/dynamics/b2_world.cpp \
    box2d/src/dynamics/b2_world_callbacks.cpp \
    box2d/src/rope/b2_rope.cpp \
    Screens/credits.cpp \
    Screens/gamea.cpp \
    Screens/gameb.cpp \
    Screens/gameover.cpp \
    Screens/globaloptions.cpp \
    Screens/menu2p.cpp \
    imagefont.cpp \
    Screens/logo.cpp \
    main.cpp \
    Screens/mainmenu.cpp \
    Screens/menu1p.cpp \
    nt3contactlistener.cpp \
    nt3screen.cpp \
    nt3window.cpp \
    opengl2dwindow.cpp

HEADERS += \
    box2d/b2_api.h \
    box2d/b2_block_allocator.h \
    box2d/b2_body.h \
    box2d/b2_broad_phase.h \
    box2d/b2_chain_shape.h \
    box2d/b2_circle_shape.h \
    box2d/b2_collision.h \
    box2d/b2_common.h \
    box2d/b2_contact.h \
    box2d/b2_contact_manager.h \
    box2d/b2_distance.h \
    box2d/b2_distance_joint.h \
    box2d/b2_draw.h \
    box2d/b2_dynamic_tree.h \
    box2d/b2_edge_shape.h \
    box2d/b2_fixture.h \
    box2d/b2_friction_joint.h \
    box2d/b2_gear_joint.h \
    box2d/b2_growable_stack.h \
    box2d/b2_joint.h \
    box2d/b2_math.h \
    box2d/b2_motor_joint.h \
    box2d/b2_mouse_joint.h \
    box2d/b2_polygon_shape.h \
    box2d/b2_prismatic_joint.h \
    box2d/b2_pulley_joint.h \
    box2d/b2_revolute_joint.h \
    box2d/b2_rope.h \
    box2d/b2_settings.h \
    box2d/b2_shape.h \
    box2d/b2_stack_allocator.h \
    box2d/b2_time_of_impact.h \
    box2d/b2_time_step.h \
    box2d/b2_timer.h \
    box2d/b2_types.h \
    box2d/b2_weld_joint.h \
    box2d/b2_wheel_joint.h \
    box2d/b2_world.h \
    box2d/b2_world_callbacks.h \
    box2d/box2d.h \
    box2d/src/dynamics/b2_chain_circle_contact.h \
    box2d/src/dynamics/b2_chain_polygon_contact.h \
    box2d/src/dynamics/b2_circle_contact.h \
    box2d/src/dynamics/b2_contact_solver.h \
    box2d/src/dynamics/b2_edge_circle_contact.h \
    box2d/src/dynamics/b2_edge_polygon_contact.h \
    box2d/src/dynamics/b2_island.h \
    box2d/src/dynamics/b2_polygon_circle_contact.h \
    box2d/src/dynamics/b2_polygon_contact.h \
    Screens/menu2p.h \
    Screens/credits.h \
    Screens/gamea.h \
    Screens/gameb.h \
    Screens/gameover.h \
    Screens/globaloptions.h \
    Screens/logo.h \
    Screens/mainmenu.h \
    Screens/menu1p.h \
    common.h \
    imagefont.h \
    nt3contactlistener.h \
    nt3screen.h \
    nt3window.h \
    opengl2dwindow.h \
    tetrisgamestuff.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

DISTFILES += \
    NotTetris3.ico \
    NotTetris3.icns \
    README.md
