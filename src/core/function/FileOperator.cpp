/**
* Copyright (C) 2021 Saturneric
*
* This file is part of GpgFrontend.
*
* GpgFrontend is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* GpgFrontend is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
*
* The initial version of the source code is inherited from
* the gpg4usb project, which is under GPL-3.0-or-later.
*
* All the source code of GpgFrontend was modified and released by
* Saturneric<eric@bktus.com> starting on May 12, 2021.
*
* SPDX-License-Identifier: GPL-3.0-or-later
*
*/

#include "FileOperator.h"

bool GpgFrontend::FileOperator::ReadFile(const QString& file_name,
                                         QByteArray& data) {
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    LOG(ERROR) << "failed to open file" << file_name.toStdString();
    return false;
  }
  data = file.readAll();
  file.close();
  return true;
}

bool GpgFrontend::FileOperator::WriteFile(const QString& file_name,
                                          const QByteArray& data) {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG(ERROR) << "failed to open file" << file_name.toStdString();
    return false;
  }
  file.write(data);
  file.close();
  return true;
}

bool GpgFrontend::FileOperator::ReadFileStd(
    const std::filesystem::path& file_name, std::string& data) {
  QByteArray byte_data;
  bool res = ReadFile(QString::fromStdString(file_name.string()), byte_data);
  data = byte_data.toStdString();
  return res;
}

bool GpgFrontend::FileOperator::WriteFileStd(
    const std::filesystem::path& file_name, const std::string& data) {
  return WriteFile(QString::fromStdString(file_name.string()),
                       QByteArray::fromStdString(data));
}
