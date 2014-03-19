/* vim: set sw=4 sts=4 et foldmethod=syntax : */

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


    // few helper functions ; will be moved somewhere else eventually

    //attempt to convert sequence of bytes into integer
    unsigned int bytesToInt(char * bytes, int start, int count) {
        unsigned char * tmpchars = (unsigned char *) bytes;
        unsigned int value = 0;
        for (int j=0;j<count;j++) {
            value = (value << 8) + tmpchars[start+j];
        }
        return value;
    }

    //print hex bytes of a char *
    void printbytes(char * input, int count) {
        unsigned char * tmpchar2 = (unsigned char*) input;

        printf(" [ ");
        for(int j = 0; j < count; j++)
            printf(" %02x ", tmpchar2[j]);
        printf(" ]\n");
    }


    MetadataReader::MetadataReader(const QString &fileName)
    {
        fileInfo_ = QFileInfo(fileName);
        title_ = QString();
        author_ = QStringList();
        publisher_ = QString();
        year_ = QString();

        calibre_series_ = QString();
        calibre_series_index_ = 0;
        //    qDebug() << "MetadataReader instantiated : for " << fileName ;
    }

    MetadataReader::~MetadataReader()
    {
    }

    bool MetadataReader::readMobiMetadata() {

        /* for simplicity, keep failing at first error found */

        QFile mobiFile(fileInfo_.absoluteFilePath());
        if (!mobiFile.open(QIODevice::ReadOnly)) {
            qDebug("failed to open file\n");
            return false;
        }

        QDataStream mobiStream(&mobiFile);

        /* file starts with pdb header,
         * 4bytes at 60 should be 'BOOK'
         * 4bytes at 64 should be 'MOBI'
         * if file is a valid .mobi
         */

        if (!mobiFile.seek(60)) {
            qDebug("seek failed\n");
            return false;
        }

        // 8 bytes for signature
        char * tmpchar = new  char[8];

        if (mobiStream.readRawData(tmpchar,8) != 8) {
            qDebug("failed to read 8 bytes\n");
            return false;
        }

        //check'em
        if (strncmp(tmpchar,"BOOKMOBI",8) != 0) {
            qDebug ( "file does not appear to be a MobiPocket format\n");
            return false;
        }

        /* position 76 has number of PDB records
         * records follow straight after
         * we only need to get into 1st one
         */

        //goto pos 76
        mobiFile.seek(76);

        delete tmpchar;

        tmpchar = new  char[2];

        unsigned int records = 0 ;

        if (mobiStream.readRawData(tmpchar,2) == 2) {
            records = bytesToInt(tmpchar,0,2) ;   //needs a batter way to figure it out
            qDebug ( "mobi records : %d \n", records);
        }

        qDebug("parsing record0\n");

        /*
           records start at  78 and take up 8*records bytes of data
http://wiki.mobileread.com/wiki/PDB#Palm_Database_Format
         */

        mobiFile.seek(78);

        delete tmpchar;
        tmpchar = new char[8];

        //4b - record offset from start of file
        //1b - record flags
        //3b - record id


        /* skip over all the record list, we don't really need those */
        mobiFile.seek(mobiFile.pos() + 8*records);

        /* there are 2 bytes of padding after records, skip them */

        mobiFile.seek(mobiFile.pos() + 2);

        //save this location as reference for later
        qint64 header0pos = mobiFile.pos();

        /* 16 byte palmdoc header starts here, read it */
        delete tmpchar;

        /* instead of parsing, we just go over it and skip to record #0 that follows */
        mobiFile.seek(mobiFile.pos() + 16);

        //go through MOBI header
        //first let's see if we have a header, 4 bytes should be 'MOBI', 16 bytes from beginning of PalmDoc header

        tmpchar = new char[4];

        if ((mobiStream.readRawData(tmpchar,4) == 4) && (strncmp(tmpchar,"MOBI",4) == 0)) {
            qDebug("got MOBI header in record 0\n");
        } else {
            qDebug("no MOBI header. exiting\n");
            return false;
        }

        //next up is header length that includes that 'MOBI' signature just read.
        unsigned int mobiHeaderSize  = 0;

        if (mobiStream.readRawData(tmpchar,4) == 4) {
            mobiHeaderSize = bytesToInt(tmpchar,0,4);
        } else {
            qDebug("failed to read 4 bytes\n");
            return false;
        }

        printf("mobi header size : %d [ %0x ]\n", mobiHeaderSize, mobiHeaderSize);

        printf("EXTH should start at %llx\n", mobiHeaderSize + header0pos + 0x10);
        //saved position, 16 bytes for palmdoc header, and mobi header size
        //and we should arrive at EXTH

        //check if EXTH record exists, by inspecting flags in header

        //location of EXTH flags in header;
        if (!mobiFile.seek(header0pos + 0x80)) {
            qDebug("failed to seek to EXTH header\n");
            return false;
        };

        bool got_exth_header = false;

        if (mobiStream.readRawData(tmpchar,4) != 4) {
            qDebug("failed to read 4 bytes\n");
            printbytes(tmpchar,4);
            return false;
        }

        unsigned int exth_flags = bytesToInt(tmpchar,0,4);

        //flags field should have 0x40 byte set
        //that means that EXTH data follows the MOBI header
        if ( exth_flags & 0x40)
            got_exth_header = true;


        if (!got_exth_header) {
            qDebug("EXTH not found, exiting\n");
            return false;
        }

        qDebug("EXTH header exists\n");

        //go through EXTH header, if found (indicated in MOBI header)

        //navigating to start of EXTH
        qDebug("seeking to EXTH header\n");

        //first palmdoc entry offset + 16bytes + entire mobi header is where exth starts
        qint64 exth_pos = header0pos + mobiHeaderSize+0x10;

        if (!mobiFile.seek(exth_pos)) {
            qDebug("failed to seek to EXTH data\n");
            return false;
        }

        printf("at position %llu [ %llx ]\n", mobiFile.pos(), mobiFile.pos());
        /*
           4 bytes - 'EXTH'
           4 bytes - length of entire header, not padded
           4 bytes - number of records

           < records follow >
           record {
           4 bytes      type
           4 bytes      length of entire record (that is, including 8 bytes for type and length)
           length-8     actual record data
           }

           0-3 bytes padding - not relevant here, we're not going that far
         */


        delete tmpchar;
        tmpchar = new char [12];

        qDebug("reading 12 bytes\n");
        if (mobiStream.readRawData(tmpchar,12) != 12) {
            qDebug("failed to read 12 bytes from file\n");
            return false;
        }
        //        printbytes(tmpchar,12);
        if (strncmp(tmpchar,"EXTH",4) != 0) {
            qDebug("EXTH header not found\n");
            return false;
        }

        qDebug("got EXTH header\n");
        unsigned int headerlength = bytesToInt(tmpchar,4,4);
        unsigned int exthRecords = bytesToInt(tmpchar,8,4);

        printf("header is %x long \n", headerlength);
        printf("there are %x records \n",exthRecords);

        //go through the EXTH records
        for (unsigned int j=0; j<exthRecords; j++)
        {
            char * tmprecord = new char [8];
            if (mobiStream.readRawData(tmprecord,8) !=8 ) {
                qDebug("failed to seek 8 bytes\n");
                return false;
            }

            //                printf("record %d/%d\n",j,exthRecords);
            unsigned int recType = bytesToInt(tmprecord,0,4);
            //                printf(" type   : %d\n",recType);
            unsigned int recLength = bytesToInt(tmprecord,4,4);
            //                printf(" length : %0x\n", recLength);

            char * recordData = new char[(int)recLength - 8];
            if (mobiStream.readRawData(recordData,(int)recLength - 8) != (int)recLength -8) {
                qDebug("failed to read record data\n");
                return false;
            }

            QString tmpstring = QString(recordData);
            tmpstring.resize((int)recLength -8);

            switch (recType) {
                case 100:
                    printf("author : %s \n", recordData);

                    /* authors are in array, add one by one
                     * with no repetitions
                     */
                    if (!author_.contains(tmpstring))
                        author_ << tmpstring;
                    break;
                case 101:
                    publisher_ = tmpstring;
                    break;
                case 106:
                    printf("date : %s \n", recordData);
                    year_ = tmpstring.section('-', 0, 0);
                    break;
                case 105:
                    printf("genre : %s \n", recordData);
                    break;
                case 503:
                    printf("title : %s \n", recordData);
                    title_ = tmpstring;
                    break;

                default:
                    break;
            }
            delete tmprecord;
        } // for unsigned int






        return true;

    }

    void MetadataReader::collect()
    {
        //	qDebug() << "MetadataReader::collect() " ;
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
                qDebug() << "trying opf : " << opfFile.fileName() << "\n";
                xmlStream = QString(opfFile.readAll());
                opfFile.close();
                break;
            }
        }

        if (xmlStream.isEmpty())
            qDebug("empty xml stream\n");
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
            else if (fileInfo_.suffix() == "mobi")
            {
                qDebug("trying to read mobi metadata\n");
                if (!readMobiMetadata()) {
                    qDebug("mobi scan failed\n");
                }
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
                                            //                            qDebug() << "creator:" << value;
                                            break;
                                        case DC_DATE:
                                            {
                                                QString yearString = value.section('-', 0, 0);
                                                if (yearString.size() == 4)
                                                {
                                                    year_ = yearString;
                                                }
                                                //                            qDebug() << "date:" << value;
                                                break;
                                            }
                                        case DC_PUBLISHER:
                                            publisher_ = value;
                                            //                            qDebug() << "publisher:" << value;
                                            break;
                                        case DC_TITLE:
                                            title_ = value;
                                            //                            qDebug() << "title:" << value;
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
                                                //                            qDebug() << "CreateDate:" << value;
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
            qDebug() << "attrname " << i << " " << attributes[i].name() << " " << attributes[i].value().toString();
            if (attributes[i].name() == "content")
            {
                contentString = attributes[i].value().toString();
            }
            else if (attributes[i].name() == "name")
            {
                ClbPropertyType clbPropertyId = ClbPropertyType(clbProperties.indexOf(attributes[i].value().toString()));

                //parameters in calibre are often reversed (value, name)
                //which made this checks come up empty

                //there is an assumption that they go in pairs of 0 and 1.
                //might crash if something is changed in calibre metadata someday

                switch (clbPropertyId)
                {
                    case CLB_SERIES:
                        //qDebug() << "attrname " << 1-i << " " << attributes[1-i].name() << " " << attributes[1-i].value().toString();
                        calibre_series_ = attributes[1-i].value().toString();
                        //qDebug() << " got series as " << calibre_series_;
                        break;
                    case CLB_SERIES_INDEX:
                        qDebug() << "attrname " << 1-i << " " << attributes[1-i].name() << " " << attributes[1-i].value().toString();
                        calibre_series_index_ =
                            attributes[1-i].value().toString().remove(QChar('.'), Qt::CaseInsensitive).toInt();
                        //qDebug() << " got index as " << calibre_series_index_;
                        break;
                    default:
                        break;
                }
            }
        }
    }

}
