#ifndef NIM_H
#define NIM_H

#include <QWidget>
#include <QGraphicsScene>

#include <vector>
#include <memory>
#include <unordered_map>

namespace Ui {
class Nim;
}  // namespace Ui

class QTextStream;

class GraphicsScene : public QGraphicsScene {
    Q_OBJECT

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

signals:
    void mousePressed(double, double);
    void mouseMoved(double, double);
    void mouseReleased(double, double);
};

class Nim : public QWidget {
    Q_OBJECT

public:
    explicit Nim(QWidget* parent = nullptr);
    ~Nim() override;

private slots:
    bool reset();

private:
    void resetImpl();


private slots:
    void calcGame();



private slots:
    void load();

private:
    bool loadImpl();

    bool readGraph(QTextStream& in);
    void fillAdjacencyMatrix();
    void fillGraph();


private slots:
    void mousePressed(double x, double y);
    void mouseMoved(double x, double y);
    void mouseReleased(double x, double y);

    void moveNode(double x, double y);

private:
    std::unique_ptr<Ui::Nim> ui;
    GraphicsScene scene;
    bool loaded = false;
    std::vector<std::vector<int>> edges;
    std::vector<std::vector<int>> backEdges;

    struct NodeItem {
        size_t number;

        std::unique_ptr<QGraphicsEllipseItem> ellipseItem;
        std::unique_ptr<QGraphicsSimpleTextItem> textItem;

        NodeItem(GraphicsScene* scene, double x, double y, size_t idx);
    };
    std::vector<NodeItem> nodeItems;

    struct EdgeItem {
        std::unique_ptr<QGraphicsLineItem> lineItem;
        std::unique_ptr<QGraphicsLineItem> arrowLeftItem;
        std::unique_ptr<QGraphicsLineItem> arrowRightItem;
        size_t from, to;

        EdgeItem(GraphicsScene* scene, const NodeItem& a, const NodeItem& b, size_t u, size_t v);
    };
    std::vector<EdgeItem> edgeItems;

    NodeItem* currentMovable = nullptr;
    QPointF lastPos;
};

#endif // NIM_H
