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
        void placeObject(QString, float, float, float, float, float, float, float, int, int);
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

        QPushButton *mDeleteGenerationObjectButton;
        QPushButton *mNewGenerationObjectButton;
        QPushButton *mLoadGenerationSettingsButton;
        QPushButton *mSaveGenerationSettingsButton;
        QPushButton *mActionButton;

        QVBoxLayout *mMainLayout;
        QVBoxLayout *mCellCoordinatesQVBoxLayoutA;
        QVBoxLayout *mCellCoordinatesQVBoxLayoutB;
        QHBoxLayout *mCellCoordinatesQHBoxLayout;
        QVBoxLayout *mGeneratedObjectsLayout; //Layout holding all generated objects

        std::vector<QComboBox*> mGeneratedObjects; //Name of object, also used for checking number of objects
        std::vector<QDoubleSpinBox*> mGeneratedObjectChanceSpinBoxes;
        std::vector<QComboBox*> mGeneratedObjectTerrainTexType;
        std::vector<QDoubleSpinBox*> mFollowLandShapeFactor;
        std::vector<QCheckBox*> mRandomZRotationCheckBox;
        std::vector<QDoubleSpinBox*> mRandomRotation;
        std::vector<QSpinBox*> mRandomDisplacement;
        std::vector<QSpinBox*> mZDisplacement;
        std::vector<QSpinBox*> mMinZHeight;
        std::vector<QSpinBox*> mMaxZHeight;

        const int landSize {ESM::Land::LAND_SIZE};

        private slots:
        void createNewGenerationObject();
        void deleteGenerationObject();
        void loadGenerationSettings();
        void saveGenerationSettings();
        void placeObjectsNow();
    };
}

#endif
