#pragma once

// Ориентированный граф с весами на рёбрах. Узлы нумеруются с 1.

#include <QVector>
#include <QString>

class Graph
{
public:
    struct Edge
    {
        int    from;
        int    to;
        double weight;
    };

    Graph() = default;
    explicit Graph(int nodeCount);

    void setNodeCount(int n);

    int nodeCount() const { return m_nodeCount; }
    int edgeCount() const { return m_edges.size(); }

    // Если данные плохие — возвращает false и записывает причину в errorMsg. Граф при этом не меняется.
    bool addEdge(int from, int to, double weight, QString *errorMsg = nullptr);

    bool removeEdge(int from, int to);
    bool hasEdge(int from, int to) const;

    const QVector<Edge> &edges() const { return m_edges; }

    // Все рёбра, у которых начало — указанный узел. Используется в алгоритме ДП для перебора соседних узлов.
    QVector<Edge> outgoingEdges(int node) const;

    void clear();

    bool isValidNode(int node) const;

private:
    int           m_nodeCount = 0;
    QVector<Edge> m_edges;
};

