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

#include "CompositeOsgRenderer"

#include "osgQOpenGLWidget"

#include <mutex>
#include <osgViewer/View>
//#include <osgViewer/CompositeViewer>

#include <QApplication>
#include <QScreen>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <QThread>

CompositeOsgRenderer::CompositeOsgRenderer(QObject* parent)
    : QObject(parent), osgViewer::CompositeViewer(), mSimulationTime(0.0)
{
    _firstFrame = true;
}

CompositeOsgRenderer::CompositeOsgRenderer(osg::ArgumentParser* arguments, QObject* parent)
    : QObject(parent), osgViewer::CompositeViewer(*arguments), mSimulationTime(0.0)
{
    _firstFrame = true;
}

CompositeOsgRenderer::~CompositeOsgRenderer()
{
}

void CompositeOsgRenderer::update()
{
    double dt = mFrameTimer.time_s();
    mFrameTimer.setStartTick();

    emit simulationUpdated(dt);

    mSimulationTime += dt;

    double minFrameTime = _runMaxFrameRate > 0.0 ? 1.0 / _runMaxFrameRate : 0.0;
    if (dt < minFrameTime)
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(minFrameTime - dt));
    }

    osgQOpenGLWidget* osgWidget = dynamic_cast<osgQOpenGLWidget*>(parent());
    if (osgWidget)
    {
        osgWidget->_osgWantsToRenderFrame = true;
        osgWidget->update();
    }
}

void CompositeOsgRenderer::resize(int windowWidth, int windowHeight)
{
    if(!m_osgInitialized)
        return;

    for(RefViews::iterator itr = _views.begin();
        itr != _views.end();
        ++itr)
    {
        osgViewer::View* view = itr->get();
        if(view)
        {
            view->getCamera()->setViewport(new osg::Viewport(0, 0, windowWidth,
                                               windowHeight));

            m_osgWinEmb->resized(0, 0, windowWidth, windowHeight);
            m_osgWinEmb->getEventQueue()->windowResize(0, 0, windowWidth, windowHeight);
       }
   }

    update();
}

void CompositeOsgRenderer::setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb)
{
    m_osgWinEmb = osgWinEmb;
}

void CompositeOsgRenderer::setupOSG(int windowWidth, int windowHeight)
{
    m_osgInitialized = true;

    m_osgWinEmb->getEventQueue()->syncWindowRectangleWithGraphicsContext();
    // disable key event (default is Escape key) that the viewer checks on each
    // frame to see
    // if the viewer's done flag should be set to signal end of viewers main
    // loop.
    setKeyEventSetsDone(0);
    setReleaseContextAtEndOfFrameHint(false);
    setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);

    setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);

    connect( &mTimer, SIGNAL(timeout()), this, SLOT(update()) );
    mTimer.start( 10 );

    osgViewer::CompositeViewer::Windows windows;
    getWindows(windows);

    _timerId = startTimer(10, Qt::PreciseTimer);
    _lastFrameStartTime.setStartTick(0);
}

// called from ViewerWidget paintGL() method
void CompositeOsgRenderer::frame(double simulationTime)
{
    if(_done) return;

    if(_firstFrame)
    {
        viewerInit();
        realize();
        _firstFrame = false;
    }

    advance(simulationTime);

    eventTraversal();
    updateTraversal();
    renderingTraversals();
}

void CompositeOsgRenderer::timerEvent(QTimerEvent* /*event*/)
{
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
