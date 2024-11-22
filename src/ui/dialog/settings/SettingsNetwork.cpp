/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "SettingsNetwork.h"

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "ui/thread/ProxyConnectionTestTask.h"
#include "ui_NetworkSettings.h"

GpgFrontend::UI::NetworkTab::NetworkTab(QWidget *parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_NetworkSettings>()) {
  ui_->setupUi(this);

  connect(ui_->enableProxyCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) {
            switch_ui_enabled(state == Qt::Checked);
            // when selecting no proxy option, apply it immediately
            if (state != Qt::Checked) apply_proxy_settings();
          });

  connect(
      ui_->autoFetchKeyPublishStatusCheckBox, &QCheckBox::stateChanged, this,
      [=](int state) {
        ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(
            state == Qt::Checked
                ? Qt::Unchecked
                : ui_->forbidALLGnuPGNetworkConnectionCheckBox->checkState());
      });

  connect(ui_->forbidALLGnuPGNetworkConnectionCheckBox,
          &QCheckBox::stateChanged, this, [=](int state) {
            ui_->autoFetchKeyPublishStatusCheckBox->setCheckState(
                state == Qt::Checked
                    ? Qt::Unchecked
                    : ui_->autoFetchKeyPublishStatusCheckBox->checkState());
          });

  connect(
      ui_->proxyTypeComboBox, &QComboBox::currentTextChanged, this,
      [=](const QString &current_text) { switch_ui_proxy_type(current_text); });

  connect(ui_->checkProxyConnectionButton, &QPushButton::clicked, this,
          &NetworkTab::slot_test_proxy_connection_result);

  ui_->proxyGroupBox->setTitle(tr("Proxy"));
  ui_->capabilityGroupBox->setTitle(tr("Network Ability"));
  ui_->operationsGroupBox->setTitle(tr("Operations"));

  ui_->enableProxyCheckBox->setText(tr("Enable Proxy"));
  ui_->proxyServerPortLabel->setText(tr("Port"));

  ui_->proxyServerAddressLabel->setText(tr("Host Address"));
  ui_->proxyServerPortLabel->setText(tr("Port"));
  ui_->proxyTypeLabel->setText(tr("Proxy Type"));
  ui_->usernameLabel->setText(tr("Username"));
  ui_->passwordLabel->setText(tr("Password"));

  ui_->checkProxyConnectionButton->setText(
      tr("Apply Proxy Settings and Check Proxy Connection"));

  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setText(
      tr("Forbid all GnuPG network connection."));

  if (Module::IsModuleActivate(kVersionCheckingModuleID)) {
    ui_->prohibitUpdateCheck->setText(
        tr("Prohibit checking for version updates when the program starts."));
  }
  ui_->autoFetchKeyPublishStatusCheckBox->setText(
      tr("Automatically fetch key publish status from key server."));
  ui_->networkAbilityTipsLabel->setText(
      tr("Tips: These Option Changes take effect only after the "
         "application restart."));
  SetSettings();
}

void GpgFrontend::UI::NetworkTab::SetSettings() {
  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  auto proxy_host = settings.value("proxy/proxy_host").toString();
  ui_->proxyServerAddressEdit->setText(proxy_host);
  auto username = settings.value("proxy/username").toString();
  ui_->usernameEdit->setText(username);
  auto password = settings.value("proxy/password").toString();
  ui_->passwordEdit->setText(password);

  auto port = settings.value("proxy/port", 0).toInt();
  ui_->portSpin->setValue(port);

  ui_->proxyTypeComboBox->setCurrentText("HTTP");

  auto proxy_type = settings.value("proxy/proxy_type").toString();
  ui_->proxyTypeComboBox->setCurrentText(proxy_type);

  switch_ui_proxy_type(ui_->proxyTypeComboBox->currentText());

  ui_->enableProxyCheckBox->setCheckState(Qt::Unchecked);

  auto proxy_enable = settings.value("proxy/enable", false).toBool();
  ui_->enableProxyCheckBox->setCheckState(proxy_enable ? Qt::Checked
                                                       : Qt::Unchecked);

  auto forbid_all_gnupg_connection =
      settings.value("network/forbid_all_gnupg_connection").toBool();
  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(
      forbid_all_gnupg_connection ? Qt::Checked : Qt::Unchecked);

  auto prohibit_update_checking =
      settings.value("network/prohibit_update_checking").toBool();
  ui_->prohibitUpdateCheck->setCheckState(
      prohibit_update_checking ? Qt::Checked : Qt::Unchecked);

  auto auto_fetch_key_publish_status =
      settings.value("network/auto_fetch_key_publish_status", false).toBool();
  ui_->autoFetchKeyPublishStatusCheckBox->setCheckState(
      auto_fetch_key_publish_status ? Qt::Checked : Qt::Unchecked);

  switch_ui_proxy_type(ui_->proxyTypeComboBox->currentText());
  switch_ui_enabled(ui_->enableProxyCheckBox->isChecked());
}

void GpgFrontend::UI::NetworkTab::ApplySettings() {
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  settings.setValue("proxy/proxy_host", ui_->proxyServerAddressEdit->text());
  settings.setValue("proxy/username", ui_->usernameEdit->text());
  settings.setValue("proxy/password", ui_->passwordEdit->text());
  settings.setValue("proxy/port", ui_->portSpin->value());
  settings.setValue("proxy/proxy_type", ui_->proxyTypeComboBox->currentText());
  settings.setValue("proxy/enable", ui_->enableProxyCheckBox->isChecked());

  settings.setValue("network/forbid_all_gnupg_connection",
                    ui_->forbidALLGnuPGNetworkConnectionCheckBox->isChecked());
  settings.setValue("network/prohibit_update_checking",
                    ui_->prohibitUpdateCheck->isChecked());
  settings.setValue("network/auto_fetch_key_publish_status",
                    ui_->autoFetchKeyPublishStatusCheckBox->isChecked());

  apply_proxy_settings();
}

void GpgFrontend::UI::NetworkTab::slot_test_proxy_connection_result() {
  apply_proxy_settings();

  bool ok;
  auto url = QInputDialog::getText(this, tr("Test Server Url Accessibility"),
                                   tr("Server Url"), QLineEdit::Normal,
                                   "https://", &ok);
  if (ok && !url.isEmpty()) {
    auto *task = new ProxyConnectionTestTask(url, 800);
    connect(task,
            &GpgFrontend::UI::ProxyConnectionTestTask::
                SignalProxyConnectionTestResult,
            this, [=](const QString &result) {
              if (result == "Reachable") {
                QMessageBox::information(
                    this, tr("Success"),
                    tr("Successfully connect to the target "
                       "server through the proxy server."));
              } else {
                QMessageBox::critical(
                    this, tr("Failed"),
                    tr("Unable to connect to the target server through the "
                       "proxy server. Proxy settings may be invalid."));
              }
            });

    // Waiting Dialog
    auto *waiting_dialog = new QProgressDialog(this);
    waiting_dialog->setMaximum(0);
    waiting_dialog->setMinimum(0);
    auto *waiting_dialog_label = new QLabel(
        tr("Test Proxy Server Connection...") + "<br /><br />" +
        tr("Is using your proxy settings to access the url. Note that this "
           "test "
           "operation will apply your proxy settings to the entire software."));
    waiting_dialog_label->setWordWrap(true);
    waiting_dialog->setLabel(waiting_dialog_label);
    waiting_dialog->resize(420, 120);
    connect(task, &Thread::Task::SignalTaskEnd, [=]() {
      waiting_dialog->close();
      waiting_dialog->deleteLater();
    });

    // Show Waiting Dialog
    waiting_dialog->show();
    waiting_dialog->setFocus();

    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(task);

    QEventLoop loop;
    connect(task, &Thread::Task::SignalTaskEnd, &loop, &QEventLoop::quit);
    connect(waiting_dialog, &QProgressDialog::canceled, &loop,
            &QEventLoop::quit);
    loop.exec();
  }
}

void GpgFrontend::UI::NetworkTab::apply_proxy_settings() {
  // apply settings
  QNetworkProxy proxy;
  if (ui_->enableProxyCheckBox->isChecked() &&
      proxy_type_ != QNetworkProxy::DefaultProxy) {
    proxy.setType(proxy_type_);
    proxy.setHostName(ui_->proxyServerAddressEdit->text());
    proxy.setPort(ui_->portSpin->value());
    if (!ui_->usernameEdit->text().isEmpty()) {
      proxy.setUser(ui_->usernameEdit->text());
      proxy.setPassword(ui_->passwordEdit->text());
    }
  } else {
    proxy.setType(proxy_type_);
  }
  QNetworkProxy::setApplicationProxy(proxy);
}

void GpgFrontend::UI::NetworkTab::switch_ui_enabled(bool enabled) {
  ui_->proxyServerAddressEdit->setDisabled(!enabled);
  ui_->portSpin->setDisabled(!enabled);
  ui_->proxyTypeComboBox->setDisabled(!enabled);
  ui_->usernameEdit->setDisabled(!enabled);
  ui_->passwordEdit->setDisabled(!enabled);
  ui_->checkProxyConnectionButton->setDisabled(!enabled);
  if (!enabled) proxy_type_ = QNetworkProxy::NoProxy;
}

void GpgFrontend::UI::NetworkTab::switch_ui_proxy_type(
    const QString &type_text) {
  if (type_text == "HTTP") {
    ui_->proxyServerAddressEdit->setDisabled(false);
    ui_->portSpin->setDisabled(false);
    ui_->usernameEdit->setDisabled(false);
    ui_->passwordEdit->setDisabled(false);
    proxy_type_ = QNetworkProxy::HttpProxy;
  } else if (type_text == "Socks5") {
    ui_->proxyServerAddressEdit->setDisabled(false);
    ui_->portSpin->setDisabled(false);
    ui_->usernameEdit->setDisabled(false);
    ui_->passwordEdit->setDisabled(false);
    proxy_type_ = QNetworkProxy::Socks5Proxy;
  } else {
    ui_->proxyServerAddressEdit->setDisabled(true);
    ui_->portSpin->setDisabled(true);
    ui_->usernameEdit->setDisabled(true);
    ui_->passwordEdit->setDisabled(true);
    proxy_type_ = QNetworkProxy::DefaultProxy;
  }
}
