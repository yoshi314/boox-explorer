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

#include "book_scanner.h"
#include "metadata_reader.h"
#include "file_system_utils.h"

#include <QDir>
#include <QVariant>
#include <QSqlQuery>
#include <QDateTime>
#include <QDebug>

namespace obx
{

BookScanner::BookScanner(const QStringList &extensions)
{
    extensions_ = extensions;
}

BookScanner::~BookScanner()
{
}

bool BookScanner::scan(const QString &path)
{
    bool result = false;
    QDir dir(path);

    qDebug() << "scan:" << path;

    if (dir.exists())
    {
        Q_FOREACH(QFileInfo info,
                  dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files,
                                    QDir::DirsFirst))
        {
            if (info.isDir())
            {
                result |= scan(info.absoluteFilePath());
            }
            else
            {
                if (extensions_.contains(info.suffix()))
                {
                    result |= scanFile(info.absoluteFilePath());
                }
            }
        }
    }
    else if (extensions_.contains(QFileInfo(path).suffix()))
    {
        result = scanFile(path);
    }

    return result;
}

bool BookScanner::scanFile(const QString &fileName)
{
    bool result = false;

    qDebug() << "scanFile:" << fileName;

    QSqlQuery query;
    query.prepare("SELECT id FROM books WHERE file = :file");
    query.bindValue(":file", fileName);
    query.exec();
    if (!query.next())
    {
        MetadataReader metadata(fileName);
        metadata.collect();

        query.prepare("INSERT INTO books (file, title, author, publisher, year, series, "
                                         "series_index, add_date, read_count, cover) "
                      "VALUES (:file, :title, :author, :publisher, :year, :series, "
                              ":series_index, :add_date, :read_count, :cover)");
        query.bindValue(":file", fileName);
        query.bindValue(":title", metadata.title());
        query.bindValue(":author", metadata.author().at(0));
        query.bindValue(":publisher", metadata.publisher());
        query.bindValue(":year", metadata.year());
        query.bindValue(":series", metadata.calibreSeries());
        query.bindValue(":series_index", metadata.calibreSeriesIndex());
        query.bindValue(":add_date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":read_count", 0);
        query.bindValue(":cover", FileSystemUtils::getMatchingIcon(QFileInfo(fileName),
                                                                   QStringList() << "png" << "jpg"));
        result = query.exec();

        FileSystemUtils::getThumbnail(QFileInfo(fileName), QStringList() << "png" << "jpg");
    }

    return result;
}

}