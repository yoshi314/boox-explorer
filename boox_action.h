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

#ifndef BOOX_ACTION_H_
#define BOOX_ACTION_H_

#include "onyx/ui/context_dialog_base.h"

namespace obx
{

class BooxActions : public ui::BaseActions
{
    Q_OBJECT

public:
    BooxActions();
    ~BooxActions();

public:
    void initializeActions(QIcon icon, QString title);
    void addAction(QIcon icon, QString title, int itemKey);
    void addSeparator();
    int  selected();
    bool isEmpty();
};

}

#endif // BOOX_ACTION_H_
