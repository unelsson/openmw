// To-do: Getting Texture Id and Texture Filenames to base class Terraintexturemode
// To-do: Better data handling options for mBrushTexture
// To-do: loading texture bitmaps from virtual file system (vfs) for texture overlay icon

#include "terraintexturemode.hpp"
#include "editmode.hpp"

#include <iostream>
#include <string>

//Some includes are not needed (have to clean this up later)
#include <QWidget>
#include <QIcon>
#include <QPainter>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QEvent>
#include <QDropEvent>
#include <QColor>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDrag>
#include <QAbstractItemModel>
#include <QModelIndex>

#include "../../model/world/commands.hpp"

#include "../widget/modebutton.hpp"
#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetoolbar.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/land.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/resourcetable.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/data.hpp"

#include <osg/ref_ptr>
#include <osg/Image>

#include <components/resource/imagemanager.hpp>
#include <extern/osgQt/GraphicsWindowQt>
#include <components/fallback/fallback.hpp>
#include <components/misc/stringops.hpp>

#include "terrainstorage.hpp"

#include "pagedworldspacewidget.hpp"


CSVRender::BrushSizeControls::BrushSizeControls(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    brushSizeSlider = new QSlider(Qt::Horizontal);
    brushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    brushSizeSlider->setTickInterval(10);
    brushSizeSlider->setSingleStep(1);

    brushSizeSpinBox = new QSpinBox;
    brushSizeSpinBox->setRange(1, 100);
    brushSizeSpinBox->setSingleStep(1);

    layoutSliderSize = new QHBoxLayout;
    layoutSliderSize->addWidget(brushSizeSlider);
    layoutSliderSize->addWidget(brushSizeSpinBox);

    connect(brushSizeSlider, SIGNAL(valueChanged(int)), brushSizeSpinBox, SLOT(setValue(int)));
    connect(brushSizeSpinBox, SIGNAL(valueChanged(int)), brushSizeSlider, SLOT(setValue(int)));

    setLayout(layoutSliderSize);
}

CSVRender::TextureBrushButton::TextureBrushButton (const QIcon & icon, const QString & text, QWidget * parent)
    : QPushButton(icon, text, parent)
{
}

void CSVRender::TextureBrushButton::dragEnterEvent (QDragEnterEvent *event)
{
  event->accept();
}

void CSVRender::TextureBrushButton::dropEvent (QDropEvent *event)
{
  const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

  if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
      return;

  if (mime->holdsType (CSMWorld::UniversalId::Type_LandTexture))
  {
      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          std::string mBrushTexture(uid.getId());
          emit passBrushTexture(mBrushTexture);
      }
  }
}

CSVRender::TextureBrushWindow::TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent)
    : QWidget(parent), mWorldspaceWidget (worldspaceWidget)
{
    mBrushTextureLabel = "Brush: " + mBrushTexture;
    selectedBrush = new QLabel(QString::fromUtf8(mBrushTextureLabel.c_str()), this);

    pointIcon = drawIconTexture(QPixmap (iconPointImage.c_str()));
    squareIcon = drawIconTexture(QPixmap (iconSquareImage.c_str()));
    circleIcon = drawIconTexture(QPixmap (iconCircleImage.c_str()));
    customIcon = drawIconTexture(QPixmap (iconCustomImage.c_str()));

    buttonPoint->setIcon(drawIconTexture(QPixmap (iconPointImage.c_str())));
    buttonSquare->setIcon(drawIconTexture(QPixmap (iconSquareImage.c_str())));
    buttonCircle->setIcon(drawIconTexture(QPixmap (iconCircleImage.c_str())));
    buttonCustom->setIcon(drawIconTexture(QPixmap (iconCustomImage.c_str())));

    QVBoxLayout *layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);

    QHBoxLayout *layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setContentsMargins (QMargins (0, 0, 0, 0));
    layoutHorizontal->setSpacing(0);

    configureButtonInitialSettings(buttonPoint);
    configureButtonInitialSettings(buttonSquare);
    configureButtonInitialSettings(buttonCircle);
    configureButtonInitialSettings(buttonCustom);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(buttonPoint);
    brushButtonGroup->addButton(buttonSquare);
    brushButtonGroup->addButton(buttonCircle);
    brushButtonGroup->addButton(buttonCustom);

    brushButtonGroup->setExclusive(true);

    layoutHorizontal->addWidget(buttonPoint);
    layoutHorizontal->addWidget(buttonSquare);
    layoutHorizontal->addWidget(buttonCircle);
    layoutHorizontal->addWidget(buttonCustom);

    horizontalGroupBox = new QGroupBox(tr(""));
    horizontalGroupBox->setLayout(layoutHorizontal);

    BrushSizeControls* sizeSliders = new BrushSizeControls(tr(""), this);

    layoutMain->addWidget(horizontalGroupBox);
    layoutMain->addWidget(sizeSliders);
    layoutMain->addWidget(selectedBrush);

    setLayout(layoutMain);

    connect(buttonPoint, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(buttonSquare, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(buttonCircle, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(buttonCustom, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));

    setWindowFlags(Qt::Window);
}

void CSVRender::TextureBrushWindow::configureButtonInitialSettings(TextureBrushButton *button)
{
  button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
  button->setContentsMargins (QMargins (0, 0, 0, 0));
  button->setIconSize (QSize (48-6, 48-6));
  button->setFixedSize (48, 48);
  button->setAcceptDrops(true);
  button->setCheckable(true);
}

void CSVRender::TextureBrushWindow::getBrushTexture(std::string brushTexture)
{
    mBrushTexture = brushTexture;
    mBrushTextureLabel = "Brush:" + mBrushTexture;
    selectedBrush->setText(QString::fromUtf8(mBrushTextureLabel.c_str()));
    buttonPoint->setIcon(drawIconTexture(QPixmap (iconPointImage.c_str())));
    buttonSquare->setIcon(drawIconTexture(QPixmap (iconSquareImage.c_str())));
    buttonCircle->setIcon(drawIconTexture(QPixmap (iconCircleImage.c_str())));
    buttonCustom->setIcon(drawIconTexture(QPixmap (iconCustomImage.c_str())));
    //return CSVRender::TerrainTextureMode::drawIconTexture();
}

// This function should eventually load brush texture as bitmap and set it as overlay
// Currently only holds very basic code for icon drawing and landtexture loading
QIcon CSVRender::TextureBrushWindow::drawIconTexture(QPixmap pixmapBrush)
{
    //Get Image Texture
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    CSMWorld::Data& mData = document.getData();
    CSMWorld::IdTable& ltexs = dynamic_cast<CSMWorld::IdTable&> (
*document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    std::string hash = "#";
    std::string space = " ";
    std::size_t hashlocation = mBrushTexture.find(hash);
    std::string idRow = mBrushTexture.substr (hashlocation+1);
    std::cout << "mBrushTexture" << mBrushTexture << std::endl;
    std::cout << "idRow" << idRow << std::endl;

    QModelIndex index = ltexs.index(stoi(idRow), 6);
    QString text = ltexs.data(index).toString();
    std::string textureFilenameTga = text.toUtf8().constData();
    std::cout << "0,6:" << textureFilenameTga << std::endl;

    std::string textureName = "textures/" + textureFilenameTga.substr(0, textureFilenameTga.size()-3)+"dds";
    std::cout << "textureName: " << textureName << std::endl;
    for(std::size_t i=0; i<textureName.size();++i)
        textureName[i] = std::tolower(textureName[i]);
    std::cout << "textureName(toLower): " << textureName << std::endl;
    //std::string textureName = "textures/tx_sand_01.dds";
    Resource::ImageManager* imageManager = mData.getResourceSystem()->getImageManager();
    osg::ref_ptr<osg::Image> brushTextureOsg = new osg::Image();
    brushTextureOsg = imageManager->getNewImage(textureName);

    //Decompress to pixel format GL_RGB8
    if (brushTextureOsg->getPixelFormat() == 33776)
    {

        osg::ref_ptr<osg::Image> newImage = new osg::Image;
        newImage->setFileName(brushTextureOsg->getFileName());
        newImage->allocateImage(brushTextureOsg->s(), brushTextureOsg->t(), brushTextureOsg->r(), GL_RGB, GL_UNSIGNED_BYTE);
        for (int s=0; s<brushTextureOsg->s(); ++s)
            for (int t=0; t<brushTextureOsg->t(); ++t)
                for (int r=0; r<brushTextureOsg->r(); ++r)
                    newImage->setColor(brushTextureOsg->getColor(s,t,r), s,t,r);
        brushTextureOsg = newImage;
        brushTextureOsg->setInternalTextureFormat(GL_RGB8);
    }

    const uchar *qImageBuffer = (const uchar*)brushTextureOsg->data();

    QImage img(qImageBuffer, brushTextureOsg->s(), brushTextureOsg->t(), QImage::Format_RGB888);
    QPixmap pixmapObject(QPixmap::fromImage(img));

    QPainter painter(&pixmapBrush);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    QBrush brush;
    brush.setTexture(pixmapObject);
    painter.setBrush(brush); //QBrush::setTexture(const QPixmap &pixmap)
    painter.drawRect(pixmapBrush.rect());

    QIcon brushIcon(pixmapBrush);
    return brushIcon;
}

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent)
, textureBrushWindow(new TextureBrushWindow(worldspaceWidget, this))
{

}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::activate(toolbar);

}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primaryEditPressed(const WorldspaceHitResult& hit) // Apply changes here
{
  std::cout << "PRIMARYPRESSED" << std::endl;
  std::cout << hit.index0 << std::endl;
  std::cout << hit.index1 << std::endl;
  std::cout << hit.index2 << std::endl;
  std::cout << hit.hit << std::endl;
  std::string cellId = getWorldspaceWidget().getCellId (hit.worldPos);
  std::cout << cellId << std::endl;
  std::string hash = "#";
  std::string space = " ";
  std::size_t hashlocation = cellId.find(hash);
  std::size_t spacelocation = cellId.find(space);
  std::string splicedX = cellId.substr (hashlocation+1, spacelocation-hashlocation);
  std::string splicedY = cellId.substr (spacelocation+1);
  std::cout << splicedX << std::endl;
  std::cout << splicedY << std::endl;
  CSMWorld::CellCoordinates mCoordinates(stoi(splicedX), stoi(splicedY));
  //mCoordinates.my = cellId;

  /*CSMWorld::ModifyLandTexturesCommand::ModifyLandTexturesCommand(IdTable& landTable,
      IdTable& ltexTable, QUndoCommand* parent)*/

  /*CSMWorld::DeleteCommand* command = new CSMWorld::DeleteCommand(referencesTable,
      static_cast<ObjectTag*>(iter->get())->mObject->getReferenceId());

  getWorldspaceWidget().getDocument().getUndoStack().push(command);*/
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
      textureBrushWindow->setWindowTitle("Texture brush settings");
      textureBrushWindow->show();
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
}

bool CSVRender::TerrainTextureMode::primaryEditStartDrag (const QPoint& pos) //Collect one command to macro here
{
    std::cout << "Textureeditdragstart" << std::endl;
    return false;
}

bool CSVRender::TerrainTextureMode::secondaryEditStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::primarySelectStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::secondarySelectStartDrag (const QPoint& pos)
{
    return false;
}

void CSVRender::TerrainTextureMode::drag (const QPoint& pos, int diffX, int diffY, double speedFactor) {
}

void CSVRender::TerrainTextureMode::dragCompleted(const QPoint& pos) {
}

void CSVRender::TerrainTextureMode::dragAborted() {
}

void CSVRender::TerrainTextureMode::dragWheel (int diff, double speedFactor) {}

void CSVRender::TerrainTextureMode::dragEnterEvent (QDragEnterEvent *event) {
}

void CSVRender::TerrainTextureMode::dropEvent (QDropEvent *event) {
}

void CSVRender::TerrainTextureMode::dragMoveEvent (QDragMoveEvent *event) {
}

void CSVRender::TerrainTextureMode::modifyLandMacro (CSMWorld::CommandMacro& commands)
{
  /*CSMDoc::Document& document = getWorldspaceWidget().getDocument();
  CSMWorld::Data& mData = document.getData();

  // Setup land if available
  const CSMWorld::IdCollection<CSMWorld::Land>& land = mData.getLand();
  int landIndex = land.searchId(mId);
  if (landIndex != -1 && !land.getRecord(mId).isDeleted())
  {
      const ESM::Land& esmLand = land.getRecord(mId).get();

      if (esmLand.getLandData (ESM::Land::DATA_VHGT))
      {
          if (mTerrain)
          {
              mTerrain->unloadCell(mCoordinates.getX(), mCoordinates.getY());
              mTerrain->clearAssociatedCaches();
          }
              else
          {
              mTerrain.reset(new Terrain::TerrainGrid(mCellNode, mCellNode,
                  mData.getResourceSystem().get(), new TerrainStorage(mData), Mask_Terrain));
          }

          mTerrain->loadCell(esmLand.mX, esmLand.mY);

          return;
      }
  }*/
}
