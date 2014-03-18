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

#include <QDir>
#include <QVariant>
#include <QSqlQuery>
#include <QDateTime>
#include <QDebug>

#include "book_scanner.h"
#include "metadata_reader.h"
#include "file_system_utils.h"

namespace obx
{

BookScanner::BookScanner(const QStringList &bookExtensions)
{
    book_extensions_ = bookExtensions;
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
                QStringList extensions = FileSystemUtils::suffixList(info);
                for (int i = 0; i < extensions.size(); i++)
                {
                    if (book_extensions_.contains(extensions.at(i)))
                    {
                        result |= scanFile(info.absoluteFilePath());
                        break;
                    }
                }
            }
        }
    }
    else
    {
        QStringList extensions = FileSystemUtils::suffixList(QFileInfo(path));
        for (int i = 0; i < extensions.size(); i++)
        {
            if (book_extensions_.contains(extensions.at(i)))
            {
                result = scanFile(path);
                break;
            }
        }
    }

    return result;
}

bool BookScanner::scanFile(const QString &fileName)
{
    bool result = false;

    qDebug() << "scanFile:" << fileName;

    MetadataReader metadata(fileName);
    metadata.collect();

    QSqlQuery query;
    query.prepare("SELECT id FROM books WHERE file = :file");
    query.bindValue(":file", fileName);
    query.exec();
    if (!query.next())
    {
        // Add new entry
		query.prepare("INSERT INTO books (file, title, author, publisher, year, series, "
                                         "series_index, add_date, read_count, cover) "
                      "VALUES (:file, :title, :author, :publisher, :year, :series, "
                              ":series_index, :add_date, :read_count, :cover)");
        query.bindValue(":add_date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":read_count", 0);
    	//qDebug() << "BookScanner :: insert - " << fileName << " " << metadata.calibreSeries() << " " << metadata.calibreSeriesIndex() ;
    }
    else
    {
        // Update metadata
        query.prepare("UPDATE books "
                      "SET title = :title, author = :author, publisher = :publisher, year = :year, "
                          "series = :series, series_index = :series_index, cover = :cover "
                      "WHERE file = :file");
    	//qDebug() << "BookScanner :: update - " << fileName << " " << metadata.calibreSeries() << " " << metadata.calibreSeriesIndex() ;
    }

    query.bindValue(":file", fileName);
    
    //if no metadata, add filename as title
    if (metadata.title().isEmpty()) 
		query.bindValue(":title", fileName)
	else 
		query.bindValue(":title", metadata.title());
		
    query.bindValue(":author", metadata.author().at(0));
    query.bindValue(":publisher", metadata.publisher());
    query.bindValue(":year", metadata.year());
    query.bindValue(":series", metadata.calibreSeries());
    query.bindValue(":series_index", metadata.calibreSeriesIndex());
    query.bindValue(":cover", FileSystemUtils::getMatchingIcon(QFileInfo(fileName),
                                                               QStringList() << "png" << "jpg"));
    result = query.exec();

    FileSystemUtils::getThumbnail(QFileInfo(fileName), QStringList() << "png" << "jpg", true);


    return result;
}

}
