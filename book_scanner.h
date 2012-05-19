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

#ifndef BOOK_SCANNER_H_
#define BOOK_SCANNER_H_

#include <QString>
#include <QStringList>

namespace obx
{

class BookScanner
{
public:
    BookScanner(const QStringList &bookExtensions);
    ~BookScanner();

public:
    bool scan(const QString &path);

private:
    bool scanFile(const QString &fileName);

private:
    QStringList book_extensions_;
};

}

#endif // BOOK_SCANNER_H_
