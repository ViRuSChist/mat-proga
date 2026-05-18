#include "graph.h"

Graph::Graph(int nodeCount)
{
    setNodeCount(nodeCount);
}

void Graph::setNodeCount(int n)
{
    if (n < 0)
        n = 0;
    m_nodeCount = n;

    // Если число узлов уменьшили, какие-то рёбра могли остаться "висеть" -
    //  — ссылаться на узлы, которых больше нет. Выкидываем такие
    QVector<Edge> kept;
    kept.reserve(m_edges.size());
    for (const Edge &e : m_edges) {
        if (isValidNode(e.from) && isValidNode(e.to))
            kept.append(e);
    }
    m_edges = kept;
}

bool Graph::isValidNode(int node) const
{
    return node >= 1 && node <= m_nodeCount;
}

bool Graph::addEdge(int from, int to, double weight, QString *errorMsg)
{
    if (!isValidNode(from) || !isValidNode(to)) {
        if (errorMsg)
            *errorMsg = QString("Узел вне диапазона [1; %1].").arg(m_nodeCount);
        return false;
    }

    // Дорога из узла в себя не нужна — в кратчайший путь она войти не может.
    if (from == to) {
        if (errorMsg)
            *errorMsg = QString("Петли (ребро %1 → %1) не допускаются.").arg(from);
        return false;
    }

    // Если одно и то же ребро добавить дважды, при поиске пути будут
    // дубли вариантов. Чтобы заменить вес — нужно сначала удалить старое.
    if (hasEdge(from, to)) {
        if (errorMsg)
            *errorMsg = QString("Ребро %1 → %2 уже существует.").arg(from).arg(to);
        return false;
    }

    m_edges.append({ from, to, weight });
    return true;
}

bool Graph::removeEdge(int from, int to)
{
    for (int i = 0; i < m_edges.size(); ++i) {
        if (m_edges[i].from == from && m_edges[i].to == to) {
            m_edges.removeAt(i);
            return true;
        }
    }
    return false;
}

bool Graph::hasEdge(int from, int to) const
{
    for (const Edge &e : m_edges) {
        if (e.from == from && e.to == to)
            return true;
    }
    return false;
}

QVector<Graph::Edge> Graph::outgoingEdges(int node) const
{
    QVector<Edge> result;
    for (const Edge &e : m_edges) {
        if (e.from == node)
            result.append(e);
    }
    return result;
}

void Graph::clear()
{
    m_nodeCount = 0;
    m_edges.clear();
}
