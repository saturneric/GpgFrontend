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

#include "SettingsNetwork.h"

#include "core/function/GlobalSettingStation.h"
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
      ui_->autoImportMissingKeyCheckBox, &QCheckBox::stateChanged, this,
      [=](int state) {
        ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(
            state == Qt::Checked
                ? Qt::Unchecked
                : ui_->forbidALLGnuPGNetworkConnectionCheckBox->checkState());
      });

  connect(ui_->forbidALLGnuPGNetworkConnectionCheckBox,
          &QCheckBox::stateChanged, this, [=](int state) {
            ui_->autoImportMissingKeyCheckBox->setCheckState(
                state == Qt::Checked
                    ? Qt::Unchecked
                    : ui_->autoImportMissingKeyCheckBox->checkState());
          });

  connect(
      ui_->proxyTypeComboBox, &QComboBox::currentTextChanged, this,
      [=](const QString &current_text) { switch_ui_proxy_type(current_text); });

  connect(ui_->checkProxyConnectionButton, &QPushButton::clicked, this,
          &NetworkTab::slot_test_proxy_connection_result);

  ui_->proxyGroupBox->setTitle(_("Proxy"));
  ui_->capabilityGroupBox->setTitle(_("Network Ability"));
  ui_->operationsGroupBox->setTitle(_("Operations"));

  ui_->enableProxyCheckBox->setText(_("Enable Proxy"));
  ui_->proxyServerPortLabel->setText(_("Port"));

  ui_->proxyServerAddressLabel->setText(_("Host Address"));
  ui_->proxyServerPortLabel->setText(_("Port"));
  ui_->proxyTypeLabel->setText(_("Proxy Type"));
  ui_->usernameLabel->setText(_("Username"));
  ui_->passwordLabel->setText(_("Password"));

  ui_->checkProxyConnectionButton->setText(
      _("Apply Proxy Settings and Check Proxy Connection"));

  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setText(
      _("Forbid all GnuPG network connection."));
  ui_->prohibitUpdateCheck->setText(
      _("Prohibit checking for version updates when the program starts."));
  ui_->autoImportMissingKeyCheckBox->setText(
      _("Automatically import a missing key for signature verification."));
  ui_->networkAbilityTipsLabel->setText(
      _("Tips: These Option Changes take effect only after the "
        "application restart."));

  SetSettings();
}

void GpgFrontend::UI::NetworkTab::SetSettings() {
  auto &settings = GlobalSettingStation::GetInstance().GetMainSettings();

  try {
    std::string proxy_host = settings.lookup("proxy.proxy_host");
    ui_->proxyServerAddressEdit->setText(proxy_host.c_str());
  } catch (...) {
    SPDLOG_ERROR("setting operation error: proxy_host");
  }

  try {
    std::string std_username = settings.lookup("proxy.username");
    ui_->usernameEdit->setText(std_username.c_str());
  } catch (...) {
    SPDLOG_ERROR("setting operation error: username");
  }

  try {
    std::string std_password = settings.lookup("proxy.password");
    ui_->passwordEdit->setText(std_password.c_str());
  } catch (...) {
    SPDLOG_ERROR("setting operation error: password");
  }

  try {
    int port = settings.lookup("proxy.port");
    ui_->portSpin->setValue(port);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: port");
  }

  ui_->proxyTypeComboBox->setCurrentText("HTTP");
  try {
    std::string proxy_type = settings.lookup("proxy.proxy_type");
    ui_->proxyTypeComboBox->setCurrentText(proxy_type.c_str());
  } catch (...) {
    SPDLOG_ERROR("setting operation error: proxy_type");
  }
  switch_ui_proxy_type(ui_->proxyTypeComboBox->currentText());

  ui_->enableProxyCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool proxy_enable = settings.lookup("proxy.enable");
    if (proxy_enable)
      ui_->enableProxyCheckBox->setCheckState(Qt::Checked);
    else
      ui_->enableProxyCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: proxy_enable");
  }

  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool forbid_all_gnupg_connection =
        settings.lookup("network.forbid_all_gnupg_connection");
    if (forbid_all_gnupg_connection)
      ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(Qt::Checked);
    else
      ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(
          Qt::Unchecked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: forbid_all_gnupg_connection");
  }

  ui_->prohibitUpdateCheck->setCheckState(Qt::Unchecked);
  try {
    bool prohibit_update_checking =
        settings.lookup("network.prohibit_update_checking");
    if (prohibit_update_checking)
      ui_->prohibitUpdateCheck->setCheckState(Qt::Checked);
    else
      ui_->prohibitUpdateCheck->setCheckState(Qt::Unchecked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: prohibit_update_checking");
  }

  ui_->autoImportMissingKeyCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool auto_import_missing_key =
        settings.lookup("network.auto_import_missing_key");
    if (auto_import_missing_key)
      ui_->autoImportMissingKeyCheckBox->setCheckState(Qt::Checked);
    else
      ui_->autoImportMissingKeyCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: auto_import_missing_key");
  }

  switch_ui_enabled(ui_->enableProxyCheckBox->isChecked());
  switch_ui_proxy_type(ui_->proxyTypeComboBox->currentText());
}

void GpgFrontend::UI::NetworkTab::ApplySettings() {
  auto &settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetMainSettings();

  if (!settings.exists("proxy") ||
      settings.lookup("proxy").getType() != libconfig::Setting::TypeGroup)
    settings.add("proxy", libconfig::Setting::TypeGroup);

  auto &proxy = settings["proxy"];

  if (!proxy.exists("proxy_host"))
    proxy.add("proxy_host", libconfig::Setting::TypeString) =
        ui_->proxyServerAddressEdit->text().toStdString();
  else {
    proxy["proxy_host"] = ui_->proxyServerAddressEdit->text().toStdString();
  }

  if (!proxy.exists("username"))
    proxy.add("username", libconfig::Setting::TypeString) =
        ui_->usernameEdit->text().toStdString();
  else {
    proxy["username"] = ui_->usernameEdit->text().toStdString();
  }

  if (!proxy.exists("password"))
    proxy.add("password", libconfig::Setting::TypeString) =
        ui_->passwordEdit->text().toStdString();
  else {
    proxy["password"] = ui_->passwordEdit->text().toStdString();
  }

  if (!proxy.exists("port"))
    proxy.add("port", libconfig::Setting::TypeInt) = ui_->portSpin->value();
  else {
    proxy["port"] = ui_->portSpin->value();
  }

  if (!proxy.exists("proxy_type"))
    proxy.add("proxy_type", libconfig::Setting::TypeString) =
        ui_->proxyTypeComboBox->currentText().toStdString();
  else {
    proxy["proxy_type"] = ui_->proxyTypeComboBox->currentText().toStdString();
  }

  if (!proxy.exists("enable"))
    proxy.add("enable", libconfig::Setting::TypeBoolean) =
        ui_->enableProxyCheckBox->isChecked();
  else {
    proxy["enable"] = ui_->enableProxyCheckBox->isChecked();
  }

  if (!settings.exists("network") ||
      settings.lookup("network").getType() != libconfig::Setting::TypeGroup)
    settings.add("network", libconfig::Setting::TypeGroup);

  auto &network = settings["network"];

  if (!network.exists("forbid_all_gnupg_connection"))
    network.add("forbid_all_gnupg_connection",
                libconfig::Setting::TypeBoolean) =
        ui_->forbidALLGnuPGNetworkConnectionCheckBox->isChecked();
  else {
    network["forbid_all_gnupg_connection"] =
        ui_->forbidALLGnuPGNetworkConnectionCheckBox->isChecked();
  }

  if (!network.exists("prohibit_update_checking"))
    network.add("prohibit_update_checking", libconfig::Setting::TypeBoolean) =
        ui_->prohibitUpdateCheck->isChecked();
  else {
    network["prohibit_update_checking"] = ui_->prohibitUpdateCheck->isChecked();
  }

  if (!network.exists("auto_import_missing_key"))
    network.add("auto_import_missing_key", libconfig::Setting::TypeBoolean) =
        ui_->autoImportMissingKeyCheckBox->isChecked();
  else {
    network["auto_import_missing_key"] =
        ui_->autoImportMissingKeyCheckBox->isChecked();
  }

  apply_proxy_settings();
}

void GpgFrontend::UI::NetworkTab::slot_test_proxy_connection_result() {
  apply_proxy_settings();

  bool ok;
  auto url = QInputDialog::getText(this, _("Test Server Url Accessibility"),
                                   tr("Server Url"), QLineEdit::Normal,
                                   "https://", &ok);
  if (ok && !url.isEmpty()) {
    auto task = new ProxyConnectionTestTask(url, 800);
    connect(task,
            &GpgFrontend::UI::ProxyConnectionTestTask::
                SignalProxyConnectionTestResult,
            this, [=](const QString &result) {
              if (result == "Reachable") {
                QMessageBox::information(this, _("Success"),
                                         _("Successfully connect to the target "
                                           "server through the proxy server."));
              } else {
                QMessageBox::critical(
                    this, _("Failed"),
                    _("Unable to connect to the target server through the "
                      "proxy server. Proxy settings may be invalid."));
              }
            });

    // Waiting Dialog
    auto *waiting_dialog = new QProgressDialog(this);
    waiting_dialog->setMaximum(0);
    waiting_dialog->setMinimum(0);
    auto waiting_dialog_label = new QLabel(
        QString(_("Test Proxy Server Connection...")) + "<br /><br />" +
        _("Is using your proxy settings to access the url. Note that this test "
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
