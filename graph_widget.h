#pragma once

// Рисует граф в окне приложения. Узлы расставляются по окружности,
// рёбра — со стрелками и подписью веса. После запуска поиска
// кратчайший путь подсвечивается оранжевым. Узлы можно перемещать мышкой

#include "graph.h"
#include <QWidget>
#include <QVector>
#include <QPointF>

class GraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    void setGraph(const Graph &graph);
    void setHighlightedPath(const QVector<int> &path);
    void clearHighlight();
    void resetLayout();

protected:
    void paintEvent(QPaintEvent *event)        override;
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event)      override;

private:
    void    layoutNodesOnCircle();
    QPointF nodePosition(int node) const;
    int     nodeAt(const QPointF &pos) const;
    bool    isPathEdge(int from, int to) const;
    bool    isPathNode(int node) const;

    Graph            m_graph;
    QVector<QPointF> m_positions;       // m_positions[v] — координаты узла v
    QVector<int>     m_highlightedPath; // узлы подсвеченного пути по порядку
    int              m_draggingNode = -1;
    QPointF          m_dragOffset;
};

