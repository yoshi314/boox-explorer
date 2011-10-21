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

#include "about_dialog.h"
#include "file_system_utils.h"

#include "onyx/sys/sys.h"
#include "onyx/sys/sys_utils.h"
#include "onyx/screen/screen_proxy.h"

namespace obx
{

static const QString PAGE_BUTTON_STYLE =   "\
QPushButton                             \
{                                       \
    background: transparent;            \
    font-size: 14px;                    \
    border-width: 1px;                  \
    border-color: transparent;          \
    border-style: solid;                \
    color: black;                       \
    padding: 0px;                       \
}                                       \
QPushButton:pressed                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:focus {                     \
    border-width: 2px;                  \
    border-color: black;                \
    border-style: solid;                \
    border-radius: 3;                   \
}                                       \
QPushButton:checked                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    color: white;                       \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:disabled                    \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: dark;                 \
    color: dark;                        \
    background-color: white;            \
}";

static QIcon loadIcon(const QString & path, QSize & size)
{
    QPixmap pixmap(path);
    size = pixmap.size();
    return QIcon(pixmap);
}

AboutDialog::AboutDialog(bool mainUI, QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint)
    , vbox_(this)
    , title_widget_(this)
    , title_layout_(&title_widget_)
    , title_icon_(0)
    , title_label_(0)
    , close_button_("", 0)
    , logo_(this)
    , about_(this)
    , status_bar_(this, MESSAGE | BATTERY)
{
    resize(qApp->desktop()->screenGeometry().size());

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    vbox_.setSpacing(0);
    vbox_.setContentsMargins(0, 0, 0, 0);

    title_layout_.setSpacing(2);
    title_layout_.setContentsMargins(5, 0, 5, 0);
    title_icon_.setPixmap(QPixmap(":/images/about_dialog_icon.png"));
    QFont title_font(title_label_.font());
    title_font.setPointSize(25);
    title_label_.setFont(title_font);
    title_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    title_label_.setIndent(10);
    title_label_.setText(QCoreApplication::tr("About"));
    title_layout_.addWidget(&title_icon_, 10);
    title_layout_.addWidget(&title_label_, 300);

    QSize icon_size;
    close_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    QIcon close_icon = loadIcon(":/images/close.png", icon_size);
    close_button_.setIconSize(icon_size);
    close_button_.setIcon(close_icon);
    close_button_.setFocusPolicy(Qt::TabFocus);
    title_layout_.addWidget(&close_button_);

    // title widget
    title_widget_.setAutoFillBackground(true);
    title_widget_.setBackgroundRole(QPalette::Dark);
    title_widget_.setContentsMargins(0, 0, 0, 0);
    title_widget_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    logo_.setPixmap(QPixmap(":/images/OpenBOOX-logo.png"));
    logo_.setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    logo_.setMargin(30);
    logo_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QString aboutString = tr("The goal of the OpenBOOX project is to provide an "
                             "alternative firmware version for the Boox60 eReader from Onyx International.\n\n");

    if (mainUI)
    {
        aboutString += tr("Version: %1\n").arg(QString(qgetenv("OBX_VERSION")));
        aboutString += tr("Build Date: %1\n").arg(QString(qgetenv("OBX_BUILDDATE")));
        aboutString += tr("Donor Firmware: Boox A60 %1 %2\n\n")
                       .arg(QString(qgetenv("DONOR_VERSION")))
                       .arg(QString(qgetenv("DONOR_BUILDDATE")));

        aboutString += tr("Free Space\n");
        aboutString += tr("    Memory: %1/%2 MB\n")
                       .arg(sys::systemFreeMemory() / 1048576)
                       .arg(sys::systemTotalMemory() / 1048576);

        aboutString += tr("    Internal Flash: %1/%2 MB\n")
                       .arg(sys::freeSpace("/media/flash") / 1048576)
                       .arg(sys::diskSpace("/media/flash") / 1048576);

        if (FileSystemUtils::isSDMounted())
        {
            aboutString += tr("    SD Card: %1/%2 MB\n")
                           .arg(sys::freeSpace("/media/sd") / 1048576)
                           .arg(sys::diskSpace("/media/sd") / 1048576);
        }

        about_.setAlignment(Qt::AlignLeft|Qt::AlignTop);
    }
    else
    {
        aboutString += tr("\nOpenBOOX Explorer [standalone mode]\n\n"
                          "The OpenBOOX Explorer is running in standalone mode. "
                          "If you install the OpenBOOX firmware you can use this explorer as the main user interface. "
                          "In this case you will get additional functionality and a fully integrated user experience.\n\n"
                          "http://openbooxproject.sourceforge.net");

        about_.setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    }

    QFont text_font(about_.font());
    text_font.setPointSize(20);
    about_.setText(aboutString);
    about_.setFont(text_font);
    about_.setMargin(30);
    about_.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    about_.setWordWrap(true);

    text_font.setPointSize(16);
    text_font.setBold(true);
    status_bar_.setMessage("Licensed under the GNU General Public License, version 2", text_font);

    vbox_.addWidget(&title_widget_);
    vbox_.addWidget(&logo_);
    vbox_.addWidget(&about_);
    vbox_.addWidget(&status_bar_);

    // Connections
    connect(&close_button_, SIGNAL(clicked(bool)), this, SLOT(close()), Qt::QueuedConnection);

    this->show();
    onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC);
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::keyPressEvent(QKeyEvent *ke)
{
    ke->accept();
}

void AboutDialog::keyReleaseEvent(QKeyEvent *ke)
{
    switch(ke->key())
    {
    case Qt::Key_Escape:
        close();
        break;
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_R:
    case Qt::Key_Menu:
    default:
        break;
    }
}

}
