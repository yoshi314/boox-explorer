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

#ifndef METADATA_READER_H_
#define METADATA_READER_H_

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QXmlStreamAttributes>

namespace obx
{

class MetadataReader
{
public:
    MetadataReader(const QString &fileName);
    ~MetadataReader();

public:
    void collect();
    QString fileName();
    QString title();
    QStringList author();
    QString publisher();
    QString year();

    QString calibreSeries();
    int     calibreSeriesIndex();

private:
    void calibreCollect(QXmlStreamAttributes attributes);

private:
    QFileInfo   fileInfo_;
    QString     title_;
    QStringList author_;
    QString     publisher_;
    QString     year_;

    QString     calibre_series_;
    int         calibre_series_index_;
};

}

#endif // METADATA_READER_H_
