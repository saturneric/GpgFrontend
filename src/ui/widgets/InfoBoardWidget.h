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
#include "ui/function/InfoBoardReport.h"
#include "ui/function/TextDirection.h"
#include "ui/struct/GpgOperaResult.h"
#include "ui/struct/InfoBoardTypes.h"

class Ui_InfoBoard;

namespace GpgFrontend::UI {

class TextEditTabWidget;
class ReportSeal;

class GF_UI_EXPORT InfoBoardWidget : public QWidget {
  Q_OBJECT

 public:
  explicit InfoBoardWidget(QWidget* parent);

  void AssociateTabWidget(QTabWidget* tab);
  void ResetOptionActionsMenu();

  void SetInfoBoard(const QString& text, InfoBoardStatus status,
                    const QString& content_hash = {});
  void SetInfoBoardCards(const QString& text, InfoBoardStatus status,
                         const QList<InfoBoardCard>& cards,
                         const QString& operation = {},
                         const QString& description = {},
                         const QString& details_title = {},
                         const QStringList& details_items = {},
                         const QString& content_hash = {});
  void SetInfoBoardFromResults(const QString& text,
                               InfoBoardStatus overall_status,
                               const QList<GpgOperaResult>& results);
  void SetInfoBoardWithOpInfo(const QString& text, InfoBoardStatus status,
                              const GpgFrontend::GpgOpResultInfo& info);

  /**
   * @brief Show @p report: the one path every posting ends in.
   *
   * The other setters normalize their input into a report and call this. It
   * stamps the report (issue time, hash, reference), logs its notes once, and
   * renders it.
   */
  void SetInfoBoardReport(InfoBoardReport report);

  /// The report on the board now; empty after a reset.
  [[nodiscard]] auto CurrentReport() const -> const InfoBoardReport& {
    return current_report_;
  }

  /// Whether any object or card of the current report is collapsed.
  [[nodiscard]] auto IsReportCollapsed() const -> bool;

  /**
   * @brief Save the document as shown, with the full report as text chunks.
   *
   * The image is the current screen, collapsed or expanded; `Comment` holds
   * the whole report and `Truncated` says whether the image leaves anything
   * out. No dialog: the caller reports a failure.
   */
  auto ExportReportPng(const QString& file_path) const -> bool;

  void InitUI();
  void UpdateActionButtons();

  /**
   * @brief Adds a flat, labelled button for @p action at the end of the tool
   *        strip, after anything a module put there.
   */
  void AddBarAction(QAction* action);

  /// The status the indicator shows right now (neutral after a reset).
  [[nodiscard]] auto CurrentStatus() const -> InfoBoardStatus {
    return current_status_;
  }

  /// The indicator's colour for @p status; neutral is an "off" grey.
  [[nodiscard]] auto IndicatorColor(InfoBoardStatus status) const -> QColor;

  /**
   * @brief Re-read the appearance settings and restyle the board's text.
   *
   * Shared with construction, so a font changed in the settings dialog takes
   * effect on the open board without a restart.
   */
  void ApplyAppearanceSettings();

  void ApplyStatusStyle(InfoBoardStatus status);

  [[nodiscard]] auto StatusTitle(InfoBoardStatus status) const -> QString;
  [[nodiscard]] auto StatusColor(InfoBoardStatus status) const -> QColor;
  [[nodiscard]] auto StatusIconPath(InfoBoardStatus status) const -> QString;
  [[nodiscard]] auto StatusIndicatorToolTip(InfoBoardStatus status) const
      -> QString;

 public slots:
  void SlotReset();
  void SlotRefresh(const QString& text, InfoBoardStatus status);
  void SlotRefreshWithCards(const QString& text, InfoBoardStatus status,
                            const QList<InfoBoardCard>& cards,
                            const QString& operation,
                            const QString& description,
                            const QString& details_title,
                            const QStringList& details_items);

 signals:
  /**
   * @brief A new, non-empty result was just posted to the board.
   *
   * Emitted once per posting call, after the board shows it; never by a reset
   * or by an update carrying nothing (see IsMeaningfulInfoBoardResult). This
   * is an event, not a state: content that is merely still there never
   * re-emits it, which is what lets a host reveal itself on news without
   * overriding a user who closed it afterwards.
   */
  void SignalResultPosted(InfoBoardStatus status);

  /**
   * @brief The indicator changed to @p status: a result, a reset, a restyle.
   *
   * State, not news: for anything that mirrors the indicator elsewhere.
   */
  void SignalStatusStyleChanged(InfoBoardStatus status);

 protected:
  /// Follows an application theme switch (see changeEvent).
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;
  /// A theme switch recolours the report already on the board.
  void changeEvent(QEvent* event) override;

 private slots:
  void slot_copy();
  void slot_save();
  void slot_open_magnifier();

 private:
  struct StyleConstants {
    static constexpr int kIndicatorSize = 14;
    /// Object groups shown before the rest collapse behind "Show all".
    static constexpr int kMaxShownGroups = 6;
    /// Cards shown per group besides its Outcome card.
    static constexpr int kMaxShownDomainCards = 4;
    static constexpr int kToolButtonSide = 24;
    static constexpr int kToolIconSize = 16;
    static constexpr int kSegButtonPadding = 16;
  };

  QSharedPointer<Ui_InfoBoard> ui_;

  InfoBoardStatus current_status_{kINFO_ERROR_NEUTRAL};

  /// Message body last shown in the raw text pane, without the status prefix.
  QString info_board_body_;

  /**
   * @brief Lays the raw text pane out the way its content reads.
   *
   * The pane is always automatic: it shows whatever an operation produced, and
   * there is nothing for a reader of a read-only status pane to override. Each
   * paragraph takes the direction of its own first letter, which this surface
   * needs more than most, since a message can answer in a different script from
   * the one the interface is in.
   */
  void apply_text_direction();

  QTextEdit* text_page_{nullptr};
  QTabWidget* tab_widget_{nullptr};
  QButtonGroup* view_group_{nullptr};

  QLabel* placeholder_label_{nullptr};
  QScrollArea* doc_scroll_{nullptr};
  QFrame* doc_frame_{nullptr};
  QLabel* header_label_{nullptr};     ///< the report's name, quiet
  QLabel* operation_label_{nullptr};  ///< what was requested
  QLabel* meta_label_{nullptr};       ///< engine and counts, quiet
  QLabel* status_label_{nullptr};
  ReportSeal* seal_{nullptr};  ///< GF branding and a status mark     ///< the
                               ///< final outcome, the focal point
  QFrame* sep_top_{nullptr};

  QLabel* summary_label_{nullptr};
  QWidget* row_details_{nullptr};
  QLabel* key_details_{nullptr};
  QWidget* details_container_{nullptr};

  QFrame* extra_sep_{nullptr};
  QWidget* extra_widget_{nullptr};
  QLabel* time_label_{nullptr};
  QLabel* id_label_{nullptr};
  QLabel* hash_label_{nullptr};

  /// The report on the board, and how much of it is expanded.
  InfoBoardReport current_report_;
  bool show_all_groups_ = false;
  QSet<QString> expanded_groups_;
  /// Inside changeEvent's restyle: the restyle's own changes are ignored.
  bool restyling_ = false;
  /// Colour every toned label, and the report, from the current palette.
  void apply_report_palette();
  /// RenderReportText() of the current report: what Copy puts out.
  QString current_copy_text_;

  void setup_view_switcher();
  void set_active_view(int index, bool persist = true);
  [[nodiscard]] auto load_persisted_view() const -> int;
  void persist_view(int index) const;

  /// Stamp, show and (when @p meaningful) announce a report.
  void post_report(InfoBoardReport report, bool meaningful);
  void init_status_page();
  void render_report();
  void render_objects();
  void render_card(QVBoxLayout* layout, QWidget* parent,
                   const InfoBoardCard& card);
  void add_expander(QVBoxLayout* layout, QWidget* parent, const QString& text,
                    const QString& action_text,
                    const std::function<void()>& on_click);

  static void delete_widgets_in_layout(QLayout* layout, int start_index = 0);

  void setup_tool_buttons();
  static void setup_separator(QFrame* separator);
  void setup_info_board();
  void setup_status_page_layout(QVBoxLayout* page_layout);
  void create_field_rows(QWidget* parent, QVBoxLayout* fields_layout);
  void create_footer(QVBoxLayout* doc_layout);

  [[nodiscard]] auto build_status_symbol(InfoBoardStatus status,
                                         const QString& word) const -> QString;
  [[nodiscard]] auto build_card_stylesheet(const QColor& color) const
      -> QString;

  [[nodiscard]] auto render_doc_pixmap(qreal scale) const -> QPixmap;
  void export_doc_as_png(const QString& file_path);

  void set_info_board_text(const QString& text, InfoBoardStatus status);

  void announce_result(const QString& text, InfoBoardStatus status,
                       qsizetype structured_items);
  auto create_card(QWidget* parent, InfoBoardStatus status) const -> QFrame*;

  void reset_document_view();
  void clear_document_fields();
};

/**
 * @brief Whether a posting carries anything for a reader.
 *
 * @param text the posted raw text; whitespace alone is nothing
 * @param structured_items number of cards, results or op-info records posted
 */
auto GF_UI_EXPORT IsMeaningfulInfoBoardResult(const QString& text,
                                              qsizetype structured_items)
    -> bool;

/**
 * @brief Shows @p host when @p board posts a new result while @p host is
 *        hidden, and leaves it alone otherwise.
 *
 * Only the posting event reveals: a reset never hides @p host, and content
 * that is still on the board never re-shows a @p host the user closed. The
 * host's own close button / toggle action stays the user's control.
 */
void GF_UI_EXPORT RevealOnNewResult(InfoBoardWidget* board, QWidget* host);

}  // namespace GpgFrontend::UI
