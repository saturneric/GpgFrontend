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

#pragma once

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "core/typedef/CoreTypedef.h"
#include "ui/struct/GpgOperaResult.h"

class Ui_InfoBoard;

namespace GpgFrontend::UI {

class TextEditTabWidget;
struct InfoBoardCard;

// NOTE: keep this typed enum immediately before InfoBoardWidget, and do not
// insert any braced declaration (struct/class definition) between the two.
// lupdate's C++ parser mis-tracks the namespace scope across a typed enum
// followed by a braced declaration, which strips the GpgFrontend::UI:: prefix
// from this class's translation context and breaks every tr() lookup at
// runtime. InfoBoardCard is therefore defined below the class instead of here.
enum InfoBoardStatus : uint8_t {
  kINFO_ERROR_OK = 0,
  kINFO_ERROR_WARN = 1,
  kINFO_ERROR_CRITICAL = 2,
  kINFO_ERROR_NEUTRAL = 3,
};

class InfoBoardWidget : public QWidget {
  Q_OBJECT

 public:
  explicit InfoBoardWidget(QWidget* parent);

  void AssociateTabWidget(QTabWidget* tab);
  void ResetOptionActionsMenu();

  void SetInfoBoard(const QString& text, InfoBoardStatus status,
                    const QString& content_hash = {});
  void SetInfoBoardCards(const QString& text, InfoBoardStatus status,
                         const QContainer<InfoBoardCard>& cards,
                         const QString& operation = {},
                         const QString& description = {},
                         const QString& details_title = {},
                         const QStringList& details_items = {},
                         const QString& content_hash = {});
  void SetInfoBoardFromResults(const QString& text,
                               InfoBoardStatus overall_status,
                               const QContainer<GpgOperaResult>& results);
  void SetInfoBoardWithOpInfo(const QString& text, InfoBoardStatus status,
                              const GpgFrontend::GpgOpResultInfo& info);

  void InitUI();
  void UpdateActionButtons();

  void ApplyStatusStyle(InfoBoardStatus status);

  [[nodiscard]] auto StatusTitle(InfoBoardStatus status) const -> QString;
  [[nodiscard]] auto StatusColor(InfoBoardStatus status) const -> QColor;
  [[nodiscard]] auto StatusIconPath(InfoBoardStatus status) const -> QString;
  [[nodiscard]] auto StatusDescription(InfoBoardStatus status,
                                       const QString& operation) const
      -> QString;

 public slots:
  void SlotReset();
  void SlotRefresh(const QString& text, InfoBoardStatus status);
  void SlotRefreshWithCards(const QString& text, InfoBoardStatus status,
                            const QContainer<InfoBoardCard>& cards,
                            const QString& operation,
                            const QString& description,
                            const QString& details_title,
                            const QStringList& details_items);

 private slots:
  void slot_copy();
  void slot_save();
  void slot_open_magnifier();

 private:
  struct StyleConstants {
    static constexpr int kStampSize = 60;
    static constexpr int kIndicatorSize = 14;
    static constexpr int kKeyColumnWidth = 86;
    static constexpr int kCardKeyWidth = 76;
    static constexpr int kMaxResultCards = 6;
  };

  QSharedPointer<Ui_InfoBoard> ui_;

  QTextEdit* text_page_{nullptr};
  QTabWidget* tab_widget_{nullptr};
  QButtonGroup* view_group_{nullptr};

  QLabel* placeholder_label_{nullptr};
  QScrollArea* doc_scroll_{nullptr};
  QFrame* doc_frame_{nullptr};
  QLabel* stamp_label_{nullptr};
  QLabel* header_label_{nullptr};
  QFrame* sep_top_{nullptr};

  QFrame* row_operation_{nullptr};
  QLabel* val_operation_{nullptr};
  QFrame* row_status_{nullptr};
  QLabel* val_status_{nullptr};
  QFrame* row_details_{nullptr};
  QLabel* key_details_{nullptr};
  QWidget* details_container_{nullptr};
  QFrame* row_engine_{nullptr};
  QLabel* val_engine_{nullptr};

  QFrame* extra_sep_{nullptr};
  QWidget* extra_widget_{nullptr};
  QLabel* time_label_{nullptr};
  QLabel* id_label_{nullptr};
  QLabel* hash_label_{nullptr};
  QString current_id_;
  QString current_input_hash_;
  QString current_copy_text_;

  void setup_view_switcher();
  void set_active_view(int index, bool persist = true);
  [[nodiscard]] auto load_persisted_view() const -> int;
  void persist_view(int index) const;

  void init_status_page();
  void update_status_page(const QString& text, InfoBoardStatus status,
                          const QString& content_hash = {},
                          const QString& operation = {},
                          const QString& description = {},
                          const QContainer<InfoBoardCard>& cards = {},
                          const QString& details_title = {},
                          const QStringList& details_items = {});
  void update_doc_header(InfoBoardStatus status, const QString& operation,
                         const QString& engine, const QString& count_summary);
  void update_status_page_from_results(
      InfoBoardStatus status, const QContainer<GpgOperaResult>& results);
  auto populate_extra_from_op_info(QVBoxLayout* layout, QWidget* parent,
                                   const GpgFrontend::GpgOpResultInfo& info)
      -> bool;
  void render_cards(QVBoxLayout* layout, QWidget* parent,
                    const QContainer<InfoBoardCard>& cards);

  static auto make_stamp_pixmap(const QColor& color, const QString& icon_path,
                                int size) -> QPixmap;
  static void delete_widgets_in_layout(QLayout* layout, int start_index = 0);

  void setup_tool_buttons();
  void setup_info_board();
  void setup_status_page_layout(QVBoxLayout* page_layout);
  void create_field_rows(QWidget* parent, QVBoxLayout* fields_layout);
  void create_footer(QVBoxLayout* doc_layout);

  [[nodiscard]] auto build_status_symbol(InfoBoardStatus status) const
      -> QString;
  [[nodiscard]] auto build_header_html(InfoBoardStatus status,
                                       const QString& subtitle,
                                       const QString& accent) const -> QString;
  [[nodiscard]] auto build_card_stylesheet(const QColor& color) const
      -> QString;
  [[nodiscard]] auto build_chip_stylesheet(const QColor& color) const
      -> QString;

  void populate_details_section(const GpgFrontend::GpgOpResultInfo& info,
                                InfoBoardStatus status);
  void populate_details_section_generic(const QString& title,
                                        const QStringList& items,
                                        InfoBoardStatus status,
                                        const QString& description = {});
  void populate_extra_section(const GpgFrontend::GpgOpResultInfo& info,
                              InfoBoardStatus status);

  [[nodiscard]] auto render_doc_pixmap(qreal scale) const -> QPixmap;
  void export_doc_as_png(const QString& file_path);

  void set_info_board_text(const QString& text, InfoBoardStatus status);
  auto create_card(QWidget* parent, InfoBoardStatus status) const -> QFrame*;
  void add_card_header(QVBoxLayout* card_layout, QWidget* parent,
                       InfoBoardStatus status, const QString& title) const;
  void add_card_field(QVBoxLayout* card_layout, QWidget* parent,
                      const QString& key, const QString& value) const;
  auto create_detail_chip(QWidget* parent, const QString& text,
                          InfoBoardStatus status) const -> QLabel*;

  [[nodiscard]] auto build_op_info_copy_lines(
      const GpgFrontend::GpgOpResultInfo& info) const -> QStringList;

  void reset_document_view();
  void clear_document_fields();
};

// Defined here (rather than above the class) to keep the typed InfoBoardStatus
// enum adjacent to InfoBoardWidget; see the note on that enum.
struct InfoBoardCard {
  QString title;
  InfoBoardStatus status{kINFO_ERROR_NEUTRAL};
  QContainer<QPair<QString, QString>> fields;
};

}  // namespace GpgFrontend::UI
