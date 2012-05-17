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
#include <QFile>
#include <QFileInfo>
#include <QPixmap>

#include "file_system_utils.h"

namespace obx
{

bool FileSystemUtils::copyDir(const QString &source, const QString &destination)
{
    bool result = false;
    QDir sourceDir(source);
    QDir destDir(destination);

    if (sourceDir.exists(source))
    {
        if (!destDir.exists(destination))
        {
            result = destDir.mkdir(destination);

            if (result)
            {
                Q_FOREACH(QFileInfo info,
                          sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                                  QDir::AllDirs | QDir::Files, QDir::DirsLast))
                {
                    if (info.isDir())
                    {
                        result = copyDir(info.absoluteFilePath(),
                                         destination + "/" + QDir(info.absoluteFilePath()).dirName());
                    }
                    else
                    {
                        result = QFile::copy(info.absoluteFilePath(), destination + "/" + info.fileName());
                    }

                    if (!result)
                    {
                        break;
                    }
                }
            }
        }
    }

    return result;
}

bool FileSystemUtils::removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName))
    {
        Q_FOREACH(QFileInfo info,
                  dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files,
                                    QDir::DirsFirst))
        {
            if (info.isDir())
            {
                result = removeDir(info.absoluteFilePath());
            }
            else
            {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
            {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

bool FileSystemUtils::moveDir(const QString &source, const QString &destination)
{
    bool result = copyDir(source, destination);

    if (result)
    {
        result = removeDir(source);
    }

    return result;
}

QString FileSystemUtils::getMatchingIcon(const QFileInfo &fileInfo, const QStringList &iconExtensions)
{
    bool ok;

    QString extension = fileInfo.suffix();
    if (iconExtensions.contains(extension))
    {
        return fileInfo.absoluteFilePath();
    }

    extension.toInt(&ok);

    for (int i = 0; i < iconExtensions.size(); i++)
    {
        QString iconFileName = fileInfo.absoluteFilePath();

        if (ok || extension.isEmpty())
        {
            iconFileName.append(QString(".%1").arg(iconExtensions[i]));
        }
        else
        {
            iconFileName.replace(fileInfo.suffix(), iconExtensions[i]);
        }

        if (QFile::exists(iconFileName))
        {
            return iconFileName;
        }
    }

    return QString();
}

QString FileSystemUtils::getThumbnail(const QFileInfo &fileInfo, const QStringList &iconExtensions, bool update)
{
    QString thumbnailFilePath;

    if (fileInfo.exists())
    {
        QString thumbnailPath = fileInfo.absolutePath() + "/.obx/thumbnails/";
        thumbnailFilePath = thumbnailPath + fileInfo.baseName() + ".png";

        if (!QFile::exists(thumbnailFilePath) || update)
        {
            QString icon;
            QPixmap pixmap;

            if (!QDir().mkpath(thumbnailPath) ||
                (icon = getMatchingIcon(fileInfo, iconExtensions)).isNull() ||
                !pixmap.load(icon))
            {
                return QString();
            }

            pixmap.scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(thumbnailFilePath, "PNG");
        }
    }

    return thumbnailFilePath;
}

bool FileSystemUtils::isSDMounted()
{
    QFile mtab("/etc/mtab");
    mtab.open(QIODevice::ReadOnly);
    QString content(mtab.readAll());
    return content.contains("/media/sd");
}

bool FileSystemUtils::isRunnable(const QFileInfo &fileInfo)
{
    bool result = false;

    if (!fileInfo.isDir() && fileInfo.isExecutable())
    {
        QFile executable(fileInfo.filePath());
        executable.open(QIODevice::ReadOnly);
        QString content(executable.readLine(64));
        if (content.startsWith("#!/"))
        {
            content.remove(0, 2);
            if (QFile::exists(content.simplified()))
            {
                result = true;
            }
        }
        else
        {
            content.truncate(4);
            if (content == QString("%1ELF").arg(QChar(0x7F)))
            {
                result = true;
            }
        }
    }

    return result;
}

}
