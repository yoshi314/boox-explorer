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

#ifndef OBX_EXPLORER_H_
#define OBX_EXPLORER_H_

#include <QApplication>

#include "explorer_view.h"

namespace obx
{

class ObxExplorer : public QApplication
{
    Q_OBJECT

public:
    ObxExplorer(int &argc, char **argv);
    ~ObxExplorer();

public Q_SLOTS:
    void onScreenSizeChanged();
    void onMusicPlayerStateChanged(int);
    void onWpaConnectionChanged(WifiProfile, WpaConnection::ConnectionState);

private:
    bool         mainUI_;
    ExplorerView view_;
};

class ObxExplorerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.onyx.interface.explorer")

public:
    ObxExplorerAdaptor(ObxExplorer *application)
        : QDBusAbstractAdaptor(application)
        , app_(application)
    {
        QDBusConnection::systemBus().registerService("com.onyx.service.explorer");
        QDBusConnection::systemBus().registerObject("/com/onyx/object/explorer", app_);
    }

private:
    ObxExplorer *app_;
    NO_COPY_AND_ASSIGN(ObxExplorerAdaptor);
};

extern bool initializeDatabase();

}

#endif /* OBX_EXPLORER_H_ */
