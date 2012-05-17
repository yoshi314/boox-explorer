/*  Copyright (C) 2011-2012 OpenBOOX
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ASSOCIATION_DIALOG_H_
#define ASSOCIATION_DIALOG_H_

#include <onyx/ui/onyx_dialog.h>

using namespace ui;

namespace obx
{

class AssociationDialog : public OnyxDialog
{
    Q_OBJECT

public:
    AssociationDialog(QString extension, QStringList applications, int current, QWidget *parent = 0);
    virtual ~AssociationDialog();

public:
    int exec();

private Q_SLOTS:
    void onAppButtonChanged(bool);
    void onOkClicked(bool);
    void onCloseClicked();

private:
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);
    bool event(QEvent* qe);

private:
    typedef OnyxCheckBox * CheckBoxPtr;
    typedef std::vector<CheckBoxPtr> Buttons;

    QVBoxLayout    vbox_;
    QButtonGroup   app_group_;
    Buttons        apps_;
    QHBoxLayout    hbox_;
    OnyxPushButton ok_;

    int            selected_app_;
};

}

#endif // ASSOCIATION_DIALOG_H_
