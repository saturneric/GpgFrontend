/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "SettingsNetwork.h"

#include "ui/function/ProxyConnectionTestThread.h"
#include "ui/settings//GlobalSettingStation.h"
#include "ui_NetworkSettings.h"

GpgFrontend::UI::NetworkTab::NetworkTab(QWidget *parent)
    : QWidget(parent), ui(std::make_shared<Ui_NetworkSettings>()) {
  ui->setupUi(this);

  connect(ui->enableProxyCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { switch_ui_enabled(state == Qt::Checked); });

  connect(
      ui->proxyTypeComboBox, &QComboBox::currentTextChanged, this,
      [=](const QString &current_text) { switch_ui_proxy_type(current_text); });

  connect(ui->checkProxyConnectionButton, &QPushButton::clicked, this,
          &NetworkTab::slotTestProxyConnectionResult);

  ui->proxyGroupBox->setTitle(_("Proxy"));
  ui->capabilityGroupBox->setTitle(_("Network Capability"));
  ui->operationsGroupBox->setTitle(_("Operations"));

  ui->enableProxyCheckBox->setText(_("Enable Proxy"));
  ui->proxyServerPortLabel->setText(_("Port"));

  ui->proxyServerAddressLabel->setText(_("Host Address"));
  ui->proxyServerPortLabel->setText(_("Port"));
  ui->proxyTypeLabel->setText(_("Proxy Type"));
  ui->usernameLabel->setText(_("Username"));
  ui->passwordLabel->setText(_("Password"));

  ui->forbidALLCheckBox->setText(_("Forbid all network connection."));
  ui->forbidALLCheckBox->setDisabled(true);

  ui->prohibitUpdateCheck->setText(
      _("Prohibit checking for version updates when the program starts."));
  ui->checkProxyConnectionButton->setText(_("Check Proxy Connection"));

  setSettings();
}

void GpgFrontend::UI::NetworkTab::setSettings() {
  auto &settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    std::string proxy_host = settings.lookup("proxy.proxy_host");
    ui->proxyServerAddressEdit->setText(proxy_host.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("proxy_host");
  }

  try {
    std::string std_username = settings.lookup("proxy.username");
    ui->usernameEdit->setText(std_username.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("username");
  }

  try {
    std::string std_password = settings.lookup("proxy.password");
    ui->passwordEdit->setText(std_password.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("password");
  }

  try {
    int port = settings.lookup("proxy.port");
    ui->portSpin->setValue(port);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("port");
  }

  ui->proxyTypeComboBox->setCurrentText("HTTP");
  try {
    std::string proxy_type = settings.lookup("proxy.proxy_type");
    ui->proxyTypeComboBox->setCurrentText(proxy_type.c_str());
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("proxy_type");
  }
  switch_ui_proxy_type(ui->proxyTypeComboBox->currentText());

  ui->enableProxyCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool proxy_enable = settings.lookup("proxy.enable");
    if (proxy_enable)
      ui->enableProxyCheckBox->setCheckState(Qt::Checked);
    else
      ui->enableProxyCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("proxy_enable");
  }

  {
    auto state = ui->enableProxyCheckBox->checkState();
    switch_ui_enabled(state == Qt::Checked);
  }

  ui->forbidALLCheckBox->setCheckState(Qt::Unchecked);
  try {
    bool forbid_all_connection =
        settings.lookup("network.forbid_all_connection");
    if (forbid_all_connection)
      ui->forbidALLCheckBox->setCheckState(Qt::Checked);
    else
      ui->forbidALLCheckBox->setCheckState(Qt::Unchecked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("forbid_all_connection");
  }

  ui->prohibitUpdateCheck->setCheckState(Qt::Unchecked);
  try {
    bool prohibit_update_checking =
        settings.lookup("network.prohibit_update_checking");
    if (prohibit_update_checking)
      ui->prohibitUpdateCheck->setCheckState(Qt::Checked);
    else
      ui->prohibitUpdateCheck->setCheckState(Qt::Unchecked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("prohibit_update_checking");
  }
}

void GpgFrontend::UI::NetworkTab::applySettings() {
  LOG(INFO) << "called";

  auto &settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("proxy") ||
      settings.lookup("proxy").getType() != libconfig::Setting::TypeGroup)
    settings.add("proxy", libconfig::Setting::TypeGroup);

  auto &proxy = settings["proxy"];

  if (!proxy.exists("proxy_host"))
    proxy.add("proxy_host", libconfig::Setting::TypeString) =
        ui->proxyServerAddressEdit->text().toStdString();
  else {
    proxy["proxy_host"] = ui->proxyServerAddressEdit->text().toStdString();
  }

  if (!proxy.exists("username"))
    proxy.add("username", libconfig::Setting::TypeString) =
        ui->usernameEdit->text().toStdString();
  else {
    proxy["username"] = ui->usernameEdit->text().toStdString();
  }

  if (!proxy.exists("password"))
    proxy.add("password", libconfig::Setting::TypeString) =
        ui->passwordEdit->text().toStdString();
  else {
    proxy["password"] = ui->passwordEdit->text().toStdString();
  }

  if (!proxy.exists("port"))
    proxy.add("port", libconfig::Setting::TypeInt) = ui->portSpin->value();
  else {
    proxy["port"] = ui->portSpin->value();
  }

  if (!proxy.exists("proxy_type"))
    proxy.add("proxy_type", libconfig::Setting::TypeString) =
        ui->proxyTypeComboBox->currentText().toStdString();
  else {
    proxy["proxy_type"] = ui->proxyTypeComboBox->currentText().toStdString();
  }

  if (!proxy.exists("enable"))
    proxy.add("enable", libconfig::Setting::TypeBoolean) =
        ui->enableProxyCheckBox->isChecked();
  else {
    proxy["enable"] = ui->enableProxyCheckBox->isChecked();
  }

  if (!settings.exists("network") ||
      settings.lookup("network").getType() != libconfig::Setting::TypeGroup)
    settings.add("network", libconfig::Setting::TypeGroup);

  auto &network = settings["network"];

  if (!network.exists("forbid_all_connection"))
    network.add("forbid_all_connection", libconfig::Setting::TypeBoolean) =
        ui->forbidALLCheckBox->isChecked();
  else {
    network["forbid_all_connection"] = ui->forbidALLCheckBox->isChecked();
  }

  if (!network.exists("prohibit_update_checking"))
    network.add("prohibit_update_checking", libconfig::Setting::TypeBoolean) =
        ui->prohibitUpdateCheck->isChecked();
  else {
    network["prohibit_update_checking"] = ui->prohibitUpdateCheck->isChecked();
  }

  apply_proxy_settings();

  LOG(INFO) << "done";
}

void GpgFrontend::UI::NetworkTab::slotTestProxyConnectionResult() {
  apply_proxy_settings();

  bool ok;
  auto url = QInputDialog::getText(this, _("Test Server Url Accessibility"),
                                   tr("Server Url"), QLineEdit::Normal,
                                   "https://", &ok);
  if (ok && !url.isEmpty()) {
    auto thread = new ProxyConnectionTestThread(url, 800, this);
    connect(thread,
            &GpgFrontend::UI::ProxyConnectionTestThread::
                signalProxyConnectionTestResult,
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
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

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
    connect(thread, &QThread::finished, [=]() {
      waiting_dialog->finished(0);
      waiting_dialog->deleteLater();
    });
    connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
      LOG(INFO) << "cancel clicked";
      if (thread->isRunning()) thread->terminate();
    });

    // Show Waiting Dialog
    waiting_dialog->show();
    waiting_dialog->setFocus();

    thread->start();
    QEventLoop loop;
    connect(thread, &QThread::finished, &loop, &QEventLoop::quit);
    loop.exec();
  }
}

void GpgFrontend::UI::NetworkTab::apply_proxy_settings() {
  // apply settings
  QNetworkProxy _proxy;
  if (ui->enableProxyCheckBox->isChecked() &&
      proxy_type_ != QNetworkProxy::DefaultProxy) {
    _proxy.setType(proxy_type_);
    _proxy.setHostName(ui->proxyServerAddressEdit->text());
    _proxy.setPort(ui->portSpin->value());
    if (!ui->usernameEdit->text().isEmpty()) {
      _proxy.setUser(ui->usernameEdit->text());
      _proxy.setPassword(ui->passwordEdit->text());
    }
  } else {
    _proxy.setType(proxy_type_);
  }

  QNetworkProxy::setApplicationProxy(_proxy);
}

void GpgFrontend::UI::NetworkTab::switch_ui_enabled(bool enabled) {
  ui->proxyServerAddressEdit->setDisabled(!enabled);
  ui->portSpin->setDisabled(!enabled);
  ui->proxyTypeComboBox->setDisabled(!enabled);
  ui->usernameEdit->setDisabled(!enabled);
  ui->passwordEdit->setDisabled(!enabled);
  ui->checkProxyConnectionButton->setDisabled(!enabled);
  if (!enabled) proxy_type_ = QNetworkProxy::NoProxy;
}

void GpgFrontend::UI::NetworkTab::switch_ui_proxy_type(
    const QString &type_text) {
  if (type_text == "HTTP") {
    ui->proxyServerAddressEdit->setDisabled(false);
    ui->portSpin->setDisabled(false);
    ui->usernameEdit->setDisabled(false);
    ui->passwordEdit->setDisabled(false);
    proxy_type_ = QNetworkProxy::HttpProxy;
  } else if (type_text == "Socks5") {
    ui->proxyServerAddressEdit->setDisabled(false);
    ui->portSpin->setDisabled(false);
    ui->usernameEdit->setDisabled(false);
    ui->passwordEdit->setDisabled(false);
    proxy_type_ = QNetworkProxy::Socks5Proxy;
  } else {
    ui->proxyServerAddressEdit->setDisabled(true);
    ui->portSpin->setDisabled(true);
    ui->usernameEdit->setDisabled(true);
    ui->passwordEdit->setDisabled(true);
    proxy_type_ = QNetworkProxy::DefaultProxy;
  }
}
