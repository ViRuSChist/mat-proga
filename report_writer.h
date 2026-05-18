#pragma once

// Сохранение хода решения в .txt: исходные данные, топологический
// порядок, все шаги вычисления f(v) и сам кратчайший путь.

#include "graph.h"
#include "dp_solver.h"
#include <QString>

class ReportWriter
{
public:
    bool writeReport(const QString          &path,
                     const Graph            &graph,
                     int                     fromNode,
                     int                     toNode,
                     const DPSolver::Result &result,
                     QString                &errorMsg) const;

private:
    QString sectionHeader(const QString &title) const;
    QString sectionInputData(const Graph &graph, int fromNode, int toNode) const;
    QString sectionTopoOrder(const DPSolver::Result &result) const;
    QString sectionSteps(const DPSolver::Result &result) const;
    QString sectionTable(const DPSolver::Result &result) const;
    QString sectionAnswer(const DPSolver::Result &result) const;

    QString formatNumber(double v) const;
};

