/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "VersionCheckTask.h"

#include <QMetaType>
#include <QtNetwork>
#include <memory>
#include <nlohmann/json.hpp>

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend::Module::Integrated::VersionCheckingModule {

VersionCheckTask::VersionCheckTask()
    : Task("version_check_task"),
      network_manager_(new QNetworkAccessManager(this)),
      current_version_(std::string("v") + std::to_string(VERSION_MAJOR) + "." +
                       std::to_string(VERSION_MINOR) + "." +
                       std::to_string(VERSION_PATCH)) {
  HoldOnLifeCycle(true);
  qRegisterMetaType<SoftwareVersion>("SoftwareVersion");
  version_.current_version = current_version_;
}

void VersionCheckTask::Run() {
  try {
    using namespace nlohmann;
    MODULE_LOG_DEBUG("current version: {}", current_version_);
    std::string latest_version_url =
        "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";

    QNetworkRequest latest_request;
    latest_request.setUrl(QUrl(latest_version_url.c_str()));
    latest_reply_ = network_manager_->get(latest_request);
    connect(latest_reply_, &QNetworkReply::finished, this,
            &VersionCheckTask::slot_parse_latest_version_info);

    // loading done
    version_.loading_done = true;

  } catch (...) {
    MODULE_LOG_ERROR("unknown error occurred");
    emit SignalTaskShouldEnd(-1);
  }
}

void VersionCheckTask::slot_parse_latest_version_info() {
  version_.current_version = current_version_;

  try {
    if (latest_reply_ == nullptr ||
        latest_reply_->error() != QNetworkReply::NoError) {
      MODULE_LOG_ERROR("latest version request error");
      version_.latest_version = current_version_;
    } else {
      latest_reply_bytes_ = latest_reply_->readAll();

      auto latest_reply_json =
          nlohmann::json::parse(latest_reply_bytes_.toStdString());

      std::string latest_version = latest_reply_json["tag_name"];

      MODULE_LOG_INFO("latest version from Github: {}", latest_version);

      QRegularExpression re(R"(^[vV](\d+\.)?(\d+\.)?(\*|\d+))");
      auto version_match = re.match(latest_version.c_str());
      if (version_match.hasMatch()) {
        latest_version = version_match.captured(0).toStdString();
        MODULE_LOG_DEBUG("latest version matched: {}", latest_version);
      } else {
        latest_version = current_version_;
        SPDLOG_WARN("latest version unknown");
      }

      bool prerelease = latest_reply_json["prerelease"],
           draft = latest_reply_json["draft"];
      std::string publish_date = latest_reply_json["published_at"];
      std::string release_note = latest_reply_json["body"];
      version_.latest_version = latest_version;
      version_.latest_prerelease_version_from_remote = prerelease;
      version_.latest_draft_from_remote = draft;
      version_.publish_date = publish_date;
      version_.release_note = release_note;
    }
  } catch (...) {
    MODULE_LOG_ERROR("unknown error occurred");
    version_.loading_done = false;
  }

  if (latest_reply_ != nullptr) {
    latest_reply_->deleteLater();
  }

  try {
    std::string current_version_url =
        "https://api.github.com/repos/saturneric/gpgfrontend/releases/tags/" +
        current_version_;

    QNetworkRequest current_request;
    current_request.setUrl(QUrl(current_version_url.c_str()));
    current_reply_ = network_manager_->get(current_request);

    connect(current_reply_, &QNetworkReply::finished, this,
            &VersionCheckTask::slot_parse_current_version_info);
  } catch (...) {
    MODULE_LOG_ERROR("current version request create error");
    emit SignalTaskShouldEnd(-1);
  }
}

void VersionCheckTask::slot_parse_current_version_info() {
  try {
    if (current_reply_ == nullptr ||
        current_reply_->error() != QNetworkReply::NoError) {
      if (current_reply_ != nullptr) {
        MODULE_LOG_ERROR("current version request network error: {}",
                         current_reply_->errorString().toStdString());
      } else {
        MODULE_LOG_ERROR(
            "current version request network error, null reply object");
      }
      version_.current_version_publish_in_remote = false;
      version_.loading_done = false;
    } else {
      version_.current_version_publish_in_remote = true;
      current_reply_bytes_ = current_reply_->readAll();
      MODULE_LOG_DEBUG("current version: {}", current_reply_bytes_.size());
      auto current_reply_json =
          nlohmann::json::parse(current_reply_bytes_.toStdString());
      bool current_prerelease = current_reply_json["prerelease"],
           current_draft = current_reply_json["draft"];
      version_.latest_prerelease_version_from_remote = current_prerelease;
      version_.latest_draft_from_remote = current_draft;
      version_.loading_done = true;
    }
  } catch (...) {
    MODULE_LOG_ERROR("unknown error occurred");
    version_.loading_done = false;
  }

  MODULE_LOG_DEBUG("current version parse done: {}",
                   version_.current_version_publish_in_remote);

  if (current_reply_ != nullptr) {
    current_reply_->deleteLater();
  }

  emit SignalUpgradeVersion(version_);
  emit SignalTaskShouldEnd(0);
}

}  // namespace GpgFrontend::Module::Integrated::VersionCheckingModule
