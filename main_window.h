#pragma once

// Главное окно. только показывает граф,
// принимает действия пользователя и вызывает нужные классы:
// GraphValidator для проверок, DPSolver для поиска пути,
// ReportWriter для сохранения хода решения в .txt.

#include <QMainWindow>
#include <QString>
#include "graph.h"
#include "dp_solver.h"

class QTableWidget;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class GraphWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onNodeCountChanged(int n);
    void onAddEdge();
    void onRemoveSelectedEdge();
    void onClearGraph();
    void onLoadFromFile();
    void onSaveGraph();
    void onSolve();
    void onSaveReport();

private:
    void buildUi();
    void rebuildEdgeTable();
    void resetResult(const QString &statusText = QString());
    void applyDemoGraph();

    Graph            m_graph;
    DPSolver::Result m_lastResult;
    bool             m_hasResult = false;

    GraphWidget    *m_graphWidget    = nullptr;
    QTableWidget   *m_edgesTable     = nullptr;

    QSpinBox       *m_nodeCountSpin  = nullptr;
    QSpinBox       *m_fromSpin       = nullptr;
    QSpinBox       *m_toSpin         = nullptr;

    QSpinBox       *m_edgeFromSpin   = nullptr;
    QSpinBox       *m_edgeToSpin     = nullptr;
    QDoubleSpinBox *m_edgeWeightSpin = nullptr;

    QLabel         *m_resultLabel    = nullptr;
    QLabel         *m_statusLabel    = nullptr;
    QPlainTextEdit *m_logView        = nullptr;

    QPushButton    *m_addEdgeBtn     = nullptr;
    QPushButton    *m_removeEdgeBtn  = nullptr;
    QPushButton    *m_clearBtn       = nullptr;
    QPushButton    *m_loadBtn        = nullptr;
    QPushButton    *m_saveGraphBtn   = nullptr;
    QPushButton    *m_solveBtn       = nullptr;
    QPushButton    *m_saveReportBtn  = nullptr;
    QPushButton    *m_resetLayoutBtn = nullptr;
};

