#include "dp_solver.h"
#include <QQueue>
#include <limits>

namespace {
// Условная "бесконечность" для f(v) — пока не посчитано или путь не существует.
constexpr double kInfinity = std::numeric_limits<double>::infinity();
}

QVector<int> DPSolver::topologicalSort(const Graph &graph) const
{
    // Алгоритм Кана. Считаем у каждого узла число входящих стрелок,
    // узлы с нулём кладём в очередь. По одному снимаем из очереди,
    // у соседних узлов счётчик уменьшаем — когда счётчик стал нулевым,
    // соседний узел тоже кладём в очередь. Порядок, в котором мы
    // снимали узлы, и есть топологический порядок
    const int n = graph.nodeCount();
    QVector<int> indeg(n + 1, 0);

    for (const Graph::Edge &e : graph.edges())
        ++indeg[e.to];

    QQueue<int> queue;
    for (int v = 1; v <= n; ++v) {
        if (indeg[v] == 0)
            queue.enqueue(v);
    }

    QVector<int> order;
    order.reserve(n);

    while (!queue.isEmpty()) {
        const int v = queue.dequeue();
        order.append(v);
        for (const Graph::Edge &e : graph.outgoingEdges(v)) {
            if (--indeg[e.to] == 0)
                queue.enqueue(e.to);
        }
    }
    return order;
}

QVector<int> DPSolver::reconstructPath(const QVector<int> &next, int from, int to) const
{
    // Когда f(v) посчитано для всех узлов, мы знаем не только стоимость,
    // но и какой соседний узел дал минимум — он записан в next[v].
    // Чтобы собрать сам путь, идём от стартового узла, переходя по next
    // safety защищает от зацикливания, если в next попадут битые данные.
    QVector<int> path;
    int v = from;
    int safety = next.size() + 1;

    while (v != to && safety-- > 0) {
        path.append(v);
        if (v < 0 || v >= next.size() || next[v] < 0)
            return {};
        v = next[v];
    }
    if (v != to)
        return {};

    path.append(to);
    return path;
}

DPSolver::Result DPSolver::solve(const Graph &graph, int from, int to) const
{
    Result res;
    res.from = from;
    res.to   = to;

    const int n = graph.nodeCount();

    if (!graph.isValidNode(from) || !graph.isValidNode(to) || from == to) {
        res.errorMessage = QStringLiteral("Некорректные начальный и/или конечный узлы.");
        return res;
    }

    // Если получился топологический порядок короче, чем число узлов — в графе есть цикл, и метод ДП тут неприменим.
    res.topoOrder = topologicalSort(graph);
    if (res.topoOrder.size() < n) {
        res.errorMessage = QStringLiteral("Граф содержит цикл — метод ДП неприменим.");
        return res;
    }

    // На старте f(v) = бесконечность у всех узлов, кроме конечного.
    // У конечного узла стоимость до самого себя равна 0.
    res.f    = QVector<double>(n + 1, kInfinity);
    res.next = QVector<int>(n + 1, -1);
    res.f[to] = 0.0;

    // Идём по топологическому порядку с конца.
    for (int i = res.topoOrder.size() - 1; i >= 0; --i) {
        const int v = res.topoOrder[i];

        Step step;
        step.node = v;

        if (v == to) {
            // база рекурсии: до конечного узла идти не надо
            step.isBase    = true;
            step.fValue    = 0.0;
            step.chosenVia = -1;
            res.steps.append(step);
            continue;
        }

        step.isBase = false;

        // Перебираем все стрелки из v и ищем минимум суммы w(v,u) + f(u).
        // Все варианты сохраняем в step.variants —
        // потом из них собирается подробный отчёт о ходе решения.
        double bestSum = kInfinity;
        int    bestVia = -1;

        for (const Graph::Edge &e : graph.outgoingEdges(v)) {
            const double fVia = res.f[e.to];
            const double sum  = fVia + e.weight;

            Variant cand;
            cand.via    = e.to;
            cand.weight = e.weight;
            cand.fVia   = fVia;
            cand.sum    = sum;
            step.variants.append(cand);

            if (sum < bestSum) {
                bestSum = sum;
                bestVia = e.to;
            }
        }

        // Если все варианты ведут в недостижимые узлы (f(u) = бесконечность),
        // то и f(v) останется бесконечным. Это нормально на промежуточных
        // узлах — в конце проверим, что хотя бы f(from) конечно.
        res.f[v]       = bestSum;
        res.next[v]    = bestVia;
        step.fValue    = bestSum;
        step.chosenVia = bestVia;

        res.steps.append(step);
    }

    // Если из начального узла не получилось добраться до конечного — f(from) останется бесконечным.
    if (res.f[from] == kInfinity) {
        res.errorMessage = QString("Из узла %1 невозможно добраться до узла %2.")
                               .arg(from).arg(to);
        return res;
    }

    res.path = reconstructPath(res.next, from, to);
    if (res.path.isEmpty()) {
        res.errorMessage = QStringLiteral("Не удалось восстановить путь.");
        return res;
    }

    res.totalCost = res.f[from];
    res.success   = true;
    return res;
}
