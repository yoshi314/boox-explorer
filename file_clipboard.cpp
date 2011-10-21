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

#include <QFile>
#include <QFileInfo>

#include "file_system_utils.h"
#include "file_clipboard.h"

namespace obx
{

FileClipboard::FileClipboard()
{
}

FileClipboard::~FileClipboard()
{
}

void FileClipboard::cut(const QString &absoluteFilePath)
{
    fileInfo_ = QFileInfo(absoluteFilePath);
    action_ = ACT_MOVE;
}

void FileClipboard::copy(const QString &absoluteFilePath)
{
    fileInfo_ = QFileInfo(absoluteFilePath);
    action_ = ACT_COPY;
}

bool FileClipboard::paste(const QString &absolutePath)
{
    const ActionFunction actionFunction[][2] =
    {
        {QFile::copy, QFile::rename},
        {FileSystemUtils::copyDir, FileSystemUtils::moveDir}
    };

    return (actionFunction[fileInfo_.isDir()][action_])(fileInfo_.absoluteFilePath(),
                                                        absolutePath + "/" + fileInfo_.fileName());
}

void FileClipboard::clear()
{
    action_ = ACT_NONE;
    fileInfo_ = QFileInfo();
}

bool FileClipboard::isEmpty()
{
    return (action_ == ACT_NONE);
}

bool FileClipboard::isClippedToMove()
{
    return (action_ == ACT_MOVE);
}

bool FileClipboard::holdsDir()
{
    return fileInfo_.isDir();
}

QString FileClipboard::fileName()
{
    return fileInfo_.fileName();
}

QFileInfo FileClipboard::fileInfo()
{
    return fileInfo_;
}

FileClipboard::ActionType FileClipboard::action_ = ACT_NONE;
QFileInfo FileClipboard::fileInfo_;

}
