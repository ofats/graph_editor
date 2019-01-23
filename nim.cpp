#include "nim.h"
#include "ui_nim.h"

#include <QMessageBox>
#include <QGraphicsView>
#include <QFileDialog>
#include <QTextStream>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneMouseEvent>

#include <iostream>
#include <cmath>
#include <set>
#include <queue>

#define LOCATION \
    "[function: " << __func__ << ", line: " << __LINE__ << "]"

constexpr char GRAPHS_PATH[] = "/home/tender-bum/Documents/graphs";
constexpr double NODE_SIZE = 50;
constexpr double TEXT_WIDTH_DELTA = 3;
constexpr double TEXT_HEIGHT_DELTA = 8;

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    std::cerr << "[mouse:press]" << event->scenePos().x() << ' '
              << event->scenePos().y() << std::endl;
    emit mousePressed(event->scenePos().x(), event->scenePos().y());
    QGraphicsScene::mousePressEvent(event);
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    std::cerr << "[mouse:move]" << event->scenePos().x() << ' '
              << event->scenePos().y() << std::endl;
    emit mouseMoved(event->scenePos().x(), event->scenePos().y());
    QGraphicsScene::mouseMoveEvent(event);
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    std::cerr << "[mouse:release]" << event->scenePos().x() << ' '
              << event->scenePos().y() << std::endl;
    emit mouseReleased(event->scenePos().x(), event->scenePos().y());
    QGraphicsScene::mouseReleaseEvent(event);
}

Nim::Nim(QWidget *parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::Nim>())
{
    ui->setupUi(this);

    scene.setSceneRect(0, 0, ui->graphView->width(), ui->graphView->height());
    ui->graphView->setScene(&scene);
    ui->graphView->show();

    setMouseTracking(false);

    connect(ui->loadGraphButton, SIGNAL(clicked()), this, SLOT(load()));
    connect(ui->calcButton, SIGNAL(clicked()), this, SLOT(calcGame()));
    connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(reset()));

    connect(&scene, SIGNAL(mousePressed(double, double)), this, SLOT(mousePressed(double, double)));
    connect(&scene, SIGNAL(mouseMoved(double, double)), this, SLOT(mouseMoved(double, double)));
    connect(&scene, SIGNAL(mouseReleased(double, double)), this, SLOT(mouseReleased(double, double)));
}

Nim::~Nim() = default;

bool Nim::reset() {
    std::cout << LOCATION << std::endl;

    if (!loaded) {
        std::cout << "Nothing to reset." << std::endl;
        return true;
    }

    QMessageBox msgBox{this};
    msgBox.setWindowTitle("Подтвердите очистку");
    msgBox.setText("Текущий граф будет очищен. Продолжить?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    switch (msgBox.exec()) {
    case QMessageBox::Ok:
        resetImpl();
        loaded = false;
        std::cout << "Reset succeded." << std::endl;
        return true;
    default:
        std::cout << "Reset canceled." << std::endl;
        return false;
    }
}

void Nim::resetImpl() {
    std::cout << LOCATION << std::endl;

    ui->adjacencyMatrix->clear();
    ui->adjacencyMatrix->setRowCount(0);
    ui->logsTextBrowser->clear();
    edges.clear();
    backEdges.clear();

    nodeItems.clear();
    edgeItems.clear();
}

void Nim::calcGame() {
    std::vector<size_t> degrees;
    degrees.reserve(edges.size());
    for (const auto& v : edges) {
        degrees.push_back(v.size());
    }

    QPen losePen{Qt::red};
    QPen winPen{Qt::green};

    QTextBrowser tb;
    ui->logsTextBrowser->insertPlainText("Нахождение вершин без исходящих ребер: ");
    std::queue<size_t> cur;
    for (size_t u = 0; u < edges.size(); ++u) {
        if (edges[u].empty()) {
            nodeItems[u].ellipseItem->setPen(losePen);
            cur.push(u);
            ui->logsTextBrowser->insertPlainText(QString::number(u) + ", ");
        }
    }
    ui->logsTextBrowser->insertPlainText("\n");

    enum class NodeState {
        Lose,
        Win
    };
    std::vector<NodeState> result(edges.size(), NodeState::Lose);

    ui->logsTextBrowser->insertPlainText("Начало процесса ретроспективного анализа.\n");
    while (!cur.empty()) {
        size_t u = cur.front();
        ui->logsTextBrowser->insertPlainText("Просмотр вершин, входящих в " + QString::number(u) + " (состояние " +
                                    (result[u] == NodeState::Lose ? "проигрышное" : "выигрышное") + "):\n");

        cur.pop();
        for (int v : backEdges[u]) {
            ui->logsTextBrowser->insertPlainText("\tВершина " + QString::number(v) + ".");
            if (result[u] == NodeState::Lose) {
                result[static_cast<size_t>(v)] = NodeState::Win;
            }
            if (--degrees[static_cast<size_t>(v)] == 0) {
                switch (result[static_cast<size_t>(v)]) {
                case NodeState::Lose:
                    nodeItems[static_cast<size_t>(v)].ellipseItem->setPen(losePen);
                    break;
                case NodeState::Win:
                    nodeItems[static_cast<size_t>(v)].ellipseItem->setPen(winPen);
                    break;
                }
                ui->logsTextBrowser->insertPlainText(" Исходящие ребра разобраны, добавление в очередь.");
                cur.push(static_cast<size_t>(v));
            }
            ui->logsTextBrowser->insertPlainText("\n");
        }
    }
}

void Nim::load() {
    std::cout << LOCATION << std::endl;

    if ((!loaded || reset()) && loadImpl()) {
        loaded = true;
        std::cout << "Load succeded." << std::endl;
    } else {
        std::cout << "Load canceled." << std::endl;
    }
}

bool Nim::loadImpl() {
    std::cout << LOCATION << std::endl;

    for (;;) {
        auto path = QFileDialog::getOpenFileName(this, "Выберите файл с графом:",
                                                 GRAPHS_PATH, "Текстовые файлы (*.txt)");
        if (path.isEmpty()) {
            std::cout << "No file was given." << std::endl;
            return false;
        }

        QFile file{path};
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << "Not readable file was given." << std::endl;
            int but = QMessageBox::critical(this, "Невозможно прочитать файл",
                                            "Файл с данными не доступен для чтения. "
                                            "Попробовать другой файл?",
                                            QMessageBox::Ok | QMessageBox::Cancel);
            if (but == QMessageBox::Ok) {
                continue;
            }
            return false;
        }

        QTextStream in{&file};
        if (!readGraph(in)) {
            std::cerr << "Wrong formated file was given." << std::endl;
            int but = QMessageBox::critical(this, "Неверный формат",
                                            "Формат представления графа неверный. "
                                            "Попробовать другой файл?",
                                            QMessageBox::Ok | QMessageBox::Cancel);
            if (but == QMessageBox::Ok) {
                continue;
            }
            return false;
        }
        break;
    }

    fillAdjacencyMatrix();
    fillGraph();
    return true;
}

bool checkCycleDfs(int u, const std::vector<std::vector<int>>& edges,
                   std::vector<int>& visited) {
    std::cout << LOCATION << std::endl;

    visited[static_cast<size_t>(u)] = 1;
    for (int v : edges[static_cast<size_t>(u)]) {
        if (visited[static_cast<size_t>(v)] == 1) {
            return true;
        }
        if (visited[static_cast<size_t>(v)] == 0 && checkCycleDfs(v, edges, visited)) {
            return true;
        }
    }
    visited[static_cast<size_t>(u)] = 2;
    return false;
}

bool checkCycle(const std::vector<std::vector<int>>& edges) {
    std::cout << LOCATION << std::endl;

    std::vector<int> visited(edges.size(), 0);
    for (int u = 0; u < static_cast<int>(edges.size()); ++u) {
        if (checkCycleDfs(u, edges, visited)) {
            return true;
        }
    }
    return false;
}

bool Nim::readGraph(QTextStream &in) {
    std::cout << LOCATION << std::endl;

    int n, m;
    in >> n >> m;
    std::cout << n << ' ' << m << std::endl;
    std::vector<std::vector<int>> newEdges(static_cast<size_t>(n));
    std::set<std::pair<int, int>> was;
    for (int i = 0; i < m; ++i) {
        int v, u;
        in >> v >> u;
        std::cout << v << ' ' << u << std::endl;
        if (v == u) {
            std::cerr << "Loop in graph" << std::endl;
            return false;
        }
        if (!was.emplace(v, u).second) {
            std::cerr << "Multiple edges in graph" << std::endl;
            return false;
        }
        newEdges[static_cast<size_t>(v)].push_back(u);
    }
    if (checkCycle(newEdges)) {
        std::cerr << "Cycle in graph" << std::endl;
        return false;
    }

    edges = std::move(newEdges);
    backEdges.resize(edges.size());
    for (int u = 0; u < static_cast<int>(edges.size()); ++u) {
        for (int v : edges[static_cast<size_t>(u)]) {
            backEdges[static_cast<size_t>(v)].push_back(u);
        }
    }
    return true;
}

void Nim::fillAdjacencyMatrix() {
    ui->adjacencyMatrix->setColumnCount(static_cast<int>(edges.size()));
    ui->adjacencyMatrix->setRowCount(static_cast<int>(edges.size()));
    for (int u = 0; u < static_cast<int>(edges.size()); ++u) {
        for (int v = 0; v < static_cast<int>(edges.size()); ++v) {
            ui->adjacencyMatrix->setItem(u, v, new QTableWidgetItem(""));
        }
    }
    for (int u = 0; u < static_cast<int>(edges.size()); ++u) {
        for (int v : edges[static_cast<size_t>(u)]) {
            ui->adjacencyMatrix->item(u, v)->setBackgroundColor(palette().color(QPalette::LinkVisited));
        }
    }
}

void Nim::fillGraph() {
    double yPos = ui->graphView->height() / 2 - NODE_SIZE / 2;
    double horDist = ui->graphView->width() / static_cast<int>(edges.size());

    for (size_t i = 0; i < edges.size(); ++i) {
        double xPos = i * horDist + horDist / 2 - NODE_SIZE / 2;
        nodeItems.emplace_back(&scene, xPos, yPos, i);
    }

    for (size_t u = 0; u < edges.size(); ++u) {
        for (int v : edges[u]) {
            edgeItems.emplace_back(&scene, nodeItems[u], nodeItems[static_cast<size_t>(v)], u, v);
        }
    }
}

Nim::NodeItem::NodeItem(GraphicsScene* scene, double x, double y, size_t idx)
    : number(idx)
{
    QBrush ellipseBrush{scene->palette().color(QPalette::Light)};
    QPen ellipsePen{scene->palette().color(QPalette::Text)};

    QBrush textBrush{scene->palette().color(QPalette::Text)};

    ellipseItem.reset(scene->addEllipse(0, 0, NODE_SIZE, NODE_SIZE));
    ellipseItem->setX(x);
    ellipseItem->setY(y);
    ellipseItem->setZValue(1);
    ellipseItem->setBrush(ellipseBrush);
    ellipseItem->setPen(ellipsePen);

    textItem.reset(scene->addSimpleText(QString::number(idx)));
    textItem->setX(x + NODE_SIZE / 2 - TEXT_WIDTH_DELTA);
    textItem->setY(y + NODE_SIZE / 2 - TEXT_HEIGHT_DELTA);
    textItem->setZValue(2);
    textItem->setBrush(textBrush);
}

double length(const QPointF& p) {
    return std::sqrt(p.x() * p.x() + p.y() * p.y());
}

Nim::EdgeItem::EdgeItem(GraphicsScene* scene, const NodeItem& a,
                        const NodeItem& b, size_t u, size_t v)
    : from(u)
    , to(v)
{
    QPen edgePen{scene->palette().color(QPalette::Text)};
    lineItem.reset(scene->addLine(a.ellipseItem->x() + NODE_SIZE / 2,
                                  a.ellipseItem->y() + NODE_SIZE / 2,
                                  b.ellipseItem->x() + NODE_SIZE / 2,
                                  b.ellipseItem->y() + NODE_SIZE / 2));
    lineItem->setZValue(0);
    lineItem->setPen(edgePen);

    QPointF aPoint{a.ellipseItem->x() + NODE_SIZE / 2, a.ellipseItem->y() + NODE_SIZE / 2};
    QPointF bPoint{b.ellipseItem->x() + NODE_SIZE / 2, b.ellipseItem->y() + NODE_SIZE / 2};
    auto arrowA = bPoint - aPoint;
    arrowA /= length(arrowA);
    QPointF arrowN{arrowA.y(), -arrowA.x()};
    auto arrowCon = aPoint + arrowA * (length(bPoint - aPoint) - NODE_SIZE / 2);
    auto arrowLeft = (arrowCon - arrowA * (NODE_SIZE / 2)) + arrowN * (NODE_SIZE / 12);
    auto arrowRight = (arrowCon - arrowA * (NODE_SIZE / 2)) - arrowN * (NODE_SIZE / 12);

    arrowLeftItem.reset(scene->addLine(QLineF{arrowLeft, arrowCon}));
    arrowLeftItem->setZValue(0);
    arrowLeftItem->setPen(edgePen);

    arrowRightItem.reset(scene->addLine(QLineF{arrowRight, arrowCon}));
    arrowRightItem->setZValue(0);
    arrowRightItem->setPen(edgePen);
}

void Nim::mousePressed(double x, double y) {
    std::cerr << LOCATION << std::endl;

    auto it = std::find_if(nodeItems.begin(), nodeItems.end(), [&](const auto& n) {
        QPointF vec = QPointF(x, y) - n.ellipseItem->pos() - QPointF{NODE_SIZE / 2, NODE_SIZE / 2};
        return std::sqrt(vec.x() * vec.x() + vec.y() * vec.y()) <= NODE_SIZE / 2;
    });
    if (it != nodeItems.end()) {
        std::cerr << "Found node for movement" << std::endl;
        lastPos = QPointF{x, y};
        currentMovable = &*it;
    }
}

void Nim::mouseMoved(double x, double y) {
    std::cerr << LOCATION << std::endl;

    if (currentMovable != nullptr) {
        moveNode(x, y);
        lastPos = QPointF{x, y};
    }
}

void Nim::mouseReleased(double x, double y) {
    std::cerr << LOCATION << std::endl;

    if (currentMovable != nullptr) {
        moveNode(x, y);
        currentMovable = nullptr;
    }
}

void Nim::moveNode(double x, double y) {
    std::cerr << LOCATION << std::endl;

    currentMovable->ellipseItem->moveBy(x - lastPos.x(), y - lastPos.y());
    currentMovable->textItem->moveBy(x - lastPos.x(), y - lastPos.y());

    for (auto& edgeItem : edgeItems) {
        if (edgeItem.from == currentMovable->number || edgeItem.to == currentMovable->number) {
            edgeItem = EdgeItem(&scene, nodeItems[edgeItem.from], nodeItems[edgeItem.to], edgeItem.from, edgeItem.to);
        }
    }
}
