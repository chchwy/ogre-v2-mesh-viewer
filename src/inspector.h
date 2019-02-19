#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QWidget>

class QScrollArea;


class Inspector : public QWidget
{
    Q_OBJECT
public:
    Inspector(QWidget* parent);

    void addWidget(QWidget* w);
    void endAddWidget();

private:
    QScrollArea* mScrollArea = nullptr;
    QWidget* mInnerWidget = nullptr;
};

#endif // INSPECTOR_H
