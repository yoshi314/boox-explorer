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

#include <onyx/sys/sys.h>
#include <onyx/screen/screen_proxy.h>
#include <onyx/screen/screen_update_watcher.h>
#include <onyx/ui/keyboard_navigator.h>

#include "association_dialog.h"

namespace obx
{

AssociationDialog::AssociationDialog(QString extension, QStringList applications, int selected, QWidget *parent)
    : OnyxDialog(parent)
    , vbox_(&content_widget_)
    , hbox_(0)
    , ok_(tr("OK"), 0)
    , selected_app_(selected)
{
    setModal(true);

    updateTitle(tr("Open %1 with... ").arg(extension));
    updateTitleIcon(QPixmap(":/images/filetype_settings_small.png"));

    OnyxCheckBox *btn = 0;
    int index = 0;
    bool set_intial_focus = true;
    for(int row = 0; row < applications.count(); row++, index++)
    {
        btn = new OnyxCheckBox(applications.at(row), 0);
        app_group_.addButton(btn);
        apps_.push_back(btn);
        if (selected == row)
        {
            btn->setFocus();
            btn->setChecked(true);
            set_intial_focus = false;
        }
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(onAppButtonChanged(bool)), Qt::QueuedConnection);
        vbox_.addWidget(btn);
    }

    ok_.setFocusPolicy(Qt::TabFocus);
    hbox_.addStretch();
    hbox_.addWidget(&ok_);

    vbox_.addStretch();
    vbox_.addLayout(&hbox_);

    connect(&ok_, SIGNAL(clicked(bool)), this, SLOT(onOkClicked(bool)));

    onyx::screen::watcher().addWatcher(this);
}

AssociationDialog::~AssociationDialog()
{
}

int AssociationDialog::exec()
{
    shadows_.show(true);
    show();
    onyx::screen::instance().flush();
    onyx::screen::instance().updateWidgetRegion(
        0,
        outbounding(parentWidget()),
        onyx::screen::ScreenProxy::GC,
        false,
        onyx::screen::ScreenCommand::WAIT_ALL);
    return QDialog::exec();
}

void AssociationDialog::onAppButtonChanged(bool)
{
    int count = static_cast<int>(apps_.size());
    for(int i = 0; i < count; i++)
    {
        if (apps_[i]->isChecked())
        {
            selected_app_ = i;
            break;
        }
    }
}

void AssociationDialog::onOkClicked(bool)
{
    shadows_.show(false);
    done(selected_app_);
}

void AssociationDialog::onCloseClicked()
{
    shadows_.show(false);
    done(-1);
}

void AssociationDialog::keyPressEvent(QKeyEvent *ke)
{
    ke->accept();
}

void AssociationDialog::keyReleaseEvent(QKeyEvent *ke)
{
    QWidget *wnd = 0;
    ke->accept();
    switch (ke->key())
    {
    case Qt::Key_Up:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
        wnd = ui::moveFocus(&content_widget_, ke->key());
        if (wnd)
        {
            wnd->setFocus();
        }
        break;
    case Qt::Key_Return:
        break;
    case Qt::Key_Escape:
        done(-1);
        break;
    }
}

bool AssociationDialog::event(QEvent* qe)
{
    bool ret = QDialog::event(qe);
    if (qe->type() == QEvent::UpdateRequest &&
        onyx::screen::instance().isUpdateEnabled())
    {
        onyx::screen::instance().updateWidget(this, onyx::screen::ScreenProxy::DW);
    }
    return ret;
}

}
