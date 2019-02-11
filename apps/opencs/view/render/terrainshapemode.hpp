#ifndef CSV_RENDER_TERRAINSHAPEMODE_H
#define CSV_RENDER_TERRAINSHAPEMODE_H

#include "editmode.hpp"

#include <string>
#include <memory>

#include <QWidget>
#include <QEvent>

#ifndef Q_MOC_RUN
#include "../../model/world/data.hpp"
#include "../../model/world/land.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"
#endif

#include "terrainselection.hpp"

namespace CSVWidget
{
    class SceneToolShapeBrush;
}

namespace CSVRender
{
    class PagedWorldspaceWidget;

    /// \brief EditMode for handling the terrain shape editing
    class TerrainShapeMode : public EditMode
    {
        Q_OBJECT

        public:

            enum InteractionType
            {
                InteractionType_PrimaryEdit,
                InteractionType_PrimarySelect,
                InteractionType_SecondaryEdit,
                InteractionType_SecondarySelect,
                InteractionType_None
            };

            /// Editmode for terrain shape grid
            TerrainShapeMode(WorldspaceWidget*, osg::Group* parentNode, QWidget* parent = nullptr);

            /// Create single command for one-click shape editing
            void primaryEditPressed (const WorldspaceHitResult& hit);

            /// Open brush settings window
            void primarySelectPressed(const WorldspaceHitResult&);

            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            /// Start shape editing command macro
            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);

            /// Handle shape edit behavior during dragging
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            /// End shape editing command macro
            virtual void dragCompleted(const QPoint& pos);

            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            /// Reset the height-map array
            void resetAlteredHeightMap ();

            /// Handle brush mechanics for shape editing
            void editTerrainShapeGrid (std::pair<int, int> vertexCoords, bool dragOperation);

            /// Handle brush mechanics for shape selection
            void selectTerrainShapes (std::pair<int, int>, unsigned char, bool);

            /// Push shape edits to command macro
            void pushEditToCommand (CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

            /// Create new cell and land if needed
            bool allowLandShapeEditing(std::string textureFileName);

        private:
            std::string mCellId;
            std::string mBrushTexture;
            int mBrushSize;
            int mBrushShape;
            std::vector<std::pair<int, int>> mCustomBrushShape;
            CSVWidget::SceneToolShapeBrush *mShapeBrushScenetool;
            int mDragMode;
            osg::Group* mParentNode;
            bool mIsEditing;
            std::unique_ptr<TerrainSelection> mTerrainShapeSelection;
            float mAlteredHeightMap[ESM::Land::LAND_SIZE * ESM::Land::LAND_SIZE + ESM::Land::LAND_SIZE];

            const int cellSize {ESM::Land::REAL_SIZE};
            const int landSize {ESM::Land::LAND_SIZE};
            const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

            PagedWorldspaceWidget& getPagedWorldspaceWidget();

        signals:
            void passBrushTexture(std::string brushTexture);

        public slots:
            //void handleDropEvent(QDropEvent *event);
            void setBrushSize(int brushSize);
            void setBrushShape(int brushShape);
    };
}


#endif