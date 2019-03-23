#ifndef CSM_WORLD_OBJECTPROCGENTOOL_H
#define CSM_WORLD_OBJECTPROCGENTOOL_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>

#include "../doc/document.hpp"

#include "cellcoordinates.hpp"

namespace CSMWorld
{
    class ObjectProcGenTool : public QWidget
    {
        Q_OBJECT

        public:
        ObjectProcGenTool(CSMDoc::Document& document, QWidget* parent = 0);
        ~ObjectProcGenTool ();

        private:
        void createInterface();
        void placeObject(QString, int, int, float, float);

        CSMDoc::Document& mDocument;
        QGroupBox *mSpinBoxGroup;
        QLabel *mCornerALabel;
        QLabel *mCornerBLabel;
        QSpinBox *mCellXSpinBoxCornerA;
        QSpinBox *mCellYSpinBoxCornerA;
        QSpinBox *mCellXSpinBoxCornerB;
        QSpinBox *mCellYSpinBoxCornerB;
        QPushButton *mDeleteGenerationObjectButton;
        QPushButton *mNewGenerationObjectButton;
        QPushButton *mActionButton;
        QVBoxLayout *mMainLayout;
        QVBoxLayout *mCellCoordinatesLayout;
        QVBoxLayout *mGeneratedObjectsLayout;
        std::vector<QComboBox*> mGeneratedObjects;
        std::vector<QDoubleSpinBox*> mGeneratedObjectChanceSpinBoxes;

        const int landSize {ESM::Land::LAND_SIZE};

        private slots:
        void createNewGenerationObject();
        void deleteGenerationObject();
        void placeObjectsNow();
    };
}

#endif
