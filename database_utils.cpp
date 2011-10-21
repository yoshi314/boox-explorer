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

#include <QStringList>
#include <QSqlQuery>

#include "database_utils.h"

namespace obx
{

int DatabaseUtils::execQueries(const QStringList &queries)
{
    QSqlQuery query;

    for (int i = 0; i < queries.size(); i++)
    {
        if (!query.exec(queries.at(i)))
        {
            return i;
        }
    }

    return queries.size();
}

bool DatabaseUtils::clearDatabase(const QStringList &exclude)
{
    QStringList tables = QSqlDatabase::database().tables();
    QStringList queries;

    for (int i = 0; i < tables.size(); i++)
    {
        if (!exclude.contains(tables.at(i)))
        {
            queries << QString("DROP TABLE %1").arg(tables.at(i));
        }
    }

    return (DatabaseUtils::execQueries(queries) == queries.size());
}

}
