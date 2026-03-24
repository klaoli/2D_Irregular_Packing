#include "PackingWidget.h"
#include "../geometry.h"
#include <QPainterPath>
#include <QFontMetrics>

const QColor PackingWidget::s_colors[] = {
    QColor(231, 76, 60),   QColor(46, 204, 113),  QColor(52, 152, 219),
    QColor(155, 89, 182),  QColor(241, 196, 15),  QColor(230, 126, 34),
    QColor(26, 188, 156),  QColor(52, 73, 94),    QColor(192, 57, 43),
    QColor(39, 174, 96),   QColor(41, 128, 185),  QColor(142, 68, 173),
    QColor(243, 156, 18),  QColor(211, 84, 0),    QColor(22, 160, 133),
    QColor(44, 62, 80)
};

PackingWidget::PackingWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(600, 400);
    setStyleSheet("background-color: #1a1a2e;");
}

void PackingWidget::updateLayout(const MyNest::box_t &bin,
                                  const std::vector<MyNest::Piece> &pieces,
                                  const std::vector<MyNest::Vector> &vectors,
                                  double utilization)
{
    m_bin = bin;
    m_pieces = pieces;
    m_vectors = vectors;
    m_utilization = utilization;
    m_hasData = true;
    update(); // 触发重绘
}

void PackingWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_hasData) {
        painter.setPen(QColor(200, 200, 200));
        QFont font("Microsoft YaHei", 16);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, "等待排样数据...");
        return;
    }

    // 计算缩放和偏移，使板材居中显示
    double binW = m_bin.max_corner().x() - m_bin.min_corner().x();
    double binH = m_bin.max_corner().y() - m_bin.min_corner().y();
    if (binW < 1e-6 || binH < 1e-6) return;

    double margin = 40.0;
    double scaleX = (width() - 2 * margin) / binW;
    double scaleY = (height() - 2 * margin) / binH;
    double scale = std::min(scaleX, scaleY);

    double offsetX = margin + (width() - 2 * margin - binW * scale) / 2.0;
    double offsetY = margin + (height() - 2 * margin - binH * scale) / 2.0;

    // 绘制板材矩形
    QRectF binRect(offsetX, offsetY, binW * scale, binH * scale);
    painter.setPen(QPen(QColor(0, 255, 136), 2));
    painter.setBrush(QColor(16, 29, 44));
    painter.drawRect(binRect);

    // 绘制每个零件
    MyNest::Geometry *geo = MyNest::Geometry::getInstance();
    int numColors = sizeof(s_colors) / sizeof(s_colors[0]);

    for (size_t i = 0; i < m_pieces.size(); ++i) {
        auto poly = geo->translate(m_pieces[i].polygon, m_vectors[i].x, m_vectors[i].y);
        const auto &outer = poly.outer();
        if (outer.size() < 3) continue;

        QPainterPath path;
        double px = (outer[0].x() - m_bin.min_corner().x()) * scale + offsetX;
        double py = (outer[0].y() - m_bin.min_corner().y()) * scale + offsetY;
        path.moveTo(px, py);

        for (size_t j = 1; j < outer.size(); ++j) {
            px = (outer[j].x() - m_bin.min_corner().x()) * scale + offsetX;
            py = (outer[j].y() - m_bin.min_corner().y()) * scale + offsetY;
            path.lineTo(px, py);
        }
        path.closeSubpath();

        QColor fillColor = s_colors[i % numColors];
        fillColor.setAlpha(180);
        painter.setBrush(fillColor);
        painter.setPen(QPen(Qt::white, 0.5));
        painter.drawPath(path);
    }

    // 绘制利用率文字
    painter.setPen(QColor(0, 255, 136));
    QFont font("Consolas", 14, QFont::Bold);
    painter.setFont(font);
    QString utilText = QString("利用率: %1%").arg(m_utilization * 100.0, 0, 'f', 2);
    painter.drawText(QRectF(offsetX, offsetY - 30, binW * scale, 25), Qt::AlignRight, utilText);
}
