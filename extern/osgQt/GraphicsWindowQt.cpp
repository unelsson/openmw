/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "GraphicsWindowQt"

#include <osg/DeleteHandler>
#include <osgViewer/ViewerBase>
#include <QInputEvent>
#include <QPointer>

#include <QWindow>

using namespace osgQt;

#if (QT_VERSION < QT_VERSION_CHECK(5, 2, 0))
    #define GETDEVICEPIXELRATIO() 1.0
#else
    #define GETDEVICEPIXELRATIO() devicePixelRatio()
#endif

GraphicsWindowQt::GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWidget* parent, const QOpenGLWidget* shareWidget, Qt::WindowFlags f )
:   _realized(false)
{

    _widget = NULL;
    _traits = traits;
    init( parent, shareWidget, f );
}

GraphicsWindowQt::GraphicsWindowQt( QOpenGLWidget* widget )
:   _realized(false)
{
    _widget = widget;
    _traits = _widget ? createTraits( _widget ) : new osg::GraphicsContext::Traits;
    init( NULL, NULL, 0 );
}

GraphicsWindowQt::~GraphicsWindowQt()
{
    close();

    // remove reference from QOpenGLWidget
    /*if ( _widget )
        _widget->_gw = NULL;*/
}

bool GraphicsWindowQt::init( QWidget* parent, const QOpenGLWidget* shareWidget, Qt::WindowFlags f )
{
    // update _widget and parent by WindowData
    WindowData* windowData = _traits.get() ? dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : 0;
    if ( !_widget )
        _widget = windowData ? windowData->_widget : NULL;
    if ( !parent )
        parent = windowData ? windowData->_parent : NULL;

    // create widget if it does not exist
    _ownsWidget = _widget == NULL;
    if ( !_widget )
    {
        // shareWidget
        if ( !shareWidget ) {
            GraphicsWindowQt* sharedContextQt = dynamic_cast<GraphicsWindowQt*>(_traits->sharedContext.get());
            if ( sharedContextQt )
                shareWidget = sharedContextQt->getQOpenGLWidget();
        }

        // WindowFlags
        Qt::WindowFlags flags = f | Qt::Window | Qt::CustomizeWindowHint;
        if ( _traits->windowDecoration )
            flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint;

        // create widget
        _widget = new QOpenGLWidget(parent, flags);
    }

    // set widget name and position
    // (do not set it when we inherited the widget)
    if ( _ownsWidget )
    {
        _widget->setWindowTitle( _traits->windowName.c_str() );
        _widget->move( _traits->x, _traits->y );
        if ( !_traits->supportsResize ) _widget->setFixedSize( _traits->width, _traits->height );
        else _widget->resize( _traits->width, _traits->height );
    }

    // initialize widget properties
    //_widget->setAutoBufferSwap( false );
    _widget->setMouseTracking( true );
    //_widget->setGraphicsWindow( this );
    useCursor( _traits->useCursor );

    // initialize State
    setState( new osg::State );
    getState()->setGraphicsContext(this);

    // initialize contextID
    if ( _traits.valid() && _traits->sharedContext.valid() )
    {
        getState()->setContextID( _traits->sharedContext->getState()->getContextID() );
        incrementContextIDUsageCount( getState()->getContextID() );
    }
    else
    {
        getState()->setContextID( osg::GraphicsContext::createNewContextID() );
    }

    // make sure the event queue has the correct window rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    return true;
}

QSurfaceFormat GraphicsWindowQt::traits2QSurfaceFormat( const osg::GraphicsContext::Traits* traits )
{
    QSurfaceFormat format( QSurfaceFormat::defaultFormat() );

    format.setAlphaBufferSize( traits->alpha );
    format.setRedBufferSize( traits->red );
    format.setGreenBufferSize( traits->green );
    format.setBlueBufferSize( traits->blue );
    format.setDepthBufferSize( traits->depth );
    format.setStencilBufferSize( traits->stencil );
    //format.setSampleBuffers( traits->sampleBuffers );
    format.setSamples( traits->samples );

    //format.setAlpha( traits->alpha>0 );
    //format.setDepth( traits->depth>0 );
    //format.setStencil( traits->stencil>0 );
    //format.setDoubleBuffer( traits->doubleBuffer );
    format.setSwapInterval( traits->vsync ? 1 : 0 );
    format.setStereo( traits->quadBufferStereo ? 1 : 0 );

    return format;
}

void GraphicsWindowQt::QSurfaceFormat2traits( const QSurfaceFormat& format, osg::GraphicsContext::Traits* traits )
{
    traits->red = format.redBufferSize();
    traits->green = format.greenBufferSize();
    traits->blue = format.blueBufferSize();
    traits->alpha = format.hasAlpha() ? format.alphaBufferSize() : 0;
    //traits->depth = format.depthBufferSize() : 0;
    //traits->stencil = format.stencilBufferSize() : 0;

    //not found in qt5.0, what was it for?
    //traits->sampleBuffers = format.sampleBuffers() ? 1 : 0;

    traits->samples = format.samples();

    traits->quadBufferStereo = format.stereo();

    //maybe related to swapBehavior() SurfaceFormat::SwapBehavior
    //traits->doubleBuffer = format.doubleBuffer();

    traits->vsync = format.swapInterval() >= 1;
}

osg::GraphicsContext::Traits* GraphicsWindowQt::createTraits( const QOpenGLWidget* widget )
{
    osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits;

    QSurfaceFormat2traits( widget->format(), traits );

    QRect r = widget->geometry();
    traits->x = r.x();
    traits->y = r.y();
    traits->width = r.width();
    traits->height = r.height();

    traits->windowName = widget->windowTitle().toLocal8Bit().data();
    Qt::WindowFlags f = widget->windowFlags();
    traits->windowDecoration = ( f & Qt::WindowTitleHint ) &&
                            ( f & Qt::WindowMinMaxButtonsHint ) &&
                            ( f & Qt::WindowSystemMenuHint );
    QSizePolicy sp = widget->sizePolicy();
    traits->supportsResize = sp.horizontalPolicy() != QSizePolicy::Fixed ||
                            sp.verticalPolicy() != QSizePolicy::Fixed;

    return traits;
}

/*bool GraphicsWindowQt::setWindowRectangleImplementation( int x, int y, int width, int height )
{
    if ( _widget == NULL )
        return false;

    _widget->setGeometry( x, y, width, height );
    return true;
}*/

/*void GraphicsWindowQt::getWindowRectangle( int& x, int& y, int& width, int& height )
{
    if ( _widget )
    {
        const QRect& geom = _widget->geometry();
        x = geom.x();
        y = geom.y();
        width = geom.width();
        height = geom.height();
    }
}*/

/*bool GraphicsWindowQt::setWindowDecorationImplementation( bool windowDecoration )
{
    Qt::WindowFlags flags = Qt::Window|Qt::CustomizeWindowHint;//|Qt::WindowStaysOnTopHint;
    if ( windowDecoration )
        flags |= Qt::WindowTitleHint|Qt::WindowMinMaxButtonsHint|Qt::WindowSystemMenuHint;
    _traits->windowDecoration = windowDecoration;

    if ( _widget )
    {
        _widget->setWindowFlags( flags );

        return true;
    }

    return false;
}*/

/*bool GraphicsWindowQt::getWindowDecoration() const
{
    return _traits->windowDecoration;
}*/

/*void GraphicsWindowQt::grabFocus()
{
    if ( _widget )
        _widget->setFocus( Qt::ActiveWindowFocusReason );
}*/

/*void GraphicsWindowQt::grabFocusIfPointerInWindow()
{
    if ( _widget->underMouse() )
        _widget->setFocus( Qt::ActiveWindowFocusReason );
}*/

/*void GraphicsWindowQt::raiseWindow()
{
    if ( _widget )
        _widget->raise();
}*/

/*void GraphicsWindowQt::setWindowName( const std::string& name )
{
    if ( _widget )
        _widget->setWindowTitle( name.c_str() );
}*/

/*std::string GraphicsWindowQt::getWindowName()
{
    return _widget ? _widget->windowTitle().toStdString() : "";
}*/

/*void GraphicsWindowQt::useCursor( bool cursorOn )
{
    if ( _widget )
    {
        _traits->useCursor = cursorOn;
        if ( !cursorOn ) _widget->setCursor( Qt::BlankCursor );
        else _widget->setCursor( _currentCursor );
    }
}*/

/*void GraphicsWindowQt::setCursor( MouseCursor cursor )
{
    if ( cursor==InheritCursor && _widget )
    {
        _widget->unsetCursor();
    }

    switch ( cursor )
    {
    case NoCursor: _currentCursor = Qt::BlankCursor; break;
    case RightArrowCursor: case LeftArrowCursor: _currentCursor = Qt::ArrowCursor; break;
    case InfoCursor: _currentCursor = Qt::SizeAllCursor; break;
    case DestroyCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case HelpCursor: _currentCursor = Qt::WhatsThisCursor; break;
    case CycleCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case SprayCursor: _currentCursor = Qt::SizeAllCursor; break;
    case WaitCursor: _currentCursor = Qt::WaitCursor; break;
    case TextCursor: _currentCursor = Qt::IBeamCursor; break;
    case CrosshairCursor: _currentCursor = Qt::CrossCursor; break;
    case HandCursor: _currentCursor = Qt::OpenHandCursor; break;
    case UpDownCursor: _currentCursor = Qt::SizeVerCursor; break;
    case LeftRightCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopSideCursor: case BottomSideCursor: _currentCursor = Qt::UpArrowCursor; break;
    case LeftSideCursor: case RightSideCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    default: break;
    };
    if ( _widget ) _widget->setCursor( _currentCursor );
}*/

/*bool GraphicsWindowQt::valid() const
{
    return _widget && _widget->isValid();
}*/

/*bool GraphicsWindowQt::realizeImplementation()
{
    // save the current context
    // note: this will save only Qt-based contexts
    const QGLContext *savedContext = QGLContext::currentContext();

    // initialize GL context for the widget
    if ( !valid() )
        _widget->glInit();

    // make current
    _realized = true;
    bool result = makeCurrent();
    _realized = false;

    // fail if we do not have current context
    if ( !result )
    {
        if ( savedContext )
            const_cast< QGLContext* >( savedContext )->makeCurrent();

        OSG_WARN << "Window realize: Can make context current." << std::endl;
        return false;
    }

    _realized = true;

    // make sure the event queue has the correct window rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    // make this window's context not current
    // note: this must be done as we will probably make the context current from another thread
    //       and it is not allowed to have one context current in two threads
    if( !releaseContext() )
        OSG_WARN << "Window realize: Can not release context." << std::endl;

    // restore previous context
    if ( savedContext )
        const_cast< QGLContext* >( savedContext )->makeCurrent();

    return true;
}*/

/*bool GraphicsWindowQt::isRealizedImplementation() const
{
    return _realized;
}*/

/*void GraphicsWindowQt::closeImplementation()
{
    if ( _widget )
        _widget->close();
    _realized = false;
}*/

/*void GraphicsWindowQt::runOperations()
{
    // While in graphics thread this is last chance to do something useful before
    // graphics thread will execute its operations.
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();

    if (QGLContext::currentContext() != _widget->context())
        _widget->makeCurrent();

    GraphicsWindow::runOperations();
}

bool GraphicsWindowQt::makeCurrentImplementation()
{
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();

    _widget->makeCurrent();

    return true;
}

bool GraphicsWindowQt::releaseContextImplementation()
{
    _widget->doneCurrent();
    return true;
}*/

class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:

    QtWindowingSystem()
    {
        OSG_INFO << "QtWindowingSystemInterface()" << std::endl;
    }

    ~QtWindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
    }

    // Access the Qt windowing system through this singleton class.
    static QtWindowingSystem* getInterface()
    {
        static QtWindowingSystem* qtInterface = new QtWindowingSystem;
        return qtInterface;
    }

    // Return the number of screens present in the system
    virtual unsigned int getNumScreens( const osg::GraphicsContext::ScreenIdentifier& /*si*/ )
    {
        OSG_WARN << "osgQt: getNumScreens() not implemented yet." << std::endl;
        return 0;
    }

    // Return the resolution of specified screen
    // (0,0) is returned if screen is unknown
    virtual void getScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/ )
    {
        OSG_WARN << "osgQt: getScreenSettings() not implemented yet." << std::endl;
    }

    // Set the resolution for given screen
    virtual bool setScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/ )
    {
        OSG_WARN << "osgQt: setScreenSettings() not implemented yet." << std::endl;
        return false;
    }

    // Enumerates available resolutions
    virtual void enumerateScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/ )
    {
        OSG_WARN << "osgQt: enumerateScreenSettings() not implemented yet." << std::endl;
    }

    // Create a graphics context with given traits
    virtual osg::GraphicsContext* createGraphicsContext( osg::GraphicsContext::Traits* traits )
    {
        if (traits->pbuffer)
        {
            OSG_WARN << "osgQt: createGraphicsContext - pbuffer not implemented yet." << std::endl;
            return NULL;
        }
        else
        {
            osg::ref_ptr< GraphicsWindowQt > window = new GraphicsWindowQt( traits );
            if (window->valid()) return window.release();
            else return NULL;
        }
    }

private:

    // No implementation for these
    QtWindowingSystem( const QtWindowingSystem& );
    QtWindowingSystem& operator=( const QtWindowingSystem& );
};
