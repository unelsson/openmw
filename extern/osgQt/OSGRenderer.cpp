// Copyright (C) 2017 Mike Krus
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "OSGRenderer"

#include "osgQOpenGLWidget"

//#include <osgQOpenGL/CullVisitorEx>
//#include <osgQOpenGL/GraphicsWindowEx>
#include <QApplication>
#include <QScreen>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <QThread>

OSGRenderer::OSGRenderer(QObject* parent)
    : QObject(parent), osgViewer::Viewer()
{
    //    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
    //                     [this]()
    //    {
    //        _applicationAboutToQuit = true;
    //        killTimer(_timerId);
    //        _timerId = 0;
    //    });
}

OSGRenderer::OSGRenderer(osg::ArgumentParser* arguments, QObject* parent)
    : QObject(parent), osgViewer::Viewer(*arguments)
{
    //    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
    //                     [this]()
    //    {
    //        _applicationAboutToQuit = true;
    //        killTimer(_timerId);
    //        _timerId = 0;
    //    });
}

OSGRenderer::~OSGRenderer()
{
}

void OSGRenderer::update()
{
    osgQOpenGLWidget* osgWidget = dynamic_cast<osgQOpenGLWidget*>(parent());
    osgWidget->_osgWantsToRenderFrame = true;
    osgWidget->update();
}

void OSGRenderer::resize(int windowWidth, int windowHeight)
{
    if(!m_osgInitialized)
        return;

    _camera->setViewport(new osg::Viewport(0, 0, windowWidth,
                                           windowHeight));

    m_osgWinEmb->resized(0, 0,
                         windowWidth,
                         windowHeight);
    m_osgWinEmb->getEventQueue()->windowResize(0, 0,
                                               windowWidth,
                                               windowHeight);

    update();
}


void OSGRenderer::setupOSG(int windowWidth, int windowHeight)
{
    m_osgInitialized = true;
    m_osgWinEmb = new osgViewer::GraphicsWindowEmbedded(0, 0, windowWidth, windowHeight);

    // make sure the event queue has the correct window rectangle size and input range
    m_osgWinEmb->getEventQueue()->syncWindowRectangleWithGraphicsContext();
    _camera->setViewport(new osg::Viewport(0, 0, windowWidth, windowHeight));
    _camera->setGraphicsContext(m_osgWinEmb.get());
    // disable key event (default is Escape key) that the viewer checks on each
    // frame to see
    // if the viewer's done flag should be set to signal end of viewers main
    // loop.
    setKeyEventSetsDone(0);
    setReleaseContextAtEndOfFrameHint(false);
    setThreadingModel(osgViewer::Viewer::SingleThreaded);

    osgViewer::Viewer::Windows windows;
    getWindows(windows);

    _timerId = startTimer(10, Qt::PreciseTimer);
    _lastFrameStartTime.setStartTick(0);
}

void OSGRenderer::setKeyboardModifiers(QInputEvent* event)
{
    unsigned int modkey = event->modifiers() & (Qt::ShiftModifier |
                                                Qt::ControlModifier |
                                                Qt::AltModifier);
    unsigned int mask = 0;

    if(modkey & Qt::ShiftModifier) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;

    if(modkey & Qt::ControlModifier) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;

    if(modkey & Qt::AltModifier) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    m_osgWinEmb->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
}

bool OSGRenderer::checkEvents()
{
    // check events from any attached sources
    for(Devices::iterator eitr = _eventSources.begin();
        eitr != _eventSources.end();
        ++eitr)
    {
        osgGA::Device* es = eitr->get();

        if(es->getCapabilities() & osgGA::Device::RECEIVE_EVENTS)
        {
            if(es->checkEvents())
                return true;
        }

    }

    // get events from all windows attached to Viewer.
    Windows windows;
    getWindows(windows);

    for(Windows::iterator witr = windows.begin();
        witr != windows.end();
        ++witr)
    {
        if((*witr)->checkEvents())
            return true;
    }

    return false;
}

bool OSGRenderer::checkNeedToDoFrame()
{
    // check if any event handler has prompted a redraw
    if(_requestRedraw)
        return true;

    if(_requestContinousUpdate)
        return true;

    // check if the view needs to update the scene graph
    // this check if camera has update callback and if scene requires to update scene graph
    if(requiresUpdateSceneGraph())
        return true;

    // check if the database pager needs to update the scene
    if(getDatabasePager()->requiresUpdateSceneGraph())
        return true;

    // check if the image pager needs to update the scene
    if(getImagePager()->requiresUpdateSceneGraph())
        return true;


    // check if the scene needs to be redrawn
    if(requiresRedraw())
        return true;

    // check if events are available and need processing
    if(checkEvents())
        return true;

    // and check again if any event handler has prompted a redraw
    if(_requestRedraw)
        return true;

    if(_requestContinousUpdate)
        return true;

    return false;
}

// called from ViewerWidget paintGL() method
void OSGRenderer::frame(double simulationTime)
{
    // limit the frame rate
    if(getRunMaxFrameRate() > 0.0)
    {
        double dt = _lastFrameStartTime.time_s();
        double minFrameTime = 1.0 / getRunMaxFrameRate();

        if(dt < minFrameTime)
            QThread::usleep(static_cast<unsigned int>(1000000.0 * (minFrameTime - dt)));
    }

    // avoid excessive CPU loading when no frame is required in ON_DEMAND mode
    if(getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND)
    {
        double dt = _lastFrameStartTime.time_s();

        if(dt < 0.01)
            OpenThreads::Thread::microSleep(static_cast<unsigned int>(1000000.0 *
                                                                      (0.01 - dt)));
    }

    // record start frame time
    _lastFrameStartTime.setStartTick();
    // make frame

#if 1
    osgViewer::Viewer::frame(simulationTime);
#else

    if(_done) return;

    // OSG_NOTICE<<std::endl<<"CompositeViewer::frame()"<<std::endl<<std::endl;

    if(_firstFrame)
    {
        viewerInit();

        if(!isRealized())
        {
            realize();
        }

        _firstFrame = false;
    }

    advance(simulationTime);

    eventTraversal();
    updateTraversal();
    //    renderingTraversals();
#endif
}

void OSGRenderer::requestRedraw()
{
    osgViewer::Viewer::requestRedraw();
}

void OSGRenderer::timerEvent(QTimerEvent* /*event*/)
{
    // application is about to quit, just return
    if(_applicationAboutToQuit)
    {
        return;
    }

    // ask ViewerWidget to update 3D view
    if(getRunFrameScheme() != osgViewer::ViewerBase::ON_DEMAND ||
       checkNeedToDoFrame())
    {
        update();
    }
}
