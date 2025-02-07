/*
 * Cantata
 *
 * Copyright (c) 2011-2022 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "magnatunesettingsdialog.h"
#include "magnatuneservice.h"
#include "support/buddylabel.h"
#include <QFormLayout>

MagnatuneSettingsDialog::MagnatuneSettingsDialog(QWidget *parent)
    : Dialog(parent)
{
    setButtons(Ok|Cancel);
    setCaption(tr("Magnatune Settings"));
    QWidget *mw=new QWidget(this);
    QFormLayout *layout=new QFormLayout(mw);
    member=new QComboBox(mw);
    for (int i=0; i<MagnatuneService::MB_Count; ++i) {
        member->addItem(MagnatuneService::membershipStr((MagnatuneService::MemberShip)i, true));
    }
    user=new LineEdit(mw);
    pass=new LineEdit(mw);
    userLabel=new BuddyLabel(tr("Username:"), mw, user);
    passLabel=new BuddyLabel(tr("Password:"), mw, pass);
    pass->setEchoMode(QLineEdit::Password);
    dl=new QComboBox(mw);
    for (int i=0; i<=MagnatuneService::DL_Count; ++i) {
        dl->addItem(MagnatuneService::downloadTypeStr((MagnatuneService::DownloadType)i, true));
    }

    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setWidget(0, QFormLayout::LabelRole, new BuddyLabel(tr("Membership:"), mw, member));
    layout->setWidget(0, QFormLayout::FieldRole, member);
    layout->setWidget(1, QFormLayout::LabelRole, userLabel);
    layout->setWidget(1, QFormLayout::FieldRole, user);
    layout->setWidget(2, QFormLayout::LabelRole, passLabel);
    layout->setWidget(2, QFormLayout::FieldRole, pass);
    BuddyLabel *dlLabel=new BuddyLabel(tr("Downloads:"), mw, dl);
    layout->setWidget(3, QFormLayout::LabelRole, dlLabel);
    layout->setWidget(3, QFormLayout::FieldRole, dl);
    layout->setContentsMargins(0, 0, 0, 0);
    dlLabel->setVisible(false); // TODO: Magnatune downloads!
    dl->setVisible(false); // TODO: Magnatune downloads!

    setMainWidget(mw);
    connect(member, SIGNAL(currentIndexChanged(int)), SLOT(membershipChanged(int)));
}

bool MagnatuneSettingsDialog::run(int m, int d, const QString &u, const QString &p)
{
    member->setCurrentIndex(m);
    dl->setCurrentIndex(d);
    user->setText(u);
    user->setEnabled(m);
    pass->setText(p);
    pass->setEnabled(m);
    userLabel->setEnabled(m);
    passLabel->setEnabled(m);
    // dl->setEnabled(MagnatuneService::MB_Download==m); // TODO: Magnatune downloads!
    return QDialog::Accepted==Dialog::exec();
}

void MagnatuneSettingsDialog::membershipChanged(int i)
{
    user->setEnabled(0!=i);
    pass->setEnabled(0!=i);
    userLabel->setEnabled(0!=i);
    passLabel->setEnabled(0!=i);
    // dl->setEnabled(MagnatuneService::MB_Download==i); // TODO: Magnatune downloads!
}

#include "moc_magnatunesettingsdialog.cpp"
