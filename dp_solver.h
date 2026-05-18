#pragma once

// Поиск кратчайшего пути методом динамического программирования.
// Применим только к ациклическим графам

#include "graph.h"
#include <QVector>
#include <QString>

class DPSolver
{
public:
    // Когда считаем f(v), перебираем все стрелки из v. Каждый такой
    // вариант "пойти через соседний узел" — это Variant: куда мы шагнём,
    // сколько стоит шаг и какая полная сумма получается.
    struct Variant
    {
        int    via;     // соседний узел, в который ведёт стрелка
        double weight;  // стоимость шага v → via
        double fVia;    // ранее посчитанное f(via)
        double sum;     // weight + fVia — полная стоимость через этот соседний узел
    };

    // Шаг алгоритма — обработка одного узла.
    struct Step
    {
        int              node;       // обрабатываемый узел
        QVector<Variant> variants;   // все варианты, которые перебирали
        int              chosenVia;  // выбранный соседний узел (-1 для конечного узла)
        double           fValue;     // итоговое f(v)
        bool             isBase;     // true для конечного узла: f = 0 по определению
    };

    struct Result
    {
        bool            success     = false;
        QString         errorMessage;

        int             from        = 0;
        int             to          = 0;

        double          totalCost   = 0.0;
        QVector<int>    path;       // последовательность узлов от from до to

        QVector<int>    topoOrder;  // топологический порядок узлов
        QVector<double> f;          // f[v] — минимальная стоимость пути из v в to
        QVector<int>    next;       // next[v] — следующий узел после v на кратчайшем пути
        QVector<Step>   steps;      // подробный лог по каждому шагу
    };

    Result solve(const Graph &graph, int from, int to) const;

private:
    QVector<int> topologicalSort(const Graph &graph) const;
    QVector<int> reconstructPath(const QVector<int> &next, int from, int to) const;
};

