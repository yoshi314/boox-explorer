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

#ifndef FILE_SYSTEM_UTILS_H_
#define FILE_SYSTEM_UTILS_H_

namespace obx
{

class FileSystemUtils
{
public:
    static bool copyDir(const QString &source, const QString &destination);
    static bool removeDir(const QString &dirName);
    static bool moveDir(const QString &source, const QString &destination);
    static QString getMatchingIcon(const QFileInfo &fileInfo, const QStringList &iconExtensions);
    static QString getThumbnail(const QFileInfo &fileInfo, const QStringList &iconExtensions);
    static bool isSDMounted();
    static bool isScript(const QFileInfo &fileInfo);
    static bool isElfBinary(const QFileInfo &fileInfo);
    static bool isRunnable(const QFileInfo &fileInfo);
};

}

#endif // FILE_SYSTEM_UTILS_H_
