#include "objectprocgentool.hpp"

#include <random>

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QUndoStack>
#include <QPushButton>
#include <QComboBox>

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

    mActionButton = new QPushButton("Generate!", this);

    mMainLayout = new QVBoxLayout;
    mCellCoordinatesLayout = new QVBoxLayout;
    mGeneratedObjectsLayout = new QVBoxLayout;

    mCellCoordinatesLayout->addWidget(mCornerALabel);
    mCellCoordinatesLayout->addWidget(mCellXSpinBoxCornerA);
    mCellCoordinatesLayout->addWidget(mCellYSpinBoxCornerA);
    mCellCoordinatesLayout->addWidget(mCornerBLabel);
    mCellCoordinatesLayout->addWidget(mCellXSpinBoxCornerB);
    mCellCoordinatesLayout->addWidget(mCellYSpinBoxCornerB);

    createNewGenerationObject();

    mMainLayout->addLayout(mCellCoordinatesLayout);
    mMainLayout->addLayout(mGeneratedObjectsLayout);
    mMainLayout->addWidget(mDeleteGenerationObjectButton);
    mMainLayout->addWidget(mNewGenerationObjectButton);
    mMainLayout->addWidget(mActionButton);
    mSpinBoxGroup->setLayout(mMainLayout);
}

void CSMWorld::ObjectProcGenTool::createNewGenerationObject()
{
    QGroupBox* generatedObjectGroupBox; //layout holding single generation object
    QHBoxLayout* generatedObjectGroupBoxLayout; //layout holding single generation object
    generatedObjectGroupBox = new QGroupBox;
    generatedObjectGroupBoxLayout = new QHBoxLayout;

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

void CSMWorld::ObjectProcGenTool::placeObjectsNow()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

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
                            dist(mt) < mGeneratedObjectChanceSpinBoxes[objectCount]->value())
                                placeObject(mGeneratedObjects[objectCount]->currentText(), cellId, cellX, cellY,
                                    cellSize * static_cast<float>((cellX * landTextureSize) + xInCell) / landTextureSize,
                                    cellSize * static_cast<float>((cellY * landTextureSize) + yInCell) / landTextureSize);
                    }
                }
            }
        }
    }
}

void CSMWorld::ObjectProcGenTool::placeObject(QString objectId, std::string cellId, int cellX, int cellY, float xWorldPos, float yWorldPos)
{
    CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

    //This should be CSMWorld::CellCoordinates::toVertexCoords(xWorldPos, yWorldPos)
    const auto xd = static_cast<float>(xWorldPos * (landSize - 1) / cellSize + 0.5f);
    const auto yd = static_cast<float>(yWorldPos * (landSize - 1) / cellSize + 0.5f);
    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    //This is also in CSVRender::TerrainSelection::calculateLandHeight, should be moved to CellCoordinates class
    int localX (x - cellX * (landSize - 1));
    int localY (y - cellY * (landSize - 1));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType mPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    float zWorldPos = mPointer[localY*landSize + localX];

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
        CSMWorld::Columns::ColumnId_PositionZPos), zWorldPos);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXRot), float(rand() % 40 - 20)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYRot), float(rand() % 40 - 20)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZRot), float(rand() % 624 - 312)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_ReferenceableId),
        objectId);

    mDocument.getUndoStack().push (createCommand.release());
}
