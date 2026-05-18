#include "graph_io.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>

namespace {

// Если в строке есть '#' — это комментарий до конца строки, отрезаем.
// Заодно убираем пробелы по краям.
QString cleanLine(const QString &raw)
{
    QString s = raw;
    const int hashPos = s.indexOf('#');
    if (hashPos >= 0)
        s = s.left(hashPos);
    return s.trimmed();
}

QStringList splitTokens(const QString &line)
{
    static const QRegularExpression re("\\s+");
    return line.split(re, Qt::SkipEmptyParts);
}

}

bool GraphIO::loadFromFile(const QString &path, Graph &outGraph, QString &errorMsg)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMsg = QString("Не удалось открыть файл: %1").arg(file.errorString());
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    Graph graph;
    int   nodeCount  = -1;
    int   lineNumber = 0;

    while (!in.atEnd()) {
        ++lineNumber;
        const QString line = cleanLine(in.readLine());
        if (line.isEmpty())
            continue;

        const QStringList tokens = splitTokens(line);

        if (nodeCount < 0) {
            // Самая первая значимая строка в файле — это число узлов.
            if (tokens.size() != 1) {
                errorMsg = QString("Строка %1: ожидалось одно число (количество узлов).")
                               .arg(lineNumber);
                return false;
            }
            bool ok = false;
            nodeCount = tokens[0].toInt(&ok);
            if (!ok || nodeCount < 2) {
                errorMsg = QString("Строка %1: некорректное количество узлов (%2).")
                               .arg(lineNumber).arg(tokens[0]);
                return false;
            }
            graph.setNodeCount(nodeCount);
            continue;
        }

        // Остальные строки — рёбра: три числа подряд (откуда, куда, вес).
        if (tokens.size() != 3) {
            errorMsg = QString("Строка %1: ожидалось три числа (откуда, куда, вес).")
                           .arg(lineNumber);
            return false;
        }

        bool ok1 = false, ok2 = false, ok3 = false;
        const int    from   = tokens[0].toInt(&ok1);
        const int    to     = tokens[1].toInt(&ok2);
        const double weight = tokens[2].toDouble(&ok3);
        if (!ok1 || !ok2 || !ok3) {
            errorMsg = QString("Строка %1: не удалось прочитать числа.").arg(lineNumber);
            return false;
        }

        QString edgeError;
        if (!graph.addEdge(from, to, weight, &edgeError)) {
            errorMsg = QString("Строка %1: %2").arg(lineNumber).arg(edgeError);
            return false;
        }
    }

    if (nodeCount < 0) {
        errorMsg = QStringLiteral("Файл пуст или не содержит данных о графе.");
        return false;
    }

    outGraph = graph;
    return true;
}

bool GraphIO::saveToFile(const QString &path, const Graph &graph, QString &errorMsg)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorMsg = QString("Не удалось открыть файл для записи: %1")
                       .arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# Транспортная сеть для задачи поиска кратчайшего пути.\n";
    out << "# Первая строка — число узлов, далее рёбра: откуда, куда, вес.\n";
    out << graph.nodeCount() << '\n';

    for (const Graph::Edge &e : graph.edges())
        out << e.from << ' ' << e.to << ' ' << e.weight << '\n';

    return true;
}
