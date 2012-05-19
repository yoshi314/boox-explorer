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

#include <QXmlStreamReader>
#include <QFileInfo>
#include <QDebug>

#include <poppler/qt4/poppler-qt4.h>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include "metadata_reader.h"
#include "file_system_utils.h"

namespace obx
{

MetadataReader::MetadataReader(const QString &fileName)
{
    fileInfo_ = QFileInfo(fileName);
    title_ = QString();
    author_ = QStringList();
    publisher_ = QString();
    year_ = QString();

    calibre_series_ = QString();
    calibre_series_index_ = 0;
}

MetadataReader::~MetadataReader()
{
}

void MetadataReader::collect()
{
    typedef enum { NS_NONE = -1, NS_DC, NS_XMP, NS_XAP } NamespaceType;
    typedef enum { DC_NONE = -1, DC_CREATOR, DC_DATE, DC_PUBLISHER, DC_TITLE, DC_META } DcPropertyType;
    typedef enum { XMP_NONE = -1, XMP_CREATEDATE } XmpPropertyType;

    const QStringList namespaces = QStringList() << "dc" << "xmp" << "xap";
    const QStringList dcProperties = QStringList() << "creator" << "date" << "publisher" << "title" << "meta";
    const QStringList xmpProperties = QStringList() << "CreateDate";

    QStringList extensions = FileSystemUtils::suffixList(fileInfo_);
    QString xmlStream;

    for (int i = 0; i < extensions.size(); i++)
    {
        QFile opfFile(fileInfo_.absoluteFilePath().replace(extensions.at(i), "opf"));
        if (opfFile.open(QIODevice::ReadOnly))
        {
            xmlStream = QString(opfFile.readAll());
            opfFile.close();
            break;
        }
    }

    if (xmlStream.isEmpty())
    {
        if (fileInfo_.suffix() == "epub")
        {
            QuaZip archive(fileInfo_.absoluteFilePath());
            if (archive.open(QuaZip::mdUnzip))
            {
                while (archive.goToNextFile())
                {
                    QString fileName = archive.getCurrentFileName();
                    if (fileName.endsWith(".opf"))
                    {
                        qDebug() << fileName;
                        QuaZipFile zippedOpfFile(fileInfo_.absoluteFilePath(), fileName);
                        zippedOpfFile.open(QIODevice::ReadOnly);
                        xmlStream = QString(zippedOpfFile.readAll());
                        zippedOpfFile.close();
                        break;
                    }
                }
                archive.close();
            }
        }
        else if (fileInfo_.suffix() == "pdf")
        {
            Poppler::Document *document = Poppler::Document::load(fileInfo_.absoluteFilePath());

            if (document && !document->isLocked())
            {
                xmlStream = document->metadata();
            }

            delete document;
        }
    }

    if (!xmlStream.isEmpty())
    {
        QXmlStreamReader::TokenType token;
        NamespaceType namespaceId = NS_NONE;
        DcPropertyType dcPropertyId = DC_NONE;
        XmpPropertyType xmpPropertyId = XMP_NONE;

        QXmlStreamReader xmlReader(xmlStream);

        while((token = xmlReader.readNext()) != QXmlStreamReader::Invalid)
        {
            switch (token)
            {
            case QXmlStreamReader::StartElement:
            {
                QXmlStreamNamespaceDeclarations declarations = xmlReader.namespaceDeclarations();
                for (int i = 0; i < declarations.size(); i++)
                {
                    if (!declarations[i].prefix().isEmpty())
                    {
                        namespaceId = NamespaceType(namespaces.indexOf(declarations[i].prefix().toString()));
                        qDebug() << "namespace:" << declarations[i].prefix() << declarations[i].namespaceUri();
                        if (namespaceId != NS_NONE)
                        {
                            break;
                        }
                    }
                }

                switch (namespaceId)
                {
                case NS_DC:
                {
                    DcPropertyType startDcPropertyId = DcPropertyType(dcProperties.indexOf(xmlReader.name().toString()));
                    if (dcPropertyId == DC_NONE)
                    {
                        dcPropertyId = startDcPropertyId;
                    }
                    if (dcPropertyId == DC_META)
                    {
                        calibreCollect(xmlReader.attributes());
                    }
                    break;
                }
                case NS_XMP:
                case NS_XAP:
                {
                    XmpPropertyType startXmpPropertyId = XmpPropertyType(xmpProperties.indexOf(xmlReader.name().toString()));
                    if (xmpPropertyId == XMP_NONE)
                    {
                        xmpPropertyId = startXmpPropertyId;
                    }
                    break;
                }
                default:
                    break;
                }
                break;
            }
            case QXmlStreamReader::EndElement:
                switch (namespaceId)
                {
                case NS_DC:
                {
                    DcPropertyType endDcPropertyId = DcPropertyType(dcProperties.indexOf(xmlReader.name().toString()));
                    if (endDcPropertyId == dcPropertyId)
                    {
                        dcPropertyId = DC_NONE;
                    }
                    break;
                }
                case NS_XMP:
                case NS_XAP:
                {
                    XmpPropertyType endXmpPropertyId = XmpPropertyType(xmpProperties.indexOf(xmlReader.name().toString()));
                    if (endXmpPropertyId == xmpPropertyId)
                    {
                        xmpPropertyId = XMP_NONE;
                    }
                    break;
                }
                default:
                    break;
                }
                break;
            case QXmlStreamReader::Characters:
                if (!xmlReader.isWhitespace())
                {
                    QString value = xmlReader.text().toString();
                    switch (namespaceId)
                    {
                    case NS_DC:
                        switch(dcPropertyId)
                        {
                        case DC_CREATOR:
                            author_ << value;
                            qDebug() << "creator:" << value;
                            break;
                        case DC_DATE:
                        {
                            QString yearString = value.section('-', 0, 0);
                            if (yearString.size() == 4)
                            {
                                year_ = yearString;
                            }
                            qDebug() << "date:" << value;
                            break;
                        }
                        case DC_PUBLISHER:
                            publisher_ = value;
                            qDebug() << "publisher:" << value;
                            break;
                        case DC_TITLE:
                            title_ = value;
                            qDebug() << "title:" << value;
                        default:
                            break;
                        }
                        break;
                    case NS_XMP:
                    case NS_XAP:
                        switch(xmpPropertyId)
                        {
                        case XMP_CREATEDATE:
                        {
                            QString yearString = value.section('-', 0, 0);
                            if (year_ == 0 && yearString.size() == 4)
                            {
                                year_ = yearString;
                            }
                            qDebug() << "CreateDate:" << value;
                            break;
                        }
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case QXmlStreamReader::StartDocument:
            case QXmlStreamReader::EndDocument:
            case QXmlStreamReader::Comment:
            case QXmlStreamReader::DTD:
            case QXmlStreamReader::EntityReference:
            case QXmlStreamReader::ProcessingInstruction:
            default:
                break;
            }
        }
    }

    if (title_.isEmpty())
    {
        title_ = fileInfo_.baseName();
    }
}

QString MetadataReader::fileName()
{
    return fileInfo_.fileName();
}

QString MetadataReader::title()
{
    if (!title_.isEmpty())
    {
        return title_;
    }

    return "Unknown";
}

QStringList MetadataReader::author()
{
    if (author_.size())
    {
        return author_;
    }

    return QStringList() << "Unknown";
}

QString MetadataReader::publisher()
{
    if (!publisher_.isEmpty())
    {
        return publisher_;
    }

    return "Unknown";
}

QString MetadataReader::year()
{
    if (!year_.isEmpty())
    {
        return year_;
    }

    return "Unknown";
}

QString MetadataReader::calibreSeries()
{
    return calibre_series_;
}

int MetadataReader::calibreSeriesIndex()
{
    return calibre_series_index_;
}

void MetadataReader::calibreCollect(QXmlStreamAttributes attributes)
{
    typedef enum { CLB_NONE = -1, CLB_SERIES, CLB_SERIES_INDEX } ClbPropertyType;
    const QStringList clbProperties = QStringList() << "calibre:series" << "calibre:series_index";
    QString contentString;

    for (int i = 0; i < attributes.size(); i++)
    {
        if (attributes[i].name() == "content")
        {
            contentString = attributes[i].value().toString();
        }
        else if (attributes[i].name() == "name")
        {
            ClbPropertyType clbPropertyId = ClbPropertyType(clbProperties.indexOf(attributes[i].value().toString()));

            switch (clbPropertyId)
            {
            case CLB_SERIES:
                calibre_series_ = contentString;
                qDebug() << "calibre:series:" << calibre_series_;
                break;
            case CLB_SERIES_INDEX:
                calibre_series_index_ = contentString.toInt();
                qDebug() << "calibre:series_index:" << calibre_series_index_;
                break;
            default:
                break;
            }
        }
    }
}

}
