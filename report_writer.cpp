#include "report_writer.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDateTime>
#include <cmath>

bool ReportWriter::writeReport(const QString          &path,
                               const Graph            &graph,
                               int                     fromNode,
                               int                     toNode,
                               const DPSolver::Result &result,
                               QString                &errorMsg) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorMsg = QString("Не удалось открыть файл для записи: %1")
                       .arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "================================================================\n";
    out << "  Поиск кратчайшего пути методом динамического программирования\n";
    out << "  Дата: "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << '\n';
    out << "================================================================\n\n";

    out << sectionInputData(graph, fromNode, toNode);

    if (!result.success) {
        out << sectionHeader("Результат");
        out << "Не удалось найти путь.\n";
        out << "Причина: " << result.errorMessage << "\n";
        return true;
    }

    out << sectionTopoOrder(result);
    out << sectionSteps(result);
    out << sectionTable(result);
    out << sectionAnswer(result);

    return true;
}

QString ReportWriter::sectionHeader(const QString &title) const
{
    QString s;
    s += "----------------------------------------------------------------\n";
    s += "  " + title + "\n";
    s += "----------------------------------------------------------------\n";
    return s;
}

QString ReportWriter::sectionInputData(const Graph &graph,
                                       int          fromNode,
                                       int          toNode) const
{
    QString s = sectionHeader("Исходные данные");
    s += QString("Число узлов:    %1\n").arg(graph.nodeCount());
    s += QString("Число рёбер:    %1\n").arg(graph.edgeCount());
    s += QString("Начальный узел: %1\n").arg(fromNode);
    s += QString("Конечный узел:  %1\n\n").arg(toNode);

    s += "Рёбра графа (откуда → куда : стоимость):\n";
    for (const Graph::Edge &e : graph.edges()) {
        s += QString("    %1 → %2 : %3\n")
                 .arg(e.from)
                 .arg(e.to)
                 .arg(formatNumber(e.weight));
    }
    s += '\n';
    return s;
}

QString ReportWriter::sectionTopoOrder(const DPSolver::Result &result) const
{
    QString s = sectionHeader("Топологическая сортировка");
    s += "Узлы будут обрабатываться в обратном топологическом порядке,\n"
         "чтобы при вычислении f(v) значения f(u) для всех соседних узлов\n"
         "уже были известны.\n\n";

    QStringList items;
    for (int v : result.topoOrder)
        items << QString::number(v);
    s += "Топологический порядок:        " + items.join(" → ") + "\n";

    QStringList reversed;
    for (int i = result.topoOrder.size() - 1; i >= 0; --i)
        reversed << QString::number(result.topoOrder[i]);
    s += "Обратный (порядок обработки):  " + reversed.join(" → ") + "\n\n";

    return s;
}

QString ReportWriter::sectionSteps(const DPSolver::Result &result) const
{
    QString s = sectionHeader("Пошаговое вычисление f(v)");
    s += "f(v) = минимальная стоимость пути из узла v в узел "
         + QString::number(result.to) + ".\n";
    s += "Рекуррентное соотношение:  f(v) = min { w(v,u) + f(u) }\n";
    s += "Граничное условие:         f(" + QString::number(result.to) + ") = 0\n\n";

    int stepNo = 1;
    for (const DPSolver::Step &st : result.steps) {
        s += QString("Шаг %1.  Узел %2\n").arg(stepNo++).arg(st.node);

        if (st.isBase) {
            s += QString("    f(%1) = 0   [граничное условие]\n\n").arg(st.node);
            continue;
        }

        if (st.variants.isEmpty()) {
            s += "    Нет исходящих рёбер — узел тупиковый.\n";
            s += QString("    f(%1) = ∞\n\n").arg(st.node);
            continue;
        }

        s += "    Перебираем рёбра, исходящие из узла:\n";
        for (const DPSolver::Variant &v : st.variants) {
            const QString fStr = std::isfinite(v.fVia)
                                     ? formatNumber(v.fVia)
                                     : QStringLiteral("∞");
            const QString sumStr = std::isfinite(v.sum)
                                       ? formatNumber(v.sum)
                                       : QStringLiteral("∞");
            s += QString("        через узел %1:  w(%2,%1) + f(%1) = %3 + %4 = %5\n")
                     .arg(v.via)
                     .arg(st.node)
                     .arg(formatNumber(v.weight))
                     .arg(fStr)
                     .arg(sumStr);
        }

        if (std::isfinite(st.fValue)) {
            s += QString("    min = %1   (через узел %2)\n")
                     .arg(formatNumber(st.fValue))
                     .arg(st.chosenVia);
            s += QString("    f(%1) = %2,   next(%1) = %3\n\n")
                     .arg(st.node)
                     .arg(formatNumber(st.fValue))
                     .arg(st.chosenVia);
        } else {
            s += QString("    Все варианты ведут в недостижимые узлы — f(%1) = ∞\n\n")
                     .arg(st.node);
        }
    }
    return s;
}

QString ReportWriter::sectionTable(const DPSolver::Result &result) const
{
    QString s = sectionHeader("Итоговая таблица состояний");
    s += " v |    f(v)    | next(v)\n";
    s += "---+------------+--------\n";

    // f[0] не используется (узлы нумеруются с 1), начинаем с v = 1.
    for (int v = 1; v < result.f.size(); ++v) {
        const QString fStr = std::isfinite(result.f[v])
                                 ? formatNumber(result.f[v])
                                 : QStringLiteral("∞");
        const QString nxtStr = (result.next[v] >= 0)
                                   ? QString::number(result.next[v])
                                   : QStringLiteral("—");
        s += QString("%1 | %2 | %3\n")
                 .arg(v, 2)
                 .arg(fStr, 10)
                 .arg(nxtStr, 6);
    }
    s += '\n';
    return s;
}

QString ReportWriter::sectionAnswer(const DPSolver::Result &result) const
{
    QString s = sectionHeader("Ответ");

    QStringList nodes;
    for (int v : result.path)
        nodes << QString::number(v);
    s += "Кратчайший путь:        " + nodes.join(" → ") + "\n";
    s += QString("Минимальная стоимость:  %1\n")
             .arg(formatNumber(result.totalCost));
    return s;
}

QString ReportWriter::formatNumber(double v) const
{

    if (std::isfinite(v) && std::floor(v) == v && std::abs(v) < 1e15)
        return QString::number(static_cast<long long>(v));
    return QString::number(v, 'f', 3);
}
