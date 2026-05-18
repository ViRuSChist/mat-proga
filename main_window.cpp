#include "main_window.h"
#include "graph_widget.h"
#include "graph_io.h"
#include "graph_validator.h"
#include "report_writer.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QDir>
#include <QFile>
#include <QSignalBlocker>
#include <QStringList>
#include <cmath>

namespace {

QString prettyNumber(double v)
{
    if (std::isfinite(v) && std::floor(v) == v && std::abs(v) < 1e15)
        return QString::number(static_cast<long long>(v));
    return QString::number(v, 'f', 3);
}

// Если пользователь в диалоге сохранения ввёл имя без расширения добавляем нужное расширение
QString ensureExtension(const QString &path, const QString &ext)
{
    if (path.isEmpty())
        return path;
    if (QFileInfo(path).suffix().compare(ext, Qt::CaseInsensitive) == 0)
        return path;
    return path + '.' + ext;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Поиск кратчайшего пути методом ДП");
    resize(1200, 760);

    buildUi();
    applyDemoGraph();
}

void MainWindow::buildUi()
{
    // Левая колонка — настройки графа и таблица рёбер.
    QGroupBox *paramsBox = new QGroupBox("Параметры задачи");
    QFormLayout *paramsLayout = new QFormLayout(paramsBox);

    m_nodeCountSpin = new QSpinBox;
    m_nodeCountSpin->setRange(2, 100);
    m_nodeCountSpin->setValue(5);

    m_fromSpin = new QSpinBox;
    m_fromSpin->setRange(1, 100);
    m_fromSpin->setValue(1);

    m_toSpin = new QSpinBox;
    m_toSpin->setRange(1, 100);
    m_toSpin->setValue(5);

    paramsLayout->addRow("Число узлов n:",   m_nodeCountSpin);
    paramsLayout->addRow("Начальный узел:",  m_fromSpin);
    paramsLayout->addRow("Конечный узел:",   m_toSpin);

    QLabel *dagHint = new QLabel(
        "Граф должен быть ациклическим: метод ДП "
        "не применим к графам с циклами.");
    dagHint->setStyleSheet("color: #777; font-style: italic;");
    dagHint->setWordWrap(true);
    paramsLayout->addRow(dagHint);

    QGroupBox *addEdgeBox = new QGroupBox("Добавление ребра");
    QGridLayout *addEdgeLayout = new QGridLayout(addEdgeBox);

    m_edgeFromSpin = new QSpinBox;
    m_edgeFromSpin->setRange(1, 100);
    m_edgeFromSpin->setValue(1);

    m_edgeToSpin = new QSpinBox;
    m_edgeToSpin->setRange(1, 100);
    m_edgeToSpin->setValue(2);

    m_edgeWeightSpin = new QDoubleSpinBox;
    m_edgeWeightSpin->setRange(0.0, 1e9);
    m_edgeWeightSpin->setDecimals(3);
    m_edgeWeightSpin->setValue(1.0);

    m_addEdgeBtn    = new QPushButton("Добавить ребро");
    m_removeEdgeBtn = new QPushButton("Удалить выделенное");

    addEdgeLayout->addWidget(new QLabel("Из узла:"),   0, 0);
    addEdgeLayout->addWidget(m_edgeFromSpin,           0, 1);
    addEdgeLayout->addWidget(new QLabel("В узел:"),    1, 0);
    addEdgeLayout->addWidget(m_edgeToSpin,             1, 1);
    addEdgeLayout->addWidget(new QLabel("Вес:"),       2, 0);
    addEdgeLayout->addWidget(m_edgeWeightSpin,         2, 1);
    addEdgeLayout->addWidget(m_addEdgeBtn,             3, 0, 1, 2);
    addEdgeLayout->addWidget(m_removeEdgeBtn,          4, 0, 1, 2);

    m_edgesTable = new QTableWidget(0, 3);
    m_edgesTable->setHorizontalHeaderLabels({ "Из", "В", "Вес" });
    m_edgesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_edgesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_edgesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_loadBtn       = new QPushButton("Загрузить из файла…");
    m_saveGraphBtn  = new QPushButton("Сохранить граф…");
    m_clearBtn      = new QPushButton("Очистить граф");
    m_solveBtn      = new QPushButton("Найти кратчайший путь");
    m_solveBtn->setStyleSheet("font-weight: bold;");
    m_saveReportBtn = new QPushButton("Сохранить ход решения в .txt…");
    m_saveReportBtn->setEnabled(false);
    m_resetLayoutBtn = new QPushButton("Сбросить раскладку узлов");

    QVBoxLayout *fileBtnLayout = new QVBoxLayout;
    fileBtnLayout->addWidget(m_loadBtn);
    fileBtnLayout->addWidget(m_saveGraphBtn);
    fileBtnLayout->addWidget(m_clearBtn);

    QVBoxLayout *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(paramsBox);
    leftColumn->addWidget(addEdgeBox);
    leftColumn->addWidget(new QLabel("Список рёбер:"));
    leftColumn->addWidget(m_edgesTable, 1);
    leftColumn->addLayout(fileBtnLayout);
    leftColumn->addWidget(m_resetLayoutBtn);
    leftColumn->addWidget(m_solveBtn);
    leftColumn->addWidget(m_saveReportBtn);

    QWidget *leftWidget = new QWidget;
    leftWidget->setLayout(leftColumn);
    leftWidget->setMaximumWidth(360);

    // Правая колонка — рисунок графа и результаты решения.
    m_graphWidget = new GraphWidget;

    m_resultLabel = new QLabel("Решение не запускалось.");
    m_resultLabel->setStyleSheet("font-size: 14px;");
    m_resultLabel->setWordWrap(true);

    m_logView = new QPlainTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText("Здесь будут показаны вычисления после нажатия «Найти кратчайший путь».");
    QFont mono("Courier New");
    mono.setStyleHint(QFont::Monospace);
    m_logView->setFont(mono);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #555;");
    m_statusLabel->setWordWrap(true);

    QVBoxLayout *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(m_graphWidget, 3);
    rightColumn->addWidget(m_resultLabel);
    rightColumn->addWidget(new QLabel("Ход решения:"));
    rightColumn->addWidget(m_logView, 2);
    rightColumn->addWidget(m_statusLabel);

    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(rightColumn);

    QHBoxLayout *root = new QHBoxLayout;
    root->addWidget(leftWidget);
    root->addWidget(rightWidget, 1);

    QWidget *central = new QWidget;
    central->setLayout(root);
    setCentralWidget(central);

    connect(m_nodeCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onNodeCountChanged);
    connect(m_addEdgeBtn,    &QPushButton::clicked, this, &MainWindow::onAddEdge);
    connect(m_removeEdgeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedEdge);
    connect(m_clearBtn,      &QPushButton::clicked, this, &MainWindow::onClearGraph);
    connect(m_loadBtn,       &QPushButton::clicked, this, &MainWindow::onLoadFromFile);
    connect(m_saveGraphBtn,  &QPushButton::clicked, this, &MainWindow::onSaveGraph);
    connect(m_solveBtn,      &QPushButton::clicked, this, &MainWindow::onSolve);
    connect(m_saveReportBtn, &QPushButton::clicked, this, &MainWindow::onSaveReport);
    connect(m_resetLayoutBtn, &QPushButton::clicked, this,
            [this]() { m_graphWidget->resetLayout(); });
}

void MainWindow::applyDemoGraph()
{
    // Демо-граф из 8 узлов, который грузится при старте программы
    m_graph.clear();
    m_graph.setNodeCount(8);

    m_graph.addEdge(1, 2, 5);
    m_graph.addEdge(1, 3, 3);

    m_graph.addEdge(2, 4, 6);
    m_graph.addEdge(2, 5, 2);
    m_graph.addEdge(3, 4, 4);
    m_graph.addEdge(3, 5, 9);

    m_graph.addEdge(4, 6, 3);
    m_graph.addEdge(4, 7, 7);
    m_graph.addEdge(5, 6, 5);
    m_graph.addEdge(5, 7, 1);

    m_graph.addEdge(6, 8, 4);
    m_graph.addEdge(7, 8, 3);


    m_graph.addEdge(2, 8, 20);
    m_graph.addEdge(3, 7, 8);

    {
        const QSignalBlocker b1(m_nodeCountSpin);
        m_nodeCountSpin->setValue(m_graph.nodeCount());
    }
    m_fromSpin->setValue(1);
    m_toSpin->setValue(8);

    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult("Загружен демонстрационный граф (8 узлов). Нажмите «Найти кратчайший путь».");
}

void MainWindow::rebuildEdgeTable()
{
    const QSignalBlocker blocker(m_edgesTable);
    m_edgesTable->setRowCount(0);

    for (const Graph::Edge &e : m_graph.edges()) {
        const int row = m_edgesTable->rowCount();
        m_edgesTable->insertRow(row);
        m_edgesTable->setItem(row, 0, new QTableWidgetItem(QString::number(e.from)));
        m_edgesTable->setItem(row, 1, new QTableWidgetItem(QString::number(e.to)));
        m_edgesTable->setItem(row, 2, new QTableWidgetItem(prettyNumber(e.weight)));
    }
}

void MainWindow::resetResult(const QString &statusText)
{
    m_hasResult  = false;
    m_lastResult = DPSolver::Result();
    m_resultLabel->setText("Решение не запускалось.");
    m_logView->clear();
    m_graphWidget->clearHighlight();
    m_saveReportBtn->setEnabled(false);
    m_statusLabel->setText(statusText);
}

void MainWindow::onNodeCountChanged(int n)
{
    m_graph.setNodeCount(n);

    // Подстраиваем максимальные значения у полей выбора узлов
    // пользователь не должен иметь возможность выбрать номер узла больше n
    m_fromSpin    ->setMaximum(n);
    m_toSpin      ->setMaximum(n);
    m_edgeFromSpin->setMaximum(n);
    m_edgeToSpin  ->setMaximum(n);

    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult("Изменено число узлов.");
}

void MainWindow::onAddEdge()
{
    const int    from   = m_edgeFromSpin->value();
    const int    to     = m_edgeToSpin->value();
    const double weight = m_edgeWeightSpin->value();

    QString error;
    if (!m_graph.addEdge(from, to, weight, &error)) {
        QMessageBox::warning(this, "Не удалось добавить ребро", error);
        return;
    }
    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult(QString("Добавлено ребро %1 → %2 (вес %3).")
                    .arg(from).arg(to).arg(prettyNumber(weight)));
}

void MainWindow::onRemoveSelectedEdge()
{
    const int row = m_edgesTable->currentRow();
    if (row < 0 || row >= m_graph.edgeCount()) {
        QMessageBox::information(this, "Удаление ребра",
                                 "Сначала выделите строку в таблице рёбер.");
        return;
    }

    const Graph::Edge e = m_graph.edges().at(row);
    m_graph.removeEdge(e.from, e.to);

    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult(QString("Удалено ребро %1 → %2.").arg(e.from).arg(e.to));
}

void MainWindow::onClearGraph()
{
    const int n = m_nodeCountSpin->value();
    m_graph.clear();
    m_graph.setNodeCount(n);
    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult("Граф очищен.");
}

void MainWindow::onLoadFromFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Открыть файл с графом", QString(),
        "Текстовые файлы (*.txt);;Все файлы (*)");
    if (path.isEmpty())
        return;

    Graph loaded;
    QString err;
    if (!GraphIO::loadFromFile(path, loaded, err)) {
        QMessageBox::critical(this, "Ошибка чтения файла", err);
        return;
    }

    m_graph = loaded;

    {
        const QSignalBlocker b(m_nodeCountSpin);
        m_nodeCountSpin->setValue(m_graph.nodeCount());
    }
    m_fromSpin    ->setMaximum(m_graph.nodeCount());
    m_toSpin      ->setMaximum(m_graph.nodeCount());
    m_edgeFromSpin->setMaximum(m_graph.nodeCount());
    m_edgeToSpin  ->setMaximum(m_graph.nodeCount());

    rebuildEdgeTable();
    m_graphWidget->setGraph(m_graph);
    resetResult(QString("Загружен граф из файла: %1").arg(path));
}

void MainWindow::onSaveGraph()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить граф в файл", "graph.txt",
        "Текстовые файлы (*.txt)");
    if (path.isEmpty())
        return;

    path = ensureExtension(path, "txt");

    QString err;
    if (!GraphIO::saveToFile(path, m_graph, err)) {
        QMessageBox::critical(this, "Ошибка сохранения", err);
        return;
    }
    m_statusLabel->setText(QString("Граф сохранён: %1").arg(path));
}

void MainWindow::onSolve()
{
    const int from = m_fromSpin->value();
    const int to   = m_toSpin->value();

    // Валидатор находит больше проблем, чем сам алгоритм (отрицательные веса, тупики), и собирает их сразу все
    const GraphValidator::Result vres = GraphValidator::validate(m_graph, from, to);
    if (vres.hasErrors()) {
        QMessageBox::warning(this, "Граф некорректен", vres.summary());
        m_statusLabel->setText("Не запущено: в графе есть ошибки.");
        return;
    }

    if (!vres.issues.isEmpty())
        m_statusLabel->setText(vres.summary());
    else
        m_statusLabel->setText("Граф корректен.");

    DPSolver solver;
    m_lastResult = solver.solve(m_graph, from, to);

    if (!m_lastResult.success) {
        QMessageBox::warning(this, "Решение не получено", m_lastResult.errorMessage);
        m_resultLabel->setText("Не удалось найти путь: " + m_lastResult.errorMessage);
        m_graphWidget->clearHighlight();
        m_logView->clear();
        m_saveReportBtn->setEnabled(false);
        m_hasResult = false;
        return;
    }

    m_hasResult = true;
    m_saveReportBtn->setEnabled(true);

    QStringList nodes;
    for (int v : m_lastResult.path)
        nodes << QString::number(v);
    m_resultLabel->setText(QString("Кратчайший путь: %1\nМинимальная стоимость: %2")
                               .arg(nodes.join(" → "))
                               .arg(prettyNumber(m_lastResult.totalCost)));

    m_graphWidget->setHighlightedPath(m_lastResult.path);

    // В окне показываем тот же текст, что попадёт в .txt-файл
    ReportWriter rw;
    QString err;
    const QString tempPath = QDir::tempPath() + "/__shortest_path_preview.txt";
    if (rw.writeReport(tempPath, m_graph, from, to, m_lastResult, err)) {
        QFile f(tempPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            m_logView->setPlainText(QString::fromUtf8(f.readAll()));
    }
}

void MainWindow::onSaveReport()
{
    if (!m_hasResult) {
        QMessageBox::information(this, "Сохранение в файл",
                                 "Сначала нажмите «Найти кратчайший путь».");
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить ход решения в файл", "shortest_path.txt",
        "Текстовые файлы (*.txt)");
    if (path.isEmpty())
        return;

    path = ensureExtension(path, "txt");

    ReportWriter rw;
    QString err;
    if (!rw.writeReport(path, m_graph, m_fromSpin->value(),
                        m_toSpin->value(), m_lastResult, err)) {
        QMessageBox::critical(this, "Ошибка сохранения", err);
        return;
    }
    m_statusLabel->setText(QString("Файл сохранён: %1").arg(path));
}
