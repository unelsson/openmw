#include "objectprocgentool.hpp"

#include <random>
#include <fstream>

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    mFollowLandShapeLabel = new QLabel(tr("Rotate to land shape factor:"));;
    mFollowLandShapeFactor = new QDoubleSpinBox;
    mFollowLandShapeFactor->setDecimals(2);
    mFollowLandShapeFactor->setRange(-3.00f, 3.00f); //1.0f makes object point to normal direction
    mFollowLandShapeFactor->setValue(0.25f);

    mRandomZRotationCheckBox = new QCheckBox("Random z-rotation", this);
    mRandomZRotationCheckBox->setChecked(true);

    mRandomRotationLabel = new QLabel(tr("Random xy-rotation"));;
    mRandomRotation = new QDoubleSpinBox;
    mRandomRotation->setDecimals(2);
    mRandomRotation->setRange(0.00f, 3.12f); //radians
    mRandomRotation->setValue(0.08f);

    mRandomDisplacementLabel = new QLabel(tr("Random displacement:"));;
    mRandomDisplacement = new QSpinBox;
    mRandomDisplacement->setRange(0, 99999999); //in worldspace units
    mRandomDisplacement->setValue(500);

    mZDisplacementLabel = new QLabel(tr("Z displacement:"));;
    mZDisplacement = new QSpinBox;
    mZDisplacement->setRange(-99999999, 99999999); //in worldspace units
    mZDisplacement->setValue(0);

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

    mCellCoordinatesQVBoxLayoutA->addWidget(mCornerALabel);
    mCellCoordinatesQVBoxLayoutA->addWidget(mCellXSpinBoxCornerA);
    mCellCoordinatesQVBoxLayoutA->addWidget(mCellYSpinBoxCornerA);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCornerBLabel);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCellXSpinBoxCornerB);
    mCellCoordinatesQVBoxLayoutB->addWidget(mCellYSpinBoxCornerB);
    mCellCoordinatesQHBoxLayout->addLayout(mCellCoordinatesQVBoxLayoutA);
    mCellCoordinatesQHBoxLayout->addLayout(mCellCoordinatesQVBoxLayoutB);

    createNewGenerationObject();

    mMainLayout->addLayout(mCellCoordinatesQHBoxLayout);
    mMainLayout->addLayout(mGeneratedObjectsLayout);
    mMainLayout->addLayout(deleteNewButtonsLayout);
    mMainLayout->addWidget(mLoadGenerationSettingsButton);
    mMainLayout->addWidget(mSaveGenerationSettingsButton);
    mMainLayout->addWidget(mRandomZRotationCheckBox);
    mMainLayout->addWidget(mRandomRotationLabel);
    mMainLayout->addWidget(mRandomRotation);
    mMainLayout->addWidget(mRandomDisplacementLabel);
    mMainLayout->addWidget(mRandomDisplacement);
    mMainLayout->addWidget(mZDisplacementLabel);
    mMainLayout->addWidget(mZDisplacement);
    mMainLayout->addWidget(mFollowLandShapeLabel);
    mMainLayout->addWidget(mFollowLandShapeFactor);
    mMainLayout->addWidget(mActionButton);
    mSpinBoxGroup->setLayout(mMainLayout);
}

void CSMWorld::ObjectProcGenTool::createNewGenerationObject()
{
    QGroupBox* generatedObjectGroupBox; //layout holding single generation object
    QHBoxLayout* generatedObjectGroupBoxLayout; //layout holding single generation object
    generatedObjectGroupBox = new QGroupBox;
    generatedObjectGroupBoxLayout = new QHBoxLayout;

    generatedObjectGroupBox->setObjectName("generatedObjectGroupBox");
    generatedObjectGroupBox->setStyleSheet("QGroupBox#generatedObjectGroupBox {border: 1px solid #000000;}");

    mGeneratedObjects.push_back(new QComboBox(this));
    CSMWorld::IdTable& referenceablesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));
    for (int j = 0; j < referenceablesTable.rowCount(); ++j)
    {
        mGeneratedObjects.back()->addItem(QString::fromStdString(referenceablesTable.getId(j)));
    }

    QLabel* chanceLabel;
    chanceLabel = new QLabel(tr("%"));
    mGeneratedObjectChanceSpinBoxes.push_back(new QDoubleSpinBox);
    mGeneratedObjectChanceSpinBoxes.back()->setRange(0.f, 1.f);
    mGeneratedObjectChanceSpinBoxes.back()->setSingleStep(0.1f);
    mGeneratedObjectChanceSpinBoxes.back()->setValue(0.7f);
    mGeneratedObjectChanceSpinBoxes.back()->setDecimals(3);

    QLabel* ltexLabel;
    ltexLabel = new QLabel(tr("Tex:"));
    mGeneratedObjectTerrainTexType.push_back(new QComboBox);
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));
    for (int j = 0; j < ltexTable.rowCount(); ++j)
    {
        mGeneratedObjectTerrainTexType.back()->addItem(QString::fromStdString(ltexTable.getId(j)));
    }

    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjects.back());
    generatedObjectGroupBoxLayout->addWidget(chanceLabel);
    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjectChanceSpinBoxes.back());
    generatedObjectGroupBoxLayout->addWidget(ltexLabel);
    generatedObjectGroupBoxLayout->addWidget(mGeneratedObjectTerrainTexType.back());
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

            if (setting == "objectName")
            {
                if(mGeneratedObjects.size() <= objectCount) createNewGenerationObject();
                int index = mGeneratedObjects[objectCount]->findText(QString::fromStdString(value));
                mGeneratedObjects[objectCount]->setCurrentIndex(index);
            }

            if (setting == "objectSpawnChance")
            {
                mGeneratedObjectChanceSpinBoxes[objectCount]->setValue(boost::lexical_cast<double>(value));
                ++objectCount;
            }

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
        settingsFile << "objectName = " << mGeneratedObjects[i]->currentText().toUtf8().constData() << "\n";
        settingsFile << "objectSpawnChance = " << mGeneratedObjectChanceSpinBoxes[i]->value() << "\n";
    }
    settingsFile.close();
}

void CSMWorld::ObjectProcGenTool::placeObjectsNow()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> distChanceOfObject(0.0, 1.0);
    std::uniform_real_distribution<double> distRot(-3.12 * mRandomZRotationCheckBox->isChecked(), 3.12 * mRandomZRotationCheckBox->isChecked());
    std::uniform_real_distribution<double> distSmallRot(-mRandomRotation->value(), mRandomRotation->value());
    std::uniform_int_distribution<int> distDisplacement(-mRandomDisplacement->value(), mRandomDisplacement->value());

    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

    mDocument.getUndoStack().beginMacro ("Procedurally generate object references");

    for (int cellX = mCellXSpinBoxCornerA->value(); cellX <= mCellXSpinBoxCornerB->value(); ++cellX)
    {
        for (int cellY = mCellXSpinBoxCornerA->value(); cellY <= mCellXSpinBoxCornerB->value(); ++cellY)
        {
            std::string cellId ("#" + std::to_string(cellX) + " " + std::to_string(cellY)); // should be CSVRender::TerrainSelection::generateId()
            CSMWorld::LandTexturesColumn::DataType landTexPointer = landTable.data(landTable.getModelIndex(cellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
            for (int xInCell = 0; xInCell < landTextureSize; ++xInCell)
            {
                for (int yInCell = 0; yInCell < landTextureSize; ++yInCell)
                {
                    for(std::vector<int>::size_type objectCount = 0; objectCount != mGeneratedObjects.size(); objectCount++)
                    {
                        std::size_t hashlocation = mGeneratedObjectTerrainTexType[objectCount]->currentText().toStdString().find("#");
                        if(landTexPointer[yInCell * landTextureSize + xInCell] == stoi(mGeneratedObjectTerrainTexType[objectCount]->currentText().toStdString().substr (hashlocation+1))+1 &&
                            distChanceOfObject(mt) < mGeneratedObjectChanceSpinBoxes[objectCount]->value())
                                placeObject(mGeneratedObjects[objectCount]->currentText(),
                                    cellSize * static_cast<float>((cellX * landTextureSize) + xInCell) / landTextureSize + distDisplacement(mt), // Calculate worldPos from landtex coordinate
                                    cellSize * static_cast<float>((cellY * landTextureSize) + yInCell) / landTextureSize + distDisplacement(mt), // Calculate worldPos from landtex coordinate
                                    distSmallRot(mt), distSmallRot(mt), distRot(mt));
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
    float xRot, float yRot, float zRot)
{
    CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

    //This should be CSMWorld::CellCoordinates::toVertexCoords(xWorldPos, yWorldPos)
    //Calculate global vertex coordinate from WorldPos
    const auto xd = static_cast<float>(xWorldPos * (landSize - 1) / cellSize + 0.5f);
    const auto yd = static_cast<float>(yWorldPos * (landSize - 1) / cellSize + 0.5f);
    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    //This could be CSMWorld::CellCoordinates::vertexGlobalToCellId
    int cellX = std::floor(static_cast<float>(x) / (landSize - 1));
    int cellY = std::floor(static_cast<float>(y) / (landSize - 1));
    std::string cellId ("#" + std::to_string(cellX) + " " + std::to_string(cellY)); // should be CSVRender::TerrainSelection::generateId()

    //This is also in CSVRender::TerrainSelection::calculateLandHeight, should be moved to CellCoordinates class
    //Calculate local vertex coordinate from global vertex coordinate
    int localX (x - cellX * (landSize - 1));
    int localY (y - cellY * (landSize - 1));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType landPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    float zWorldPos = landPointer[localY*landSize + localX];

    //These are used for calculating slopes. Defaults to no slope (eg. in cell edge).
    float zWorldPosXPlus1 = zWorldPos;
    float zWorldPosYPlus1 = zWorldPos;

    if (localX < landSize - 2) zWorldPosXPlus1 = landPointer[localY*landSize + localX + 1];
    else
    {
        std::string rightCellId ("#" + std::to_string(cellX + 1) + " " + std::to_string(cellY)); // should be CSVRender::TerrainSelection::generateId()
        const CSMWorld::LandHeightsColumn::DataType rightLandPointer = landTable.data(landTable.getModelIndex(rightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosXPlus1 = rightLandPointer[localY*landSize + 1];
    }
    if (localY < landSize - 2) zWorldPosYPlus1 = landPointer[(localY + 1)*landSize + localX];
    else
    {
        std::string downCellId ("#" + std::to_string(cellX) + " " + std::to_string(cellY + 1)); // should be CSVRender::TerrainSelection::generateId()
        const CSMWorld::LandHeightsColumn::DataType downLandPointer = landTable.data(landTable.getModelIndex(downCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        zWorldPosYPlus1 = downLandPointer[localX + 1];
    }

    float xSlope = zWorldPosXPlus1 - zWorldPos;
    float ySlope = zWorldPosYPlus1 - zWorldPos;

    osg::Vec3f axisX;
    osg::Vec3f axisY;
    axisX[0] = 1;
    axisX[1] = 0;
    axisX[2] = 0;
    axisY[0] = 0;
    axisY[1] = 1;
    axisY[2] = 0;

    osg::Quat rotationX;
    osg::Quat rotationY;

    rotationX = osg::Quat(-std::atan((xSlope / (cellSize / landSize)) * mFollowLandShapeFactor->value()), axisX);
    rotationY = osg::Quat(std::atan((ySlope / (cellSize / landSize)) * mFollowLandShapeFactor->value()), axisY);

    //Quat rotation code is a modified from CSVRender::InstanceMode::drag
    osg::Quat currentRot = eulerToQuat(osg::Vec3f(xRot, yRot, zRot));
    osg::Quat combined = currentRot * rotationX;
    combined *= rotationY;
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
        CSMWorld::Columns::ColumnId_PositionZPos), zWorldPos + mZDisplacement->value());
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXRot), xRot);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYRot), yRot);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZRot), zRot);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_ReferenceableId),
        objectId);

    mDocument.getUndoStack().push (createCommand.release());
}
