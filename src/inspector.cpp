#include "stdafx.h"
#include "inspector.h"

#include <QVBoxLayout>
#include <QScrollArea>


Inspector::Inspector(QWidget* parent) : QWidget(parent)
{
    setLayout(new QVBoxLayout);
    layout()->setSpacing(0);
    layout()->setMargin(0);

    mScrollArea = new QScrollArea;
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mScrollArea->setWidgetResizable(true);
    //mScrollArea->setBackgroundRole(QPalette::Dark);
    layout()->addWidget(mScrollArea);

    mInnerWidget = new QWidget;
    mInnerWidget->setLayout(new QVBoxLayout);
    //mInnerWidget->setStyleSheet("background-color: yellow");

    mScrollArea->setWidget(mInnerWidget);
}

void Inspector::addWidget(QWidget* w)
{
    //mScrollArea
    mInnerWidget->layout()->addWidget(w);
}

void Inspector::endAddWidget()
{
    QVBoxLayout* vBoxLayout = static_cast<QVBoxLayout*>(mInnerWidget->layout());
    vBoxLayout->addStretch();
}
