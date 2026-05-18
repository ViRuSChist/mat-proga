#include "graph_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QtMath>
#include <QFontMetricsF>
#include <cmath>

namespace {

constexpr qreal kNodeRadius     = 26.0;
constexpr qreal kPadding        = 50.0;
constexpr qreal kEdgePenWidth   = 2.4;
constexpr qreal kHighlightWidth = 5.0;
constexpr qreal kArrowSize      = 14.0;

}

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(450, 450);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(252, 252, 248));
    setAutoFillBackground(true);
    setPalette(pal);
}

void GraphWidget::setGraph(const Graph &graph)
{
    const int oldCount = m_graph.nodeCount();
    m_graph = graph;

    // перемещаем узлы только когда поменялось их количество
    if (m_graph.nodeCount() != oldCount || m_positions.size() != m_graph.nodeCount() + 1)
        layoutNodesOnCircle();

    m_highlightedPath.clear();
    update();
}

void GraphWidget::setHighlightedPath(const QVector<int> &path)
{
    m_highlightedPath = path;
    update();
}

void GraphWidget::clearHighlight()
{
    m_highlightedPath.clear();
    update();
}

void GraphWidget::resetLayout()
{
    layoutNodesOnCircle();
    update();
}

void GraphWidget::layoutNodesOnCircle()
{
    // Раскладываем узлы по окружности: узел 1 наверху, остальные
    // равномерно по часовой стрелке. Если автоматическая раскладка
    // не понравится, узлы можно переместить мышкой
    const int n = m_graph.nodeCount();
    m_positions = QVector<QPointF>(n + 1, QPointF());
    if (n <= 0)
        return;

    const qreal cx     = width()  / 2.0;
    const qreal cy     = height() / 2.0;
    const qreal radius = qMax<qreal>(60.0, qMin(width(), height()) / 2.0 - kPadding);

    for (int v = 1; v <= n; ++v) {
        const qreal angle = -M_PI / 2.0 + 2.0 * M_PI * (v - 1) / n;
        m_positions[v] = QPointF(cx + radius * qCos(angle),
                                 cy + radius * qSin(angle));
    }
}

QPointF GraphWidget::nodePosition(int node) const
{
    if (node >= 1 && node < m_positions.size())
        return m_positions[node];
    return { width() / 2.0, height() / 2.0 };
}

void GraphWidget::resizeEvent(QResizeEvent *event)
{
    if (m_positions.size() != m_graph.nodeCount() + 1)
        layoutNodesOnCircle();
    QWidget::resizeEvent(event);
}

int GraphWidget::nodeAt(const QPointF &pos) const
{
    // Проверяем, попал ли курсор в кружок какого-нибудь узла
    for (int v = 1; v <= m_graph.nodeCount(); ++v) {
        const QPointF p = nodePosition(v);
        const qreal dx = pos.x() - p.x();
        const qreal dy = pos.y() - p.y();
        if (dx * dx + dy * dy <= kNodeRadius * kNodeRadius)
            return v;
    }
    return -1;
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int v = nodeAt(event->position());
    if (v > 0) {
        m_draggingNode = v;
        // Запоминаем, в каком месте кружка пользователь нажал кнопку,
        // чтобы при перетаскивании узел не прыгал под курсор
        m_dragOffset   = event->position() - nodePosition(v);
        setCursor(Qt::ClosedHandCursor);
    }
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggingNode <= 0)
        return;

    QPointF newPos = event->position() - m_dragOffset;

    // Не даём вытащить узел за границы виджета
    newPos.setX(qBound<qreal>(kNodeRadius, newPos.x(), width()  - kNodeRadius));
    newPos.setY(qBound<qreal>(kNodeRadius, newPos.y(), height() - kNodeRadius));

    m_positions[m_draggingNode] = newPos;
    update();
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_draggingNode > 0) {
        m_draggingNode = -1;
        unsetCursor();
    }
}

bool GraphWidget::isPathEdge(int from, int to) const
{
    // Ребро from→to считается частью подсвеченного пути, если эти узлы
    // идут рядом и в правильном порядке в массиве
    for (int i = 0; i + 1 < m_highlightedPath.size(); ++i) {
        if (m_highlightedPath[i] == from && m_highlightedPath[i + 1] == to)
            return true;
    }
    return false;
}

bool GraphWidget::isPathNode(int node) const
{
    for (int v : m_highlightedPath)
        if (v == node)
            return true;
    return false;
}

void GraphWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing,     true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    if (m_graph.nodeCount() == 0) {
        p.setPen(QColor(120, 120, 115));
        p.drawText(rect(), Qt::AlignCenter,
                   "Граф пуст.\nДобавьте рёбра или загрузите файл.");
        return;
    }

    // Цвета для разных элементов рисунка.
    const QColor edgeColor      ( 60,  72,  92);  // обычная стрелка
    const QColor highlightColor (218, 105,  35);  // стрелка кратчайшего пути
    const QColor textDark       ( 22,  28,  40);

    const QColor weightFill     (255, 244, 200);  // фон подписи веса
    const QColor weightBorder   (185, 130,  20);  // рамка подписи веса
    const QColor weightTextCol  ( 90,  60,   0);

    const QColor nodeFillNormal (218, 232, 246);  // обычный узел
    const QColor nodeBorderNorm ( 65,  90, 140);
    const QColor nodeFillStart  ( 56, 121, 196);  // стартовый узел (синий)
    const QColor nodeFillEnd    ( 50, 155,  98);  // конечный узел (зелёный)
    const QColor nodeFillOnPath (250, 198, 130);  // промежуточный узел пути
    const QColor nodeBorderHi   ( 90,  50,  10);
    const QColor textOnDark     (255, 255, 255);

    QFont nodeFont = p.font();
    nodeFont.setPointSizeF(nodeFont.pointSizeF() + 2.0);
    nodeFont.setBold(true);

    QFont weightFont = p.font();
    weightFont.setPointSizeF(weightFont.pointSizeF() + 0.5);
    weightFont.setBold(true);
    QFontMetricsF wfm(weightFont);

    // Рёбра рисуем в два прохода: сначала обычные, потом подсвеченные
    // оранжевые линии кратчайшего пути в любом случае окажутся поверх серых — даже там, где линии накладываются друг на друга.
    for (int pass = 0; pass < 2; ++pass) {
        const bool drawHighlighted = (pass == 1);

        for (const Graph::Edge &e : m_graph.edges()) {
            const bool hi = isPathEdge(e.from, e.to);
            if (hi != drawHighlighted)
                continue;

            const QPointF a = nodePosition(e.from);
            const QPointF b = nodePosition(e.to);

            QLineF line(a, b);
            const qreal length = line.length();
            if (length < 2.0 * kNodeRadius + 2.0)
                continue;

            // Если рисовать стрелку прямо от центра одного узла до центра
            // другого, конец стрелки окажется внутри кружка. Поэтому сдвигаем оба конца на радиус узла вдоль направления стрелки.
            const qreal ux = (b.x() - a.x()) / length;
            const qreal uy = (b.y() - a.y()) / length;
            const QPointF startPoint(a.x() + ux * kNodeRadius,
                                     a.y() + uy * kNodeRadius);
            const QPointF tipPoint  (b.x() - ux * kNodeRadius,
                                     b.y() - uy * kNodeRadius);

            const QLineF drawLine(startPoint, tipPoint);

            QPen pen(hi ? highlightColor : edgeColor);
            pen.setWidthF(hi ? kHighlightWidth : kEdgePenWidth);
            pen.setCapStyle(Qt::RoundCap);
            p.setPen(pen);
            p.drawLine(drawLine);

            // конец стрелки
            const qreal angle  = std::atan2(-drawLine.dy(), drawLine.dx());
            const qreal sz     = hi ? kArrowSize + 2.0 : kArrowSize;
            const QPointF p1 = tipPoint + QPointF(
                -sz * std::cos(angle - M_PI / 7.0),
                 sz * std::sin(angle - M_PI / 7.0));
            const QPointF p2 = tipPoint + QPointF(
                -sz * std::cos(angle + M_PI / 7.0),
                 sz * std::sin(angle + M_PI / 7.0));

            QPainterPath arrow;
            arrow.moveTo(tipPoint);
            arrow.lineTo(p1);
            arrow.lineTo(p2);
            arrow.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(hi ? highlightColor : edgeColor);
            p.drawPath(arrow);
        }
    }

    // подписи весов. Вес рисуется на жёлтом прямоугольнике с рамкой
    p.setFont(weightFont);
    for (const Graph::Edge &e : m_graph.edges()) {
        const QPointF a = nodePosition(e.from);
        const QPointF b = nodePosition(e.to);
        QLineF line(a, b);
        if (line.length() < 1e-3)
            continue;

        const QString wText =
            (std::floor(e.weight) == e.weight)
                ? QString::number(static_cast<long long>(e.weight))
                : QString::number(e.weight, 'f', 2);

        const qreal padX = 8.0;
        const qreal padY = 3.0;
        const qreal w    = wfm.horizontalAdvance(wText) + 2 * padX;
        const qreal h    = wfm.height()                 + 2 * padY;

        // Подпись веса размещаем в середине над стрелкой
        const QPointF mid = 0.5 * (line.p1() + line.p2());
        QPointF normal(-(line.p2().y() - line.p1().y()),
                        (line.p2().x() - line.p1().x()));
        const qreal nLen = std::hypot(normal.x(), normal.y());
        if (nLen > 1e-3)
            normal = normal / nLen * 14.0;
        else
            normal = QPointF(0, -14.0);
        const QPointF center = mid + normal;

        const QRectF rc(center.x() - w / 2.0, center.y() - h / 2.0, w, h);

        const bool hi = isPathEdge(e.from, e.to);

        QPen border(hi ? highlightColor : weightBorder);
        border.setWidthF(hi ? 1.6 : 1.0);
        p.setPen(border);
        p.setBrush(hi ? QColor(255, 230, 180) : weightFill);
        p.drawRoundedRect(rc, 6, 6);

        p.setPen(hi ? QColor(120, 60, 0) : weightTextCol);
        p.drawText(rc, Qt::AlignCenter, wText);
    }

    // узлы рисуем последними, чтобы кружки лежали поверх рёбер
    p.setFont(nodeFont);
    for (int v = 1; v <= m_graph.nodeCount(); ++v) {
        const QPointF c = nodePosition(v);
        const QRectF rc(c.x() - kNodeRadius, c.y() - kNodeRadius,
                        2 * kNodeRadius,     2 * kNodeRadius);

        // Узлы стартового и конечного пути красим отдельными цветами
        // Промежуточные узлы пути красим в оранжевый
        QColor fill   = nodeFillNormal;
        QColor border = nodeBorderNorm;
        QColor txt    = textDark;
        bool   onDark = false;

        if (!m_highlightedPath.isEmpty()) {
            if (v == m_highlightedPath.first()) {
                fill = nodeFillStart;
                border = nodeBorderHi;
                onDark = true;
            } else if (v == m_highlightedPath.last()) {
                fill = nodeFillEnd;
                border = nodeBorderHi;
                onDark = true;
            } else if (isPathNode(v)) {
                fill = nodeFillOnPath;
                border = nodeBorderHi;
            }
        }

        if (onDark)
            txt = textOnDark;


        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 30));
        p.drawEllipse(rc.translated(0, 1.5));

        QPen pen(border);
        pen.setWidthF(2.0);
        p.setPen(pen);
        p.setBrush(fill);
        p.drawEllipse(rc);

        p.setPen(txt);
        p.drawText(rc, Qt::AlignCenter, QString::number(v));
    }

    // Маленькая подсказка снизу — подсказывает, что узлы можно пемерещатьь
    p.setFont(QFont(p.font().family(), 8));
    p.setPen(QColor(140, 140, 135));
    p.drawText(rect().adjusted(8, 0, -8, -6),
               Qt::AlignBottom | Qt::AlignRight,
               "Узлы можно перетаскивать мышкой");
}
