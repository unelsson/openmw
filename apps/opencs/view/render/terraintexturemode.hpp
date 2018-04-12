#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QWidget>
#include <QIcon>
#include <QFrame>
#include <QEvent>
#include <QHBoxLayout>

#include <QPushButton>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/ref_ptr>

#include <components/terrain/terraingrid.hpp>

#include "../widget/modebutton.hpp"
#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/cell.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/data.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSMWorld
{
    class Data;
}

namespace CSVRender
{
    class BrushSizeControls : public QGroupBox
    {
        Q_OBJECT

        public:
            BrushSizeControls(const QString &title, QWidget *parent);

        private:
            QSlider *brushSizeSlider;
            QSpinBox *brushSizeSpinBox;
            QHBoxLayout *layoutSliderSize;
    };

    class TextureBrushButton : public QPushButton
    {
        Q_OBJECT

        public:
            TextureBrushButton (const QIcon& icon, const QString& tooltip = "",
                QWidget *parent = 0);
            virtual void dragEnterEvent (QDragEnterEvent *event);
            virtual void dropEvent (QDropEvent *event);

        signals:
            void passBrushTexture(std::string brushTexture);

    };

    class TextureBrushWindow : public QWidget
    {
        Q_OBJECT

        public:
            TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);
            void configureButtonInitialSettings(TextureBrushButton *button);

            QIcon drawIconTexture(QPixmap pixmapBrush);

            QIcon pointIcon;
            QIcon squareIcon;
            QIcon circleIcon;
            QIcon customIcon;

            const std::string iconPointImage = ":scenetoolbar/brush-point";
            const std::string iconSquareImage = ":scenetoolbar/brush-square";
            const std::string iconCircleImage = ":scenetoolbar/brush-circle";
            const std::string iconCustomImage = ":scenetoolbar/brush-custom";

            TextureBrushButton *buttonPoint = new TextureBrushButton(pointIcon, "", this);
            TextureBrushButton *buttonSquare = new TextureBrushButton(squareIcon, "", this);
            TextureBrushButton *buttonCircle = new TextureBrushButton(circleIcon, "", this);
            TextureBrushButton *buttonCustom = new TextureBrushButton(customIcon, "", this);

        private:
            QLabel *selectedBrush;
            QGroupBox *horizontalGroupBox;
            int mButtonSize;
            int mIconSize;
            WorldspaceWidget *mWorldspaceWidget;
            std::string mBrushTexture = "#0";
            std::string mBrushTextureLabel;

        public slots:
            void getBrushTexture(std::string brushTexture);
    };

    class TerrainTextureMode : public EditMode
    {
        Q_OBJECT

        public:
            std::string mBrushTexture;

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            void primaryEditPressed(const WorldspaceHitResult&);
            void primarySelectPressed(const WorldspaceHitResult&);
            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            virtual bool primaryEditStartDrag (const QPoint& pos);
            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);
            virtual void dragCompleted(const QPoint& pos);
            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragEnterEvent (QDragEnterEvent *event);
            virtual void dropEvent (QDropEvent *event);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            void modifyLandMacro (CSMWorld::CommandMacro& commands);

        private:
            TextureBrushWindow *textureBrushWindow;
            std::unique_ptr<Terrain::TerrainGrid> mTerrain;
            CSMWorld::CellCoordinates mCoordinates;
            std::string mId;
            osg::ref_ptr<osg::Group> mCellNode;
    };
}


#endif
