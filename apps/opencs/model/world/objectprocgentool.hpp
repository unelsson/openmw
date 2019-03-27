#ifndef CSM_WORLD_OBJECTPROCGENTOOL_H
#define CSM_WORLD_OBJECTPROCGENTOOL_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>

#include <osg/Quat>

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
        void placeObject(QString, float, float, float, float, float);
        osg::Vec3f quatToEuler(const osg::Quat& quat) const;
        osg::Quat eulerToQuat(const osg::Vec3f& euler) const;

        CSMDoc::Document& mDocument;
        QGroupBox *mSpinBoxGroup;
        QLabel *mCornerALabel;
        QLabel *mCornerBLabel;
        QSpinBox *mCellXSpinBoxCornerA;
        QSpinBox *mCellYSpinBoxCornerA;
        QSpinBox *mCellXSpinBoxCornerB;
        QSpinBox *mCellYSpinBoxCornerB;
        QLabel *mFollowLandShapeLabel;
        QDoubleSpinBox *mFollowLandShapeFactor;
        QCheckBox *mRandomZRotationCheckBox;
        QLabel *mRandomRotationLabel;
        QDoubleSpinBox *mRandomRotation;
        QLabel *mRandomDisplacementLabel;
        QSpinBox *mRandomDisplacement;
        QLabel *mZDisplacementLabel;
        QSpinBox *mZDisplacement;
        QPushButton *mDeleteGenerationObjectButton;
        QPushButton *mNewGenerationObjectButton;
        QPushButton *mActionButton;
        QVBoxLayout *mMainLayout;
        QVBoxLayout *mCellCoordinatesQVBoxLayoutA;
        QVBoxLayout *mCellCoordinatesQVBoxLayoutB;
        QHBoxLayout *mCellCoordinatesQHBoxLayout;
        QVBoxLayout *mGeneratedObjectsLayout; //Layout holding all generated objects
        std::vector<QComboBox*> mGeneratedObjects;
        std::vector<QDoubleSpinBox*> mGeneratedObjectChanceSpinBoxes;
        std::vector<QComboBox*> mGeneratedObjectTerrainTexType;

        const int landSize {ESM::Land::LAND_SIZE};

        private slots:
        void createNewGenerationObject();
        void deleteGenerationObject();
        void placeObjectsNow();
    };
}

#endif
