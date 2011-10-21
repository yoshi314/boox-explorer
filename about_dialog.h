/*  Copyright (C) 2011  OpenBOOX
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

#ifndef ABOUT_DIALOG_H_
#define ABOUT_DIALOG_H_

#include "onyx/ui/status_bar.h"

using namespace ui;

namespace obx
{

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(bool mainUI, QWidget *parent = 0);
    ~AboutDialog();

private:
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);

private:
    QVBoxLayout    vbox_;
    QWidget        title_widget_;
    QHBoxLayout    title_layout_;
    QLabel         title_icon_;
    QLabel         title_label_;
    OnyxPushButton close_button_;

    QLabel         logo_;
    QLabel         about_;

    StatusBar      status_bar_;
};

}

#endif // ABOUT_DIALOG_H_
