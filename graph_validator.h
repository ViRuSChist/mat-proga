#pragma once

// Проверка графа перед запуском алгоритма. Метод ДП работает только на графах без циклов
#include "graph.h"
#include <QVector>
#include <QString>

class GraphValidator
{
public:
    enum class Severity {
        Error,    // критическая проблема, алгоритм не запускается
        Warning   // некритическая проблема, на которую стоит обратить внимание
    };

    struct Issue
    {
        Severity severity;
        QString  message;
    };

    struct Result
    {
        QVector<Issue> issues;

        bool hasErrors() const;
        QString summary() const;
    };

    static Result validate(const Graph &graph, int from, int to);

private:
    static void checkBasics(const Graph &graph, int from, int to, Result &out);
    static void checkNegativeWeights(const Graph &graph, Result &out);
    static void checkCycles(const Graph &graph, Result &out);
    static void checkReachability(const Graph &graph, int from, int to, Result &out);
    static void checkDeadEnds(const Graph &graph, int target, Result &out);
};

