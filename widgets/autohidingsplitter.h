/*
 * Cantata
 *
 * Copyright (c) 2011-2022 Craig Drummond <craig.p.drummond@gmail.com>
 * This file (c) 2012 Piotr Wicijowski <piotr.wicijowski@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef AUTOHIDINGSPLITTER_H
#define AUTOHIDINGSPLITTER_H

#include "config.h"
#include "thinsplitterhandle.h"
#include <QList>
#include <QQueue>
#include <QVariantAnimation>
#include <QSet>
#include <QAbstractItemView>

Q_DECLARE_METATYPE(QList<int>)

class SplitterSizeAnimation;

class AutohidingSplitterHandle : public ThinSplitterHandle
{
    Q_OBJECT

public:
    AutohidingSplitterHandle(Qt::Orientation orientation, QSplitter *parent) : ThinSplitterHandle(orientation, parent) { }
    ~AutohidingSplitterHandle() override { }

    QSize sizeHint() const override;

Q_SIGNALS:
    void hoverStarted();
    void hoverFinished();

protected:
    void enterEvent(QEvent *) { emit hoverStarted(); }
    void leaveEvent(QEvent *) override { emit hoverFinished(); }
};

class AutohidingSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit AutohidingSplitter(Qt::Orientation orientation, QWidget *parent=nullptr);
    explicit AutohidingSplitter(QWidget *parent=nullptr);
    ~AutohidingSplitter() override;

    void setAutohidable(int index, bool autohidable = true);
    void addWidget(QWidget *widget);
    bool restoreState( const QByteArray &state);
    QByteArray saveState() const;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool isAutoHideEnabled() const { return autoHideEnabled; }

public Q_SLOTS:
    void setAutoHideEnabled(bool en);
    void setVisible(bool visible) override;

protected:
    QSplitterHandle * createHandle() override;
    void childEvent(QChildEvent *) override;
    void removeChild(QObject* pObject);
    void addChild(QObject *pObject);
    void resizeEvent(QResizeEvent *) override;

private Q_SLOTS:
    void widgetHoverStarted(int index);
    void widgetHoverFinished(int index);
    void handleHoverStarted();
    void handleHoverFinished();
    void updateResizeQueue();
    void setWidgetForHiding();
    void startAnimation();
    void updateAfterSplitterMoved(int pos, int index);
    void inhibitModifications(){haltModifications = true;}
    void resumeModifications(){haltModifications = false;}

private:
    bool autoHideEnabled;
    bool haltModifications;
    QList<int> getSizesAfterHiding()const;
    SplitterSizeAnimation *autohideAnimation;
    QList<QTimer *> animationDelayTimer;
    QList<bool> widgetAutohidden;
    QList<bool> widgetAutohiddenPrev;
    QList<bool> widgetAutohidable;
    QList<int> expandedSizes;
    QQueue<QList<int> > targetSizes;
    QSet<QWidget *> popupsBlockingAutohiding;
    friend class AutohidingSplitterHandle;
};

#endif
