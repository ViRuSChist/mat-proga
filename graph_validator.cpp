#include "graph_validator.h"
#include <QQueue>
#include <QSet>
#include <QStringList>

bool GraphValidator::Result::hasErrors() const
{
    for (const Issue &i : issues) {
        if (i.severity == Severity::Error)
            return true;
    }
    return false;
}

QString GraphValidator::Result::summary() const
{
    if (issues.isEmpty())
        return QString("Граф корректен.");

    QStringList lines;
    for (const Issue &i : issues) {
        const QString tag = (i.severity == Severity::Error)
                                ? QStringLiteral("[ОШИБКА] ")
                                : QStringLiteral("[ПРЕДУПРЕЖДЕНИЕ] ");
        lines << tag + i.message;
    }
    return lines.join('\n');
}

GraphValidator::Result GraphValidator::validate(const Graph &graph, int from, int to)
{
    Result result;

    // Если уже на самых базовых вещах что-то не так (нет рёбер, плохие
    // номера узлов и т.д.) — нет смысла гонять остальные проверки.
    checkBasics(graph, from, to, result);
    if (result.hasErrors())
        return result;

    checkNegativeWeights(graph, result);
    checkCycles(graph, result);

    // На циклическом графе проверка достижимости и тупиков теряет смысл —
    // - метод ДП всё равно не запустится. Останавливаемся.
    if (result.hasErrors())
        return result;

    checkReachability(graph, from, to, result);
    checkDeadEnds(graph, to, result);

    return result;
}

void GraphValidator::checkBasics(const Graph &graph, int from, int to, Result &out)
{
    if (graph.nodeCount() < 2) {
        out.issues.append({ Severity::Error,
                            QStringLiteral("В графе должно быть минимум 2 узла.") });
        return;
    }

    if (!graph.isValidNode(from))
        out.issues.append({ Severity::Error,
                            QString("Начальный узел %1 не существует.").arg(from) });

    if (!graph.isValidNode(to))
        out.issues.append({ Severity::Error,
                            QString("Конечный узел %1 не существует.").arg(to) });

    if (graph.isValidNode(from) && graph.isValidNode(to) && from == to)
        out.issues.append({ Severity::Error,
                            QStringLiteral("Начальный и конечный узлы совпадают.") });

    if (graph.edgeCount() == 0)
        out.issues.append({ Severity::Error,
                            QStringLiteral("В графе нет ни одного ребра.") });
}

void GraphValidator::checkNegativeWeights(const Graph &graph, Result &out)
{
    // По смыслу задачи вес ребра — стоимость проезда. Минусовых цен не бывает
    for (const Graph::Edge &e : graph.edges()) {
        if (e.weight < 0.0) {
            out.issues.append({
                Severity::Error,
                QString("Ребро %1 → %2 имеет отрицательный вес %3.")
                    .arg(e.from).arg(e.to).arg(e.weight)
            });
        }
    }
}

void GraphValidator::checkCycles(const Graph &graph, Result &out)
{
    // Ищем циклы алгоритмом Кана. Для каждого узла считаем, сколько в него
    // входит стрелок. Узлы с нулём кладём в очередь и убираем:
    // когда убираем узел, у его соседних узлов счётчик уменьшается на 1,
    // и если счётчик дошёл до нуля — кладём соседний узел в очередь.
    // Если в конце прошли по всем узлам — цикла нет. Если остались
    // необработанные — там цикл.
    const int n = graph.nodeCount();
    QVector<int> indeg(n + 1, 0);

    for (const Graph::Edge &e : graph.edges())
        ++indeg[e.to];

    QQueue<int> queue;
    for (int v = 1; v <= n; ++v) {
        if (indeg[v] == 0)
            queue.enqueue(v);
    }

    int processed = 0;
    while (!queue.isEmpty()) {
        const int v = queue.dequeue();
        ++processed;
        for (const Graph::Edge &e : graph.outgoingEdges(v)) {
            if (--indeg[e.to] == 0)
                queue.enqueue(e.to);
        }
    }

    if (processed < n) {
        out.issues.append({
            Severity::Error,
            QStringLiteral("В графе обнаружен цикл. Метод динамического "
                           "программирования применим только к ациклическим "
                           "графам (DAG).")
        });
    }
}

void GraphValidator::checkReachability(const Graph &graph, int from, int to, Result &out)
{
    // Проверяем, можно ли вообще дойти от стартового узла до конечного.
    // Кладём стартовый узел в очередь, по очереди берём из неё узлы и
    // добавляем их непосещённые соседние узлы. Если в какой-то момент
    // конечный узел оказался посещённым — путь существует. Если очередь
    // опустела, а конечный узел мы так и не увидели — пути нет.
    QSet<int> visited;
    QQueue<int> queue;
    queue.enqueue(from);
    visited.insert(from);

    while (!queue.isEmpty()) {
        const int v = queue.dequeue();
        for (const Graph::Edge &e : graph.outgoingEdges(v)) {
            if (!visited.contains(e.to)) {
                visited.insert(e.to);
                queue.enqueue(e.to);
            }
        }
    }

    if (!visited.contains(to)) {
        out.issues.append({
            Severity::Error,
            QString("Из узла %1 невозможно добраться до узла %2.").arg(from).arg(to)
        });
    }
}

void GraphValidator::checkDeadEnds(const Graph &graph, int target, Result &out)
{
    // Тупик — узел, из которого не выходит ни одной стрелки. Такой узел
    // не может быть промежуточным в пути. Сам конечный узел тупиком
    // не считаем — у него исходящих стрелок и не может быть
    QStringList deadEnds;
    for (int v = 1; v <= graph.nodeCount(); ++v) {
        if (v == target)
            continue;
        if (graph.outgoingEdges(v).isEmpty())
            deadEnds.append(QString::number(v));
    }

    if (!deadEnds.isEmpty()) {
        out.issues.append({
            Severity::Warning,
            QString("Тупиковые узлы (нет исходящих рёбер): %1. "
                    "Они не могут быть промежуточными узлами пути.")
                .arg(deadEnds.join(QStringLiteral(", ")))
        });
    }
}
