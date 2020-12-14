#include "objectprocgentool.hpp"

#include <random>
#include <fstream>

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QUndoStack>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../doc/document.hpp"
#include "../world/idtable.hpp"
#include "columnbase.hpp"
#include "commandmacro.hpp"
#include "commands.hpp"
#include "cellcoordinates.hpp"

const int cellSize {ESM::Land::REAL_SIZE};
const int landSize {ESM::Land::LAND_SIZE};
const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

CSMWorld::ObjectProcGenTool::ObjectProcGenTool(CSMDoc::Document& document, QWidget* parent) : QWidget(parent, Qt::Window)
    , mDocument(document)
{
    createInterface();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(mSpinBoxGroup);
    setLayout(layout);

    connect(mActionButton, SIGNAL(clicked()), this, SLOT(placeObjectsNow()));
    connect(mNewGenerationObjectButton, SIGNAL(clicked()), this, SLOT(createNewGenerationObject()));
    connect(mDeleteGenerationObjectButton, SIGNAL(clicked()), this, SLOT(deleteGenerationObject()));
    connect(mLoadGenerationSettingsButton, SIGNAL(clicked()), this, SLOT(loadGenerationSettings()));
    connect(mSaveGenerationSettingsButton, SIGNAL(clicked()), this, SLOT(saveGenerationSettings()));
}

CSMWorld::ObjectProcGenTool::~ObjectProcGenTool()
{
}

void CSMWorld::ObjectProcGenTool::createInterface()
{
    mSpinBoxGroup = new QGroupBox(tr("Procedural generation (objects)"));

    mCornerALabel = new QLabel(tr("Cell corner A:"));

    mCellXSpinBoxCornerA = new QSpinBox;
    mCellXSpinBoxCornerA->setRange(-99999999, 99999999);
    mCellXSpinBoxCornerA->setValue(0);

    mCellYSpinBoxCornerA = new QSpinBox;
    mCellYSpinBoxCornerA->setRange(-99999999, 99999999);
    mCellYSpinBoxCornerA->setValue(0);

    mCornerBLabel = new QLabel(tr("Cell corner B:"));

    mCellXSpinBoxCornerB = new QSpinBox;
    mCellXSpinBoxCornerB->setRange(-99999999, 99999999);
    mCellXSpinBoxCornerB->setValue(0);

    mCellYSpinBoxCornerB = new QSpinBox;
    mCellYSpinBoxCornerB->setRange(-99999999, 99999999);
    mCellYSpinBoxCornerB->setValue(0);

    mDeleteGenerationObjectButton = new QPushButton("-", this);
    mNewGenerationObjectButton = new QPushButton("+", this);
    mDeleteGenerationObjectButton->setStyleSheet("border: 1px solid #000000; border-radius:8px;");
    mNewGenerationObjectButton->setStyleSheet("border: 1px solid #000000; border-radius:8px;");
    QHBoxLayout *deleteNewButtonsLayout = new QHBoxLayout;
    deleteNewButtonsLayout->addWidget(mDeleteGenerationObjectButton);
    deleteNewButtonsLayout->addWidget(mNewGenerationObjectButton);

    mLoadGenerationSettingsButton = new QPushButton("Load...", this);
    mSaveGenerationSettingsButton = new QPushButton("Save...", this);

    mActionButton = new QPushButton("Generate!", this);

    mMainLayout = new QVBoxLayout;
    mCellCoordinatesQHBoxLayout = new QHBoxLayout;
    mCellCoordinatesQVBoxLayoutA = new QVBoxLayout;
    mCellCoordinatesQVBoxLayoutB = new QVBoxLayout;

    mGeneratedObjectsLayout = new QVBoxLayout;
    mGeneratedObjectsLayout->setSizeConstraint(QLayout::SetFixedSize);
    mGeneratedObjectsLayout->setSpacing(0);
    mGeneratedObjectsLayout->setContentsMargins(0, 0, 0, 0);

    mCellCoordinatesQVBoxLayoutA->addWidget(mCornerALabel);
    mCellCoordinatesQVBoxLayoutA->addWidget(mCellXSpinBoxCornerA);
    mCellCoordinatesQVBoxLayoutA->addWidget(mCellYSpinBoxCornerA);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCornerBLabel);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCellXSpinBoxCornerB);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCellYSpinBoxCornerB);
    mCellCoordinatesQHBoxLayout->addLayout(mCellCoordinatesQVBoxLayoutA);
    mCellCoordinatesQHBoxLayout->addLayout(mCellCoordinatesQVBoxLayoutB);

    QGroupBox* generatedObjectsGroupBox; //layout holding all generation objects
    generatedObjectsGroupBox = new QGroupBox;
    generatedObjectsGroupBox->setObjectName("generatedObjectsGroupBox");
    generatedObjectsGroupBox->setStyleSheet("QGroupBox#generatedObjectsGroupBox {margin: 0px;padding: 0px; border: 0px;}");
    generatedObjectsGroupBox->setContentsMargins(0, 0, 0, 0);
    generatedObjectsGroupBox->setLayout(mGeneratedObjectsLayout);

    QScrollArea* scrollArea;
    scrollArea = new QScrollArea;
    scrollArea->setWidget(generatedObjectsGroupBox);

    createNewGenerationObject();

    mMainLayout->addLayout(mCellCoordinatesQHBoxLayout);
    mMainLayout->addWidget(scrollArea);
    mMainLayout->addLayout(deleteNewButtonsLayout);
    mMainLayout->addWidget(mLoadGenerationSettingsButton);
    mMainLayout->addWidget(mSaveGenerationSettingsButton);
    mMainLayout->addWidget(mActionButton);
    mSpinBoxGroup->setLayout(mMainLayout);
}

void CSMWorld::ObjectProcGenTool::createNewGenerationObject()
{
    QGroupBox* generatedObjectGroupBox; //layout holding single generation object
    QHBoxLayout* generatedObjectGroupBoxLayout; //layout holding single generation object
    generatedObjectGroupBox = new QGroupBox;
    generatedObjectGroupBoxLayout = new QHBoxLayout;

    generatedObjectGroupBoxLayout->setSpacing(0);
    generatedObjectGroupBoxLayout->setContentsMargins(0, 0, 0, 0);

    generatedObjectGroupBox->setObjectName("generatedObjectGroupBox");
    generatedObjectGroupBox->setStyleSheet("QGroupBox#generatedObjectGroupBox {padding: 0px; margin: 0px;border: 0px;}");
    generatedObjectGroupBox->adjustSize();

    mGeneratedObjects.emplace_back(new QComboBox(this));
    CSMWorld::IdTable& referenceablesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));
    for (int j = 0; j < referenceablesTable.rowCount(); ++j)
    {
        mGeneratedObjects.back()->addItem(QString::fromStdString(referenceablesTable.getId(j)));
    }

    QLabel* chanceLabel;
    chanceLabel = new QLabel(tr("%"));
    mGeneratedObjectChanceSpinBoxes.emplace_back(new QDoubleSpinBox);
    mGeneratedObjectChanceSpinBoxes.back()->setRange(0.f, 1.f);
    mGeneratedObjectChanceSpinBoxes.back()->setSingleStep(0.1f);
    mGeneratedObjectChanceSpinBoxes.back()->setValue(0.7f);
    mGeneratedObjectChanceSpinBoxes.back()->setDecimals(5);

    QLabel* ltexLabel;
    ltexLabel = new QLabel(tr("Tex:"));
    mGeneratedObjectTerrainTexType.emplace_back(new QComboBox);
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));
    for (int j = 0; j < ltexTable.rowCount(); ++j)
    {
        mGeneratedObjectTerrainTexType.back()->addItem(QString::fromStdString(ltexTable.getId(j)));
    }

    QLabel* followLandShapeLabel;
    followLandShapeLabel = new QLabel(tr("Rotate to land shape factor:"));
    mFollowLandShapeFactor.emplace_back(new QDoubleSpinBox);
    mFollowLandShapeFactor.back()->setDecimals(2);
    mFollowLandShapeFactor.back()->setRange(-3.00f, 3.00f); //1.0f makes object point to normal direction
    mFollowLandShapeFactor.back()->setValue(0.25f);

    mRandomZRotationCheckBox.emplace_back(new QCheckBox("Random z-rotation", this));
    mRandomZRotationCheckBox.back()->setChecked(true);

    QLabel* randomRotationLabel;
    randomRotationLabel = new QLabel(tr("Random xy-rotation"));
    mRandomRotation.emplace_back(new QDoubleSpinBox);
    mRandomRotation.back()->setDecimals(2);
    mRandomRotation.back()->setRange(0.00f, 3.12f); //radians
    mRandomRotation.back()->setValue(0.08f);

    QLabel* randomDisplacementLabel;
    randomDisplacementLabel = new QLabel(tr("Random displacement:"));
    mRandomDisplacement.emplace_back(new QSpinBox);
    mRandomDisplacement.back()->setRange(0, 99999999); //in worldspace units
    mRandomDisplacement.back()->setValue(500);

    QLabel* zDisplacementLabel;
    zDisplacementLabel = new QLabel(tr("Z displacement:"));
    mZDisplacement.emplace_back(new QSpinBox);
    mZDisplacement.back()->setRange(-99999999, 99999999); //in worldspace units
    mZDisplacement.back()->setValue(0);

    QLabel* minZHeightLabel;
    minZHeightLabel = new QLabel(tr("Min Z:"));
    mMinZHeight.emplace_back(new QSpinBox);
    mMinZHeight.back()->setRange(-99999999, 99999999);
    mMinZHeight.back()->setValue(-100000);

    QLabel* maxZHeightLabel;
    maxZHeightLabel = new QLabel(tr("Max Z:"));
    mMaxZHeight.emplace_back(new QSpinBox);
    mMaxZHeight.back()->setRange(-99999999, 99999999);
    mMaxZHeight.back()->setValue(100000);

    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjects.back());
    generatedObjectGroupBoxLayout->addWidget(chanceLabel);
    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjectChanceSpinBoxes.back());
    generatedObjectGroupBoxLayout->addWidget(ltexLabel);
    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjectTerrainTexType.back());

    generatedObjectGroupBoxLayout->addWidget(mRandomZRotationCheckBox.back());
    generatedObjectGroupBoxLayout->addWidget(randomRotationLabel);
    generatedObjectGroupBoxLayout->addWidget(mRandomRotation.back());
    generatedObjectGroupBoxLayout->addWidget(randomDisplacementLabel);
    generatedObjectGroupBoxLayout->addWidget(mRandomDisplacement.back());
    generatedObjectGroupBoxLayout->addWidget(zDisplacementLabel);
    generatedObjectGroupBoxLayout->addWidget(mZDisplacement.back());
    generatedObjectGroupBoxLayout->addWidget(minZHeightLabel);
    generatedObjectGroupBoxLayout->addWidget(mMinZHeight.back());
    generatedObjectGroupBoxLayout->addWidget(maxZHeightLabel);
    generatedObjectGroupBoxLayout->addWidget(mMaxZHeight.back());
    generatedObjectGroupBoxLayout->addWidget(followLandShapeLabel);
    generatedObjectGroupBoxLayout->addWidget(mFollowLandShapeFactor.back());

    generatedObjectGroupBox->setLayout(generatedObjectGroupBoxLayout);
    mGeneratedObjectsLayout->addWidget(generatedObjectGroupBox);
}

void CSMWorld::ObjectProcGenTool::deleteGenerationObject()
{
    if(mGeneratedObjects.size() > 1)
    {
        QWidget *toBeDeleted;
        if ((toBeDeleted = mGeneratedObjectsLayout->takeAt(mGeneratedObjectsLayout->count() - 1)->widget()) != 0)
            delete toBeDeleted;

        mMaxZHeight.pop_back();
        mMinZHeight.pop_back();
        mZDisplacement.pop_back();
        mRandomDisplacement.pop_back();
        mRandomRotation.pop_back();
        mRandomZRotationCheckBox.pop_back();
        mFollowLandShapeFactor.pop_back();
        mGeneratedObjectTerrainTexType.pop_back();
        mGeneratedObjectChanceSpinBoxes.pop_back();
        mGeneratedObjects.pop_back();
    }
}

void CSMWorld::ObjectProcGenTool::loadGenerationSettings()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Procedural Generation Data"), "", tr("Data files (*.*)"));
    std::ifstream settingsFile (fileName.toUtf8().constData());
    std::string line;
    long unsigned int objectCount = 0;
    if (settingsFile.is_open())
    {
        while(!settingsFile.eof() && !settingsFile.fail())
        {
            getline(settingsFile, line);

            size_t i = 0;

            while (i < line.size() && std::isspace(line[i], std::locale::classic())) // skip whitespaces
            {
                ++i;
            }
            if (i >= line.size())
                continue;

            if (line[i] == '#') // skip comment
                continue;

            size_t settingEnd = line.find('=', i);
            if (settingEnd == std::string::npos)
            {
                std::stringstream error;
                error << "can't read procedural generation setting file";
                throw std::runtime_error(error.str());
            }

            std::string setting = line.substr(i, (settingEnd-i));
            boost::algorithm::trim(setting);

            size_t valueBegin = settingEnd+1;
            std::string value = line.substr(valueBegin);
            boost::algorithm::trim(value);
            bool newObject = false;

            if (setting == "objectName")
            {
                if(mGeneratedObjects.size() <= objectCount) createNewGenerationObject();
                int index = mGeneratedObjects[objectCount]->findText(QString::fromStdString(value));
                mGeneratedObjects[objectCount]->setCurrentIndex(index);
            }

            if (setting == "objectSpawnChance")
            {
                mGeneratedObjectChanceSpinBoxes[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "terrainTextureId")
            {
                int index = mGeneratedObjectTerrainTexType[objectCount]->findText(QString::fromStdString(value));
                mGeneratedObjectTerrainTexType[objectCount]->setCurrentIndex(index);
            }

            if (setting == "randomZRotation")
            {
                mRandomZRotationCheckBox[objectCount]->setChecked(boost::lexical_cast<bool>(value));
            }

            if (setting == "randomXYRotation")
            {
                mRandomRotation[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "randomDisplacement")
            {
                mRandomDisplacement[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "zDisplacement")
            {
                mZDisplacement[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "minZHeight")
            {
                mMinZHeight[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "maxZHeight")
            {
                mMaxZHeight[objectCount]->setValue(boost::lexical_cast<double>(value));
            }

            if (setting == "followLandShapeFactor")
            {
                mFollowLandShapeFactor[objectCount]->setValue(boost::lexical_cast<double>(value));
                newObject = true;
            }

            if (newObject) ++objectCount;
        }
    }
    settingsFile.close();
}

void CSMWorld::ObjectProcGenTool::saveGenerationSettings()
{
    std::ofstream settingsFile;
    settingsFile.open ("procgendata.txt");
    settingsFile << "# This file was saved in OpenCS, and holds settings for procedurally generated objects. \n";
    for(long unsigned int i = 0; i < mGeneratedObjects.size(); ++i)
    {
        settingsFile << "# Id of object \n";
        settingsFile << "objectName = " << mGeneratedObjects[i]->currentText().toUtf8().constData() << "\n";
        settingsFile << "# Spawn chance of object, float, min 0.0, max 1.0 \n";
        settingsFile << "objectSpawnChance = " << mGeneratedObjectChanceSpinBoxes[i]->value() << "\n";
        settingsFile << "# Id of the suitable terrain texture \n";
        settingsFile << "terrainTextureId = " << mGeneratedObjectTerrainTexType[i]->currentText().toUtf8().constData() << "\n";
        settingsFile << "# Random Z rotation (in radians). Randomized for + and -, therefore the value here should be halved. \n";
        settingsFile << "randomZRotation = " << mRandomZRotationCheckBox[i]->isChecked() << "\n";
        settingsFile << "# Random X and Y rotation (in radians). Randomized for + and -, therefore the value here should be halved. \n";
        settingsFile << "randomXYRotation = " << mRandomRotation[i]->value() << "\n";
        settingsFile << "# Random displacement. Randomized for + and -, therefore the value here should be halved. \n";
        settingsFile << "randomDisplacement = " << mRandomDisplacement[i]->value() << "\n";
        settingsFile << "# Fixed Z displacement, used to push object into ground or vice versa. \n";
        settingsFile << "zDisplacement = " << mZDisplacement[i]->value() << "\n";
        settingsFile << "# Minimum Z-level that is allowed for generation, e.g. waterlevel (z = 0.0). \n";
        settingsFile << "minZHeight = " << mMinZHeight[i]->value() << "\n";
        settingsFile << "# Maximum Z-level that is allowed for generation, e.g. waterlevel (z = 0.0). \n";
        settingsFile << "maxZHeight = " << mMaxZHeight[i]->value() << "\n";
        settingsFile << "# Used to control object leaning towards downhill. Value 0.0 is vertical, value of 1.0 is normal to ground. \n";
        settingsFile << "# Values above 1.0 are leaning more towards horizontal. \n";
        settingsFile << "followLandShapeFactor = " << mFollowLandShapeFactor[i]->value() << "\n";
        settingsFile << "\n";
    }
    settingsFile.close();
}

void CSMWorld::ObjectProcGenTool::placeObjectsNow()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> distZeroToOne(0.0, 1.0);
    const float textureXOffset = 0.75f; //+0.25f textureOffset + 0.5f spawn middle of texture
    const float textureYOffset = 0.25f; //-0.25f textureOffset + 0.5f spawn middle of texture

    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

    mDocument.getUndoStack().beginMacro ("Procedurally generate object references");

    for (int cellX = mCellXSpinBoxCornerA->value(); cellX <= mCellXSpinBoxCornerB->value(); ++cellX)
    {
        for (int cellY = mCellXSpinBoxCornerA->value(); cellY <= mCellXSpinBoxCornerB->value(); ++cellY)
        {
            std::string cellId (CSMWorld::CellCoordinates::generateId(cellX, cellY));
            CSMWorld::LandTexturesColumn::DataType landTexPointer = landTable.data(landTable.getModelIndex(cellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                for(std::vector<int>::size_type objectCount = 0; objectCount != mGeneratedObjects.size(); objectCount++)
                {
                    std::uniform_real_distribution<double> distZRotation(-3.12 * mRandomZRotationCheckBox[objectCount]->isChecked(), 3.12 * mRandomZRotationCheckBox[objectCount]->isChecked());
                    std::uniform_real_distribution<double> distXAndYRotation(-mRandomRotation[objectCount]->value(), mRandomRotation[objectCount]->value());
                    std::uniform_int_distribution<int> distDisplacement(-mRandomDisplacement[objectCount]->value(), mRandomDisplacement[objectCount]->value());
                    std::size_t hashlocation = mGeneratedObjectTerrainTexType[objectCount]->currentText().toStdString().find("#");
                    for (int xInCell = 0; xInCell < landTextureSize; ++xInCell)
                    {
                        for (int yInCell = 0; yInCell < landTextureSize; ++yInCell)
                        {
                        if(landTexPointer[yInCell * landTextureSize + xInCell] == stoi(mGeneratedObjectTerrainTexType[objectCount]->currentText().toStdString().substr (hashlocation+1))+1 &&
                            distZeroToOne(mt) < mGeneratedObjectChanceSpinBoxes[objectCount]->value())
                                placeObject(mGeneratedObjects[objectCount]->currentText(),
                                    cellSize * static_cast<float>((cellX * landTextureSize) + xInCell + textureXOffset) / landTextureSize + distDisplacement(mt) , // Calculate worldPos from landtex coordinate
                                    cellSize * static_cast<float>((cellY * landTextureSize) + yInCell + textureYOffset) / landTextureSize + distDisplacement(mt), // Calculate worldPos from landtex coordinate
                                    distXAndYRotation(mt), distXAndYRotation(mt), distZRotation(mt),
                                    mFollowLandShapeFactor[objectCount]->value(), mZDisplacement[objectCount]->value(),
                                    mMinZHeight[objectCount]->value(), mMaxZHeight[objectCount]->value());
                    }
                }
            }
        }
    }
    mDocument.getUndoStack().endMacro ();
}

// This is a copy of of CSVRender::InstanceMode::quatToEuler
osg::Vec3f CSMWorld::ObjectProcGenTool::quatToEuler(const osg::Quat& rot) const
{
    float x, y, z;
    float test = 2 * (rot.w() * rot.y() + rot.x() * rot.z());

    if (std::abs(test) >= 1.f)
    {
        x = atan2(rot.x(), rot.w());
        y = (test > 0) ? (osg::PI / 2) : (-osg::PI / 2);
        z = 0;
    }
    else
    {
        x = std::atan2(2 * (rot.w() * rot.x() - rot.y() * rot.z()), 1 - 2 * (rot.x() * rot.x() + rot.y() * rot.y()));
        y = std::asin(test);
        z = std::atan2(2 * (rot.w() * rot.z() - rot.x() * rot.y()), 1 - 2 * (rot.y() * rot.y() + rot.z() * rot.z()));
    }

    return osg::Vec3f(-x, -y, -z);
}

// This is a copy of of CSVRender::InstanceMode::eulerToQuat
osg::Quat CSMWorld::ObjectProcGenTool::eulerToQuat(const osg::Vec3f& euler) const
{
    osg::Quat xr = osg::Quat(-euler[0], osg::Vec3f(1,0,0));
    osg::Quat yr = osg::Quat(-euler[1], osg::Vec3f(0,1,0));
    osg::Quat zr = osg::Quat(-euler[2], osg::Vec3f(0,0,1));

    return zr * yr * xr;
}

void CSMWorld::ObjectProcGenTool::placeObject(QString objectId, float xWorldPos, float yWorldPos,
    float xRot, float yRot, float zRot, float followLandShapeFactor, float zDisplacement, int minZHeight, int maxZHeight)
{
    CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

    const float xd = static_cast<float>(xWorldPos * (landSize - 1) / cellSize);
    const float yd = static_cast<float>(yWorldPos * (landSize - 1) / cellSize);
    int x = static_cast<int>(std::floor(xd));
    int y = static_cast<int>(std::floor(yd));
    float xFloatDifference = xd - x;
    float yFloatDifference = yd - y;

    int cellX = std::floor(static_cast<float>(x) / (landSize - 1));
    int cellY = std::floor(static_cast<float>(y) / (landSize - 1));
    std::string cellId (CSMWorld::CellCoordinates::generateId(cellX, cellY));

    //Calculate local vertex coordinate from global vertex coordinate
    int localX (x - cellX * (landSize - 1));
    int localY (y - cellY * (landSize - 1));

    //Calculate land height, similar method is in CSVRender::TerrainSelection::calculateLandHeight
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType landPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    float zWorldPos = landPointer[localY*landSize + localX];

    if (zWorldPos < minZHeight || zWorldPos > maxZHeight ) return;

    //These are used to interpolate correct height and slope
    float zWorldPosXPlus1 = zWorldPos;
    float zWorldPosYPlus1 = zWorldPos;
    float zWorldPosXPlus1YPlus1 = zWorldPos;

    if (localX < landSize - 2) zWorldPosXPlus1 = landPointer[localY*landSize + localX + 1];
    else
    {
        std::string rightCellId (CSMWorld::CellCoordinates::generateId(cellX + 1, cellY));
        const CSMWorld::LandHeightsColumn::DataType rightLandPointer = landTable.data(landTable.getModelIndex(rightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosXPlus1 = rightLandPointer[localY*landSize + 1];
    }
    if (localY < landSize - 2) zWorldPosYPlus1 = landPointer[(localY + 1)*landSize + localX];
    else
    {
        std::string downCellId (CSMWorld::CellCoordinates::generateId(cellX, cellY + 1));
        const CSMWorld::LandHeightsColumn::DataType downLandPointer = landTable.data(landTable.getModelIndex(downCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosYPlus1 = downLandPointer[localX + 1];
    }
    if (localX < landSize - 2 && localY < landSize - 2) zWorldPosXPlus1YPlus1 = landPointer[(localY + 1)*landSize + localX + 1];
    else if (localY < landSize - 2)
    {
        std::string rightCellId (CSMWorld::CellCoordinates::generateId(cellX + 1, cellY));
        const CSMWorld::LandHeightsColumn::DataType rightLandPointer = landTable.data(landTable.getModelIndex(rightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosXPlus1YPlus1 = rightLandPointer[localY*landSize + 1];
    }
    else if (localX < landSize - 2)
    {
        std::string downCellId (CSMWorld::CellCoordinates::generateId(cellX, cellY + 1));
        const CSMWorld::LandHeightsColumn::DataType downLandPointer = landTable.data(landTable.getModelIndex(downCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosXPlus1YPlus1 = downLandPointer[localX + 1];
    }
    else
    {
        std::string downRightCellId (CSMWorld::CellCoordinates::generateId(cellX + 1, cellY + 1));
        const CSMWorld::LandHeightsColumn::DataType downRightLandPointer = landTable.data(landTable.getModelIndex(downRightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosXPlus1YPlus1 = downRightLandPointer[0];
    }

    // Bilinear interpolation of height
    float xInterpolateWhenY0 = zWorldPosXPlus1 * xFloatDifference + zWorldPos * (1 - xFloatDifference);
    float xInterpolateWhenY1 = zWorldPosXPlus1YPlus1 * xFloatDifference + zWorldPosYPlus1 * (1 - xFloatDifference);
    float finalInterpolation = xInterpolateWhenY1 * yFloatDifference + xInterpolateWhenY0 * (1 - yFloatDifference);

    // Average slope
    float xSlopeWhenY0 = zWorldPosXPlus1 - zWorldPos;
    float xSlopeWhenY1 = zWorldPosXPlus1YPlus1 - zWorldPosYPlus1;
    float ySlopeWhenX0 = zWorldPosYPlus1 - zWorldPos;
    float ySlopeWhenX1 = zWorldPosXPlus1YPlus1 - zWorldPosXPlus1;
    float xSlope = (xSlopeWhenY0 + xSlopeWhenY1) / 2;
    float ySlope = (ySlopeWhenX0 + ySlopeWhenX1) / 2;

    zWorldPos = finalInterpolation;

    //Quat rotation code is modified from CSVRender::InstanceMode::drag.
    osg::Quat rotationX = osg::Quat( -std::atan(xSlope / (cellSize / landSize)) * followLandShapeFactor , osg::Y_AXIS);
    osg::Quat rotationY = osg::Quat( std::atan(ySlope / (cellSize / landSize)) * followLandShapeFactor , osg::X_AXIS);
    osg::Quat currentRot = eulerToQuat(osg::Vec3f(xRot, yRot, zRot));
    osg::Quat combined = currentRot * rotationY;
    combined *= rotationX;
    osg::Vec3f euler = quatToEuler(combined);
    if (!euler.isNaN())
    {
        xRot = euler.x();
        yRot = euler.y();
        zRot = euler.z();
    }

    std::unique_ptr<CSMWorld::CreateCommand> createCommand (
        new CSMWorld::CreateCommand (
        referencesTable, mDocument.getData().getReferences().getNewId()));

    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_Cell), QString::fromStdString (cellId));
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXPos), xWorldPos);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYPos), yWorldPos);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZPos), zWorldPos + zDisplacement);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXRot), osg::RadiansToDegrees(xRot));
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYRot), osg::RadiansToDegrees(yRot));
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZRot), osg::RadiansToDegrees(zRot));
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_ReferenceableId),
        objectId);

    mDocument.getUndoStack().push (createCommand.release());
}
