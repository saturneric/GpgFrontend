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

#include "ModuleDeveloperPanel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleEventRegistry.h"
#include "core/module/ModuleManager.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/NativeInstances.h"

namespace GpgFrontend::UI {

namespace {

/// The flags a catalogue entry carries, as words.
auto FlagWords(const Module::ModuleEventSpec& spec) -> QStringList {
  QStringList words;
  words << (spec.layer == Module::ModuleEventLayer::kCORE ? "core" : "ui");
  words << (spec.semantics == Module::ModuleEventSemantics::kEXTEND
                ? "extension point"
                : "observe");
  if ((spec.flags & GF_EVENT_PATTERN) != 0) words << "pattern";
  if ((spec.flags & GF_EVENT_REPLY_CONSUMED) != 0) words << "reply read";
  if ((spec.flags & GF_EVENT_DEFERRABLE) != 0) words << "deferrable";
  return words;
}

auto Compact(const QCborMap& map) -> QString {
  return QString::fromUtf8(
      QJsonDocument(QCborValue(map).toJsonValue().toObject())
          .toJson(QJsonDocument::Compact));
}

enum ModuleColumn {
  kID,
  kSTATE,
  kORIGIN,
  kGATE,
  kLISTENING,
  kOWED,
  kCOMMANDS,
  kCALLS,
  kWIDGETS,
  kCOLUMNS,
};

}  // namespace

auto ParseEventParams(const QString& text, QString* error)
    -> std::optional<Module::Event::Params> {
  Module::Event::Params params;
  const auto lines = text.split(u'\n');
  for (qsizetype i = 0; i < lines.size(); ++i) {
    auto line = lines.at(i);
    if (line.endsWith(u'\r')) line.chop(1);
    if (line.trimmed().isEmpty()) continue;

    const auto fail = [&](const QString& why) {
      if (error != nullptr) *error = QString("line %1: %2").arg(i + 1).arg(why);
      return std::nullopt;
    };
    const auto split = line.indexOf(u'=');
    if (split < 0) return fail("expected key=value");
    const auto key = line.left(split).trimmed();
    if (key.isEmpty()) return fail("the key is empty");
    if (params.contains(key))
      return fail(QString("\"%1\" is given twice").arg(key));
    params.insert(key, GFBuffer(line.mid(split + 1)));
  }
  return params;
}

auto ParseCommandArgs(const QString& text, QString* error)
    -> std::optional<QCborMap> {
  if (text.trimmed().isEmpty()) return QCborMap{};
  QJsonParseError parse{};
  const auto doc = QJsonDocument::fromJson(text.toUtf8(), &parse);
  if (parse.error != QJsonParseError::NoError) {
    if (error != nullptr) {
      *error = QString("at offset %1: %2")
                   .arg(parse.offset)
                   .arg(parse.errorString());
    }
    return std::nullopt;
  }
  if (!doc.isObject()) {
    if (error != nullptr) *error = "the arguments must be one JSON object";
    return std::nullopt;
  }
  return QCborMap::fromJsonObject(doc.object());
}

auto DescribeEventAnswer(const QString& listener,
                         const Module::Event::Params& params) -> QString {
  static const QStringList kShown = {"ret", "err", "error_msg"};
  QStringList parts;
  parts << QString("ret=%1").arg(params.value("ret").ConvertToQString());
  for (const auto* key : {"err", "error_msg"}) {
    const auto value = params.value(key);
    if (!value.Empty()) {
      parts << QString("%1=\"%2\"").arg(key, value.ConvertToQString());
    }
  }
  // Everything else by name and size only: an answer may carry plaintext.
  for (auto it = params.cbegin(); it != params.cend(); ++it) {
    if (kShown.contains(it.key())) continue;
    parts << QString("%1(%2 bytes)").arg(it.key()).arg(it.value().Size());
  }
  return QString("%1: %2").arg(listener.isEmpty() ? QString("host") : listener,
                               parts.join(QStringLiteral(" ")));
}

auto DescribeCommandStatus(int status) -> QString {
  switch (status) {
    case GF_CMD_OK:
      return "OK";
    case GF_CMD_E_UNKNOWN:
      return "UNKNOWN";
    case GF_CMD_E_DENIED:
      return "DENIED";
    case GF_CMD_E_BAD_ARGS:
      return "BAD_ARGS";
    case GF_CMD_E_DISABLED:
      return "DISABLED";
    case GF_CMD_E_UNAVAILABLE:
      return "UNAVAILABLE";
    case GF_CMD_E_CANCELLED:
      return "CANCELLED";
    case GF_CMD_E_FAILED:
      return "FAILED";
    default:
      return QString("status %1").arg(status);
  }
}

ModuleDeveloperPanel::ModuleDeveloperPanel(QWidget* parent) : QWidget(parent) {
  auto* tabs = new QTabWidget(this);
  tabs->addTab(build_events_page(), tr("Events"));
  tabs->addTab(build_modules_page(), tr("Modules"));
  tabs->addTab(build_commands_page(), tr("Commands"));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(tabs);

  connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
    if (index == 1) slot_refresh_modules();
    if (index == 2) refresh_commands();
  });
}

void ModuleDeveloperPanel::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  slot_event_text_changed(event_box_->currentText());
  slot_refresh_modules();
  refresh_commands();
}

void ModuleDeveloperPanel::append_log(QPlainTextEdit* log,
                                      const QString& line) {
  log->appendPlainText(QString("[%1] %2").arg(
      QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")), line));
}

// ------------------------------------------------------------------ events

auto ModuleDeveloperPanel::build_events_page() -> QWidget* {
  auto* page = new QWidget(this);

  event_box_ = new QComboBox(page);
  event_box_->setEditable(true);
  event_box_->setInsertPolicy(QComboBox::NoInsert);
  for (const auto& spec : Module::ModuleEventCatalog()) {
    event_box_->addItem(QString::fromLatin1(spec.id));
    event_box_->setItemData(event_box_->count() - 1,
                            QString::fromUtf8(spec.summary), Qt::ToolTipRole);
  }

  event_summary_ = new QLabel(page);
  event_summary_->setWordWrap(true);
  event_listeners_ = new QLabel(page);
  event_listeners_->setWordWrap(true);
  event_listeners_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  event_params_ = new QPlainTextEdit(page);
  event_params_->setPlaceholderText(tr("One key=value per line"));
  event_params_->setMaximumHeight(96);

  fire_button_ = new QPushButton(tr("Fire"), page);

  event_log_ = new QPlainTextEdit(page);
  event_log_->setReadOnly(true);
  auto* clear = new QPushButton(tr("Clear Log"), page);

  auto* form = new QFormLayout();
  form->addRow(tr("Event"), event_box_);
  form->addRow(QString(), event_summary_);
  form->addRow(tr("Listeners"), event_listeners_);
  form->addRow(tr("Parameters"), event_params_);

  auto* buttons = new QHBoxLayout();
  buttons->addStretch();
  buttons->addWidget(clear);
  buttons->addWidget(fire_button_);

  auto* layout = new QVBoxLayout(page);
  layout->addLayout(form);
  layout->addLayout(buttons);
  layout->addWidget(event_log_, 1);

  connect(event_box_, &QComboBox::currentTextChanged, this,
          &ModuleDeveloperPanel::slot_event_text_changed);
  connect(fire_button_, &QPushButton::clicked, this,
          &ModuleDeveloperPanel::slot_fire_event);
  connect(clear, &QPushButton::clicked, event_log_, &QPlainTextEdit::clear);

  slot_event_text_changed(event_box_->currentText());
  return page;
}

void ModuleDeveloperPanel::slot_event_text_changed(const QString& text) {
  const auto id = text.trimmed();
  const auto* spec = Module::FindModuleEventSpec(id);
  const auto known = Module::IsKnownModuleEvent(id);

  if (spec == nullptr) {
    event_summary_->setText(tr("Not an event this build fires."));
  } else {
    event_summary_->setText(
        QString("%1\n[%2]")
            .arg(QString::fromUtf8(spec->summary),
                 FlagWords(*spec).join(QStringLiteral(", "))));
  }
  // A pattern entry names a family, and only a concrete id can be fired: the
  // placeholder has to be filled in first.
  fire_button_->setEnabled(known && !id.contains(u'<') && !id.contains(u'{'));

  const auto listeners = Module::ModuleManager::GetInstance().ListenersOf(id);
  event_listeners_->setText(listeners.isEmpty()
                                ? tr("No active module listens to it.")
                                : listeners.join(QStringLiteral("\n")));
}

void ModuleDeveloperPanel::slot_fire_event() {
  const auto id = event_box_->currentText().trimmed();
  QString error;
  const auto params = ParseEventParams(event_params_->toPlainText(), &error);
  if (!params.has_value()) {
    append_log(event_log_, tr("Parameters refused, %1").arg(error));
    return;
  }

  const auto listeners = Module::ModuleManager::GetInstance().ListenersOf(id);
  append_log(event_log_, tr("Fired %1 to %n listener(s)", nullptr,
                            static_cast<int>(listeners.size()))
                             .arg(id));

  // Every answer arrives here -- the Host's own failure answers included --
  // on this thread. A late one after the dialog closed is dropped.
  QPointer<ModuleDeveloperPanel> self(this);
  Module::TriggerEvent(id, *params,
                       [self](const Module::EventIdentifier&,
                              const Module::Event::ListenerIdentifier& listener,
                              const Module::Event::Params& answer) {
                         if (self == nullptr) return;
                         append_log(self->event_log_,
                                    DescribeEventAnswer(listener, answer));
                       });
}

// ------------------------------------------------------------------ modules

auto ModuleDeveloperPanel::build_modules_page() -> QWidget* {
  auto* page = new QWidget(this);

  module_table_ = new QTableWidget(0, kCOLUMNS, page);
  module_table_->setHorizontalHeaderLabels(
      {tr("Module"), tr("State"), tr("Origin"), tr("Entry Gate"),
       tr("Listening"), tr("Answers Owed"), tr("Commands"), tr("Calls"),
       tr("Widgets")});
  module_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  module_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  module_table_->verticalHeader()->setVisible(false);
  module_table_->horizontalHeader()->setStretchLastSection(true);

  auto* refresh = new QPushButton(tr("Refresh"), page);
  auto* buttons = new QHBoxLayout();
  buttons->addWidget(new QLabel(
      tr("Read-only. Activate and deactivate on the first tab."), page));
  buttons->addStretch();
  buttons->addWidget(refresh);

  auto* layout = new QVBoxLayout(page);
  layout->addLayout(buttons);
  layout->addWidget(module_table_, 1);

  connect(refresh, &QPushButton::clicked, this,
          &ModuleDeveloperPanel::slot_refresh_modules);
  return page;
}

void ModuleDeveloperPanel::slot_refresh_modules() {
  const auto modules = Module::ModuleManager::GetInstance().LifecycleSnapshot();
  auto& registry = CommandRegistry::Instance();

  module_table_->setRowCount(static_cast<int>(modules.size()));
  for (int row = 0; row < modules.size(); ++row) {
    const auto& m = modules.at(row);
    const auto set = [&](int column, const QString& text,
                         const QString& tooltip = {}) {
      auto* item = new QTableWidgetItem(text);
      if (!tooltip.isEmpty()) item->setToolTip(tooltip);
      module_table_->setItem(row, column, item);
    };
    set(kID, m.id);
    set(kSTATE, Module::ModuleLifecycleStateName(m.state));
    set(kORIGIN, m.integrated ? tr("Integrated") : tr("External"));
    set(kGATE,
        Module::ModuleEntryGate(m.id).IsClosed() ? tr("Closed") : tr("Open"));
    set(kLISTENING, QString::number(m.listening.size()),
        m.listening.join(QStringLiteral("\n")));
    set(kOWED, QString::number(m.owed_answers));
    const auto commands = registry.List(m.id + QStringLiteral("."));
    set(kCOMMANDS, QString::number(commands.size()),
        commands.join(QStringLiteral("\n")));
    set(kCALLS, QString::number(registry.PendingCallsOf(m.id)));
    set(kWIDGETS, QString::number(NativeInstances::Instance().CountFor(m.id)));
  }
  module_table_->resizeColumnsToContents();
}

// ------------------------------------------------------------------ commands

auto ModuleDeveloperPanel::build_commands_page() -> QWidget* {
  auto* page = new QWidget(this);

  command_filter_ = new QLineEdit(page);
  command_filter_->setPlaceholderText(tr("Filter commands..."));
  command_filter_->setClearButtonEnabled(true);
  command_list_ = new QListWidget(page);

  command_descriptor_ = new QPlainTextEdit(page);
  command_descriptor_->setReadOnly(true);

  command_args_ = new QPlainTextEdit(page);
  command_args_->setPlaceholderText(
      tr("Arguments as one JSON object; empty for none. Blobs cannot be "
         "given here."));
  command_args_->setMaximumHeight(96);

  invoke_button_ = new QPushButton(tr("Invoke as Host"), page);
  invoke_button_->setToolTip(
      tr("Runs the command with the Host's own authority, which no module "
         "has: capability checks do not apply."));
  invoke_button_->setEnabled(false);

  command_log_ = new QPlainTextEdit(page);
  command_log_->setReadOnly(true);
  auto* clear = new QPushButton(tr("Clear Log"), page);

  auto* left = new QVBoxLayout();
  left->addWidget(command_filter_);
  left->addWidget(command_list_, 1);

  auto* buttons = new QHBoxLayout();
  buttons->addStretch();
  buttons->addWidget(clear);
  buttons->addWidget(invoke_button_);

  auto* right = new QVBoxLayout();
  right->addWidget(command_descriptor_, 1);
  right->addWidget(command_args_);
  right->addLayout(buttons);
  right->addWidget(command_log_, 1);

  auto* layout = new QHBoxLayout(page);
  layout->addLayout(left, 1);
  layout->addLayout(right, 2);

  connect(command_filter_, &QLineEdit::textChanged, this,
          &ModuleDeveloperPanel::slot_filter_commands);
  connect(command_list_, &QListWidget::currentRowChanged, this,
          &ModuleDeveloperPanel::slot_select_command);
  connect(invoke_button_, &QPushButton::clicked, this,
          &ModuleDeveloperPanel::slot_invoke_command);
  connect(clear, &QPushButton::clicked, command_log_, &QPlainTextEdit::clear);
  return page;
}

void ModuleDeveloperPanel::refresh_commands() {
  const auto selected = command_list_->currentItem() != nullptr
                            ? command_list_->currentItem()->text()
                            : QString();
  command_list_->clear();
  command_list_->addItems(CommandRegistry::Instance().List());
  const auto found = command_list_->findItems(selected, Qt::MatchExactly);
  if (!found.isEmpty()) command_list_->setCurrentItem(found.first());
  slot_filter_commands(command_filter_->text());
}

void ModuleDeveloperPanel::slot_filter_commands(const QString& text) {
  for (int i = 0; i < command_list_->count(); ++i) {
    auto* item = command_list_->item(i);
    item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
  }
}

void ModuleDeveloperPanel::slot_select_command() {
  auto* item = command_list_->currentItem();
  const auto descriptor =
      item == nullptr ? std::nullopt
                      : CommandRegistry::Instance().Describe(item->text());
  invoke_button_->setEnabled(descriptor.has_value());
  if (!descriptor.has_value()) {
    command_descriptor_->clear();
    return;
  }
  command_descriptor_->setPlainText(
      QString("%1\n%2\n%3\n\n%4")
          .arg(CommandTitle(*descriptor), CommandCategory(*descriptor),
               CommandDescription(*descriptor),
               QString::fromUtf8(
                   QJsonDocument(
                       QCborValue(*descriptor).toJsonValue().toObject())
                       .toJson(QJsonDocument::Indented))));
}

void ModuleDeveloperPanel::slot_invoke_command() {
  auto* item = command_list_->currentItem();
  if (item == nullptr) return;
  const auto id = item->text();

  QString error;
  const auto args = ParseCommandArgs(command_args_->toPlainText(), &error);
  if (!args.has_value()) {
    append_log(command_log_, tr("Arguments refused, %1").arg(error));
    return;
  }

  // The result can arrive on the provider's thread; the log is this one's.
  QPointer<ModuleDeveloperPanel> self(this);
  const auto ticket = CommandRegistry::Instance().Invoke(
      id, *args, {}, CommandCaller{{}, 0, QStringLiteral("host")}, {},
      [self, id](gf::cmd::RawResult r) {
        QStringList blobs;
        for (const auto& b : r.blobs) {
          blobs << QString("%1 bytes").arg(b.Size());
        }
        auto line =
            QString("%1 -> %2 %3")
                .arg(id, DescribeCommandStatus(r.status), Compact(r.result));
        if (!r.error.isEmpty()) line += QString(" \"%1\"").arg(r.error);
        if (!blobs.isEmpty())
          line += QString(" blobs: %1").arg(blobs.join(", "));
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [self, line]() {
              if (self != nullptr) append_log(self->command_log_, line);
            },
            Qt::QueuedConnection);
      });
  append_log(command_log_, QString("%1 invoked: %2 (call %3)")
                               .arg(id, DescribeCommandStatus(ticket.status))
                               .arg(ticket.call_id));
}

}  // namespace GpgFrontend::UI
