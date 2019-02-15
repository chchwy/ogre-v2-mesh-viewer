#include "stdafx.h"
#include "inspector.h"

#include <QVBoxLayout>
#include <QScrollArea>


Inspector::Inspector(QWidget* parent) : QWidget(parent)
{
    setLayout(new QVBoxLayout);

    mScrollArea = new QScrollArea;
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mScrollArea->setWidgetResizable(true);
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
