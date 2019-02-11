#include "scenetoolshapebrush.hpp"

#include <QFrame>
#include <QIcon>
#include <QTableWidget>
#include <QHBoxLayout>

#include <QWidget>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QEvent>
#include <QDropEvent>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDrag>
#include <QTableWidget>
#include <QHeaderView>
#include <QApplication>
#include <QSizePolicy>

#include "scenetool.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/universalid.hpp"


CSVWidget::ShapeBrushSizeControls::ShapeBrushSizeControls(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    mBrushSizeSlider = new QSlider(Qt::Horizontal);
    mBrushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    mBrushSizeSlider->setTickInterval(10);
    mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mBrushSizeSlider->setSingleStep(1);

    mBrushSizeSpinBox = new QSpinBox;
    mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mBrushSizeSpinBox->setSingleStep(1);

    mLayoutSliderSize = new QHBoxLayout;
    mLayoutSliderSize->addWidget(mBrushSizeSlider);
    mLayoutSliderSize->addWidget(mBrushSizeSpinBox);

    connect(mBrushSizeSlider, SIGNAL(valueChanged(int)), mBrushSizeSpinBox, SLOT(setValue(int)));
    connect(mBrushSizeSpinBox, SIGNAL(valueChanged(int)), mBrushSizeSlider, SLOT(setValue(int)));

    setLayout(mLayoutSliderSize);
}

CSVWidget::ShapeBrushWindow::ShapeBrushWindow(CSMDoc::Document& document, QWidget *parent)
    : QFrame(parent, Qt::Popup),
    mBrushShape(0),
    mBrushSize(0),
    mDocument(document)
{
    mBrushShapeLabel = "Placeholder text!";

    mButtonPoint = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-point")), "", this);
    mButtonSquare = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-square")), "", this);
    mButtonCircle = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-circle")), "", this);
    mButtonCustom = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-custom")), "", this);

    mSizeSliders = new ShapeBrushSizeControls("Brush size", this);

    QVBoxLayout *layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);
    layoutMain->setContentsMargins(4,0,4,4);

    QHBoxLayout *layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setSpacing(0);
    layoutHorizontal->setContentsMargins (QMargins (0, 0, 0, 0));

    configureButtonInitialSettings(mButtonPoint);
    configureButtonInitialSettings(mButtonSquare);
    configureButtonInitialSettings(mButtonCircle);
    configureButtonInitialSettings(mButtonCustom);

    mButtonPoint->setToolTip (toolTipPoint);
    mButtonSquare->setToolTip (toolTipSquare);
    mButtonCircle->setToolTip (toolTipCircle);
    mButtonCustom->setToolTip (toolTipCustom);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(mButtonPoint);
    brushButtonGroup->addButton(mButtonSquare);
    brushButtonGroup->addButton(mButtonCircle);
    brushButtonGroup->addButton(mButtonCustom);

    brushButtonGroup->setExclusive(true);

    layoutHorizontal->addWidget(mButtonPoint, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonSquare, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonCircle, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonCustom, 0, Qt::AlignTop);

    mHorizontalGroupBox = new QGroupBox(tr(""));
    mHorizontalGroupBox->setLayout(layoutHorizontal);

    layoutMain->addWidget(mHorizontalGroupBox);
    layoutMain->addWidget(mSizeSliders);    

    setLayout(layoutMain);

    connect(mButtonPoint, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonSquare, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonCircle, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonCustom, SIGNAL(clicked()), this, SLOT(setBrushShape()));
}

void CSVWidget::ShapeBrushWindow::configureButtonInitialSettings(QPushButton *button)
{
  button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
  button->setContentsMargins (QMargins (0, 0, 0, 0));
  button->setIconSize (QSize (48-6, 48-6));
  button->setFixedSize (48, 48);
  button->setCheckable(true);
}

void CSVWidget::ShapeBrushWindow::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
    emit passBrushSize(mBrushSize);
}

void CSVWidget::ShapeBrushWindow::setBrushShape()
{
    if(mButtonPoint->isChecked()) mBrushShape = 0;
    if(mButtonSquare->isChecked()) mBrushShape = 1;
    if(mButtonCircle->isChecked()) mBrushShape = 2;
    if(mButtonCustom->isChecked()) mBrushShape = 3;
    emit passBrushShape(mBrushShape);
}

void CSVWidget::SceneToolShapeBrush::adjustToolTips()
{
}

CSVWidget::SceneToolShapeBrush::SceneToolShapeBrush (SceneToolbar *parent, const QString& toolTip, CSMDoc::Document& document)
: SceneTool (parent, Type_TopAction),
    mToolTip (toolTip),
    mDocument (document),
    mShapeBrushWindow(new ShapeBrushWindow(document, this))
{
    mBrushHistory.resize(1);
    mBrushHistory[0] = "L0#0";

    setAcceptDrops(true);
    connect(mShapeBrushWindow, SIGNAL(passBrushShape(int)), this, SLOT(setButtonIcon(int)));
    setButtonIcon(mShapeBrushWindow->mBrushShape);

    mPanel = new QFrame (this, Qt::Popup);

    QHBoxLayout *layout = new QHBoxLayout (mPanel);

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    mTable = new QTableWidget (0, 2, this);

    mTable->setShowGrid (true);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    mTable->horizontalHeader()->setSectionResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode (1, QHeaderView::Stretch);
#else
    mTable->horizontalHeader()->setResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setResizeMode (1, QHeaderView::Stretch);
#endif
    mTable->setSelectionMode (QAbstractItemView::NoSelection);

    layout->addWidget (mTable);

    connect (mTable, SIGNAL (clicked (const QModelIndex&)),
        this, SLOT (clicked (const QModelIndex&)));

}

void CSVWidget::SceneToolShapeBrush::setButtonIcon (int brushShape)
{
    QString tooltip = "Change brush settings <p>Currently selected: ";

    switch (brushShape)
    {
        case 0:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-point")));
            tooltip += mShapeBrushWindow->toolTipPoint;
            break;

        case 1:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-square")));
            tooltip += mShapeBrushWindow->toolTipSquare;
            break;

        case 2:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-circle")));
            tooltip += mShapeBrushWindow->toolTipCircle;
            break;

        case 3:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-custom")));
            tooltip += mShapeBrushWindow->toolTipCustom;
            break;
    }

    tooltip += "<p>(right click to access of previously used brush settings)";


    tooltip += "Another placeholdertext";

    setToolTip (tooltip);
}

void CSVWidget::SceneToolShapeBrush::showPanel (const QPoint& position)
{
}

void CSVWidget::SceneToolShapeBrush::updatePanel ()
{
}

void CSVWidget::SceneToolShapeBrush::clicked (const QModelIndex& index)
{
}

void CSVWidget::SceneToolShapeBrush::activate ()
{
    QPoint position = QCursor::pos();
    mShapeBrushWindow->mSizeSliders->mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mShapeBrushWindow->mSizeSliders->mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mShapeBrushWindow->move (position);
    mShapeBrushWindow->show();
}

void CSVWidget::SceneToolShapeBrush::dragEnterEvent (QDragEnterEvent *event)
{
    emit passEvent(event);
    event->accept();
}
void CSVWidget::SceneToolShapeBrush::dropEvent (QDropEvent *event)
{
    emit passEvent(event);
    event->accept();
}