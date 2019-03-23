#include "objectprocgentool.hpp"

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUndoStack>
#include <QPushButton>
#include <QComboBox>

#include "../doc/document.hpp"
#include "../world/idtable.hpp"
#include "columnbase.hpp"
#include "commandmacro.hpp"
#include "commands.hpp"
#include "cellcoordinates.hpp"

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

    mGeneratedObjects.push_back(new QComboBox(this));

    CSMWorld::IdTable& referenceablesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    for (auto i : mGeneratedObjects)
    {
        for (int j = 0; j < referenceablesTable.rowCount(); ++j)
        {
            i->addItem(QString::fromStdString(referenceablesTable.getId(j)));
        }
    }

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

    for (auto i : mGeneratedObjects)
    {
        mGeneratedObjectChanceSpinBoxes.push_back(new QDoubleSpinBox);
        mGeneratedObjectChanceSpinBoxes.back()->setRange(0.f, 1.f);
        mGeneratedObjectChanceSpinBoxes.back()->setSingleStep(0.1f);
        mGeneratedObjectChanceSpinBoxes.back()->setValue(0.7f);
        mGeneratedObjectsLayout->addWidget(i);
        mGeneratedObjectsLayout->addWidget(mGeneratedObjectChanceSpinBoxes.back());
    }

    mMainLayout->addLayout(mCellCoordinatesLayout);
    mMainLayout->addLayout(mGeneratedObjectsLayout);
    mMainLayout->addWidget(mDeleteGenerationObjectButton);
    mMainLayout->addWidget(mNewGenerationObjectButton);
    mMainLayout->addWidget(mActionButton);
    mSpinBoxGroup->setLayout(mMainLayout);
}

void CSMWorld::ObjectProcGenTool::createNewGenerationObject()
{
    mGeneratedObjects.push_back(new QComboBox(this));
    mGeneratedObjectChanceSpinBoxes.push_back(new QDoubleSpinBox);
    mGeneratedObjectChanceSpinBoxes.back()->setRange(0.f, 1.f);
    mGeneratedObjectChanceSpinBoxes.back()->setSingleStep(0.1f);
    mGeneratedObjectChanceSpinBoxes.back()->setValue(0.7f);

    CSMWorld::IdTable& referenceablesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));
    for (int j = 0; j < referenceablesTable.rowCount(); ++j)
    {
        mGeneratedObjects.back()->addItem(QString::fromStdString(referenceablesTable.getId(j)));
    }

    mGeneratedObjectsLayout->addWidget(mGeneratedObjects.back());
    mGeneratedObjectsLayout->addWidget(mGeneratedObjectChanceSpinBoxes.back());
}

void CSMWorld::ObjectProcGenTool::deleteGenerationObject()
{
    if(mGeneratedObjects.size() > 1)
    {
        QWidget *toBeDeleted;
        if ((toBeDeleted = mGeneratedObjectsLayout->takeAt(mGeneratedObjectsLayout->count() - 1)->widget()) != 0)
            delete toBeDeleted;
        if ((toBeDeleted = mGeneratedObjectsLayout->takeAt(mGeneratedObjectsLayout->count() - 1)->widget()) != 0)
            delete toBeDeleted;

        mGeneratedObjectChanceSpinBoxes.pop_back();
        mGeneratedObjects.pop_back();
    }
}

void CSMWorld::ObjectProcGenTool::placeObjectsNow()
{
    const int cellSize {ESM::Land::REAL_SIZE};
    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

    for (int cellX = mCellXSpinBoxCornerA->value(); cellX <= mCellXSpinBoxCornerB->value(); ++cellX)
    {
        for (int cellY = mCellXSpinBoxCornerA->value(); cellY <= mCellXSpinBoxCornerB->value(); ++cellY)
        {
            for (int xInCell = 0; xInCell < landTextureSize; ++xInCell)
            {
                for (int yInCell = 0; yInCell < landTextureSize; ++yInCell)
                {
                    placeObject(cellX, cellY,
                        cellSize * static_cast<float>((cellX * landTextureSize) + xInCell) / landTextureSize,
                        cellSize * static_cast<float>((cellY * landTextureSize) + yInCell) / landTextureSize); //TODO: Add conditions for generation
                }
            }
        }
    }
}

void CSMWorld::ObjectProcGenTool::placeObject(int cellX, int cellY, float xWorldPos, float yWorldPos) //TODO: Add proper values
{
    CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

    std::unique_ptr<CSMWorld::CreateCommand> createCommand (
        new CSMWorld::CreateCommand (
        referencesTable, mDocument.getData().getReferences().getNewId()));

    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_Cell), QString::fromStdString ("#" + std::to_string(cellX) + " " + std::to_string(cellY)));
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXPos), xWorldPos);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYPos), yWorldPos);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZPos), 100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionXRot), float(rand() % 40 - 20)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionYRot), float(rand() % 40 - 20)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_PositionZRot), float(rand() % 624 - 312)/100);
    createCommand->addValue (referencesTable.findColumnIndex (
        CSMWorld::Columns::ColumnId_ReferenceableId),
        mGeneratedObjects.back()->currentText());

    mDocument.getUndoStack().push (createCommand.release());
}
