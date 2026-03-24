#pragma once

#include <QWidget>
#include <QPainter>
#include <vector>
#include "../piece.h"
#include "../vector.h"
#include "../geometry.h"

class PackingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PackingWidget(QWidget *parent = nullptr);

public slots:
    void updateLayout(const MyNest::box_t &bin,
                      const std::vector<MyNest::Piece> &pieces,
                      const std::vector<MyNest::Vector> &vectors,
                      double utilization);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    MyNest::box_t m_bin;
    std::vector<MyNest::Piece> m_pieces;
    std::vector<MyNest::Vector> m_vectors;
    double m_utilization = 0.0;
    bool m_hasData = false;

    // 预定义颜色表
    static const QColor s_colors[];
};
