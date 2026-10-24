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

// How the Status Panel renders its one report model: the shared card layout
// with the source at the right of the header, what is collapsed and how it
// expands, and that Copy, the Details view and the PNG carry the full report.
//
// gtest bodies run on a worker thread here, and a widget may only be built on
// the GUI thread, so every case runs there with a blocking hop.

#include <gtest/gtest.h>

#include <QClipboard>
#include <QFile>
#include <QImageReader>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QToolButton>
#include <QtEndian>

#include "GpgFrontendTest.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/InfoBoardReport.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/InfoBoardDocFrame.h"
#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::Test {

namespace {

using UI::GpgOperaResult;
using UI::InfoBoardWidget;

template <typename F>
void OnGuiThread(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

auto Key(const char* text) -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport", text);
}

/// A laid-out board that is never put on a screen.
struct RenderFixture {
  QWidget window;
  InfoBoardWidget* board = new InfoBoardWidget(&window);

  RenderFixture() {
    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(board);
    // The Status view, whatever view the profile last remembered: these
    // tests look at the document, not the raw text.
    board->findChild<QStackedWidget*>(u"stackedWidget"_s)->setCurrentIndex(0);
    window.setAttribute(Qt::WA_DontShowOnScreen);
    window.resize(900, 2400);
    window.show();
    Settle();
  }

  static void Settle() {
    for (int i = 0; i < 5; ++i) {
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
      QCoreApplication::processEvents();
    }
  }

  [[nodiscard]] auto Cards() const -> QList<QFrame*> {
    QList<QFrame*> cards;
    for (auto* f : board->findChildren<QFrame*>(u"DocResultRow"_s)) {
      if (f->isVisibleTo(board)) cards.append(f);
    }
    return cards;
  }

  [[nodiscard]] auto CardTitles() const -> QStringList {
    QStringList titles;
    for (auto* card : Cards()) {
      if (auto* t = card->findChild<QLabel*>(u"InfoBoardCardTitle"_s)) {
        titles << t->text();
      }
    }
    return titles;
  }

  [[nodiscard]] auto Value(const QString& key) const -> QLabel* {
    for (auto* l : board->findChildren<QLabel*>(u"InfoBoardCardValue"_s)) {
      if (l->property("fieldKey").toString() == key && l->isVisibleTo(board)) {
        return l;
      }
    }
    return nullptr;
  }

  void ClickExpander(int which = 0) const {
    QList<QToolButton*> buttons;
    for (auto* row : board->findChildren<QWidget*>(u"InfoBoardExpander"_s)) {
      if (!row->isVisibleTo(board)) continue;
      buttons.append(row->findChildren<QToolButton*>());
    }
    ASSERT_GT(buttons.size(), which);
    buttons.at(which)->click();
    Settle();
  }
};

auto FileResult(int status, int index, const QString& path,
                QList<GpgOpStageError> errors = {}) -> GpgOperaResult {
  GpgOpResultInfo info;
  info.status = status;
  info.op = GpgOperation::kENCRYPT;
  info.operation = GpgOperationName(info.op);
  info.stageErrors = std::move(errors);
  GpgOperaResult r(info, QFileInfo(path).fileName());
  r.input_index = index;
  r.source_path = path;
  return r;
}

/**
 * @brief The text chunks of a PNG, read straight from the file.
 *
 * QImageReader only sees the chunks before the image data and folds line
 * breaks into spaces, so it cannot show whether the report survived intact.
 */
auto PngText(const QString& path) -> QMap<QString, QString> {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  const auto data = file.readAll();

  // Inflate a zlib stream; qUncompress wants the expected size up front.
  const auto inflate = [](const QByteArray& z) -> QByteArray {
    QByteArray framed(4, '\0');
    qToBigEndian<quint32>(1U << 20, framed.data());
    return qUncompress(framed + z);
  };

  QMap<QString, QString> text;
  qsizetype i = 8;
  while (i + 12 <= data.size()) {
    const auto n = qFromBigEndian<quint32>(data.constData() + i);
    const auto type = data.mid(i + 4, 4);
    const auto body = data.mid(i + 8, n);
    const auto nul = body.indexOf('\0');
    const auto key = QString::fromLatin1(body.left(nul));
    const auto rest = body.mid(nul + 1);
    if (type == "tEXt") {
      text[key] = QString::fromLatin1(rest);
    } else if (type == "zTXt") {
      text[key] = QString::fromLatin1(inflate(rest.mid(1)));
    } else if (type == "iTXt") {
      const bool compressed = rest.at(0) != 0;
      auto tail = rest.mid(2);
      tail = tail.mid(tail.indexOf('\0') + 1);  // language tag
      tail = tail.mid(tail.indexOf('\0') + 1);  // translated keyword
      text[key] = QString::fromUtf8(compressed ? inflate(tail) : tail);
    }
    i += 12 + static_cast<qsizetype>(n);
  }
  return text;
}

auto EngineError(const QString& text, const QString& detail)
    -> GpgOpStageError {
  GpgOpStageError e;
  e.scope = GpgOperation::kENCRYPT;
  e.code = GPG_ERR_GENERAL;
  e.codeText = text;
  e.engineDetails.append({GpgOperation::kENCRYPT, detail});
  return e;
}

}  // namespace

TEST(GFUiInfoBoardRenderTest, EverySourceSitsAtTheRightOfItsCardsHeader) {
  OnGuiThread([] {
    RenderFixture f;
    GpgOpResultInfo info;
    info.status = 1;
    info.op = GpgOperation::kVERIFY;
    GpgVerifySigInfo sig;
    sig.validity = GpgSigValidity::kFULLY_VALID;
    info.signatures.append(sig);
    GpgOperaResult r(info, u"signed.txt"_s);
    r.input_index = 0;
    r.source_path = u"/home/u/signed.txt"_s;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK, {r});
    RenderFixture::Settle();

    const auto cards = f.Cards();
    ASSERT_GE(cards.size(), 2) << "an Outcome card and a signature card";
    for (auto* card : cards) {
      auto* title = card->findChild<QLabel*>(u"InfoBoardCardTitle"_s);
      auto* source = card->findChild<QLabel*>(u"InfoBoardCardSource"_s);
      ASSERT_NE(title, nullptr);
      ASSERT_NE(source, nullptr);

      // In the card's coordinates: both sit inside the header row.
      const QRect t(title->mapTo(card, QPoint(0, 0)), title->size());
      const QRect s(source->mapTo(card, QPoint(0, 0)), source->size());
      const auto right = card->contentsRect().right() -
                         card->layout()->contentsMargins().right();
      EXPECT_GT(s.x(), t.x());
      EXPECT_LE(std::abs(s.right() - right), 2) << "right-aligned";
      EXPECT_LT(std::abs(s.center().y() - t.center().y()), t.height())
          << "same row as the title";
      EXPECT_EQ(source->toolTip(), u"/home/u/signed.txt"_s);
    }
  });
}

TEST(GFUiInfoBoardRenderTest,
     ALongSourceIsCutInTheMiddleAndLeavesTheTitleRoom) {
  OnGuiThread([] {
    RenderFixture f;
    const auto long_path = u"/"_s + QString(300, u'd') + u"/end-of-path.txt"_s;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK,
                                     {FileResult(1, 0, long_path)});
    RenderFixture::Settle();

    const auto cards = f.Cards();
    ASSERT_FALSE(cards.isEmpty());
    auto* card = cards.first();
    auto* title = card->findChild<QLabel*>(u"InfoBoardCardTitle"_s);
    auto* source = card->findChild<QLabel*>(u"InfoBoardCardSource"_s);
    ASSERT_NE(source, nullptr);

    EXPECT_TRUE(source->text().contains(u"…"_s))
        << source->text().toStdString();
    EXPECT_TRUE(source->text().endsWith(u".txt"_s)) << "cut in the middle";
    EXPECT_EQ(source->toolTip(), long_path);
    EXPECT_LE(source->width(), static_cast<int>(card->width() * 0.4) + 1);
    EXPECT_GE(title->width() + source->width(), source->width());
    EXPECT_GE(title->width(),
              static_cast<int>(card->contentsRect().width() * 0.5));
  });
}

TEST(GFUiInfoBoardRenderTest, ResultReasonAndEngineErrorsAreShownInFull) {
  OnGuiThread([] {
    RenderFixture f;
    const auto detail = u"detail "_s.repeated(80).trimmed();
    f.board->SetInfoBoardFromResults(
        {}, UI::kINFO_ERROR_CRITICAL,
        {FileResult(-1, 0, u"/in/a.txt"_s,
                    {EngineError(u"No secret key"_s, detail)})});
    RenderFixture::Settle();

    auto* result = f.Value(Key("Result"));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->text(), UI::OutcomeText(UI::InfoBoardOutcome::kFAILED));

    // One object: the summary says the reason, so the card does not repeat
    // it on screen; Copy still has it.
    auto* summary = f.board->findChild<QLabel*>(u"InfoBoardSummary"_s);
    ASSERT_NE(summary, nullptr);
    const auto& object = f.board->CurrentReport().objects.first();
    EXPECT_TRUE(summary->text().contains(object.reason));
    EXPECT_EQ(f.Value(Key("Reason")), nullptr);
    // A row of the copy text: the label, then the reason in the value
    // column.
    const auto copy = UI::RenderReportText(f.board->CurrentReport());
    EXPECT_TRUE(copy.contains(
        QRegularExpression(QRegularExpression::escape(Key("Reason") + u":"_s) +
                           u" +"_s + QRegularExpression::escape(object.reason))));

    auto* error = f.Value(Key("Engine error"));
    ASSERT_NE(error, nullptr);
    EXPECT_TRUE(error->text().startsWith(u"No secret key"_s));

    auto* shown_detail = f.Value(Key("Engine detail"));
    ASSERT_NE(shown_detail, nullptr);
    EXPECT_EQ(shown_detail->text(), detail) << "never elided";
    EXPECT_TRUE(shown_detail->wordWrap());
  });
}

TEST(GFUiInfoBoardRenderTest, ItemsBeyondTheFirstSixExpandWithOneClick) {
  OnGuiThread([] {
    RenderFixture f;
    QList<GpgOperaResult> results;
    for (int i = 0; i < 8; ++i) {
      results.append(
          FileResult(-1, i, u"/in/f%1.txt"_s.arg(i),
                     {EngineError(u"err-%1"_s.arg(i), u"detail-%1"_s.arg(i))}));
    }
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_CRITICAL, results);
    RenderFixture::Settle();

    EXPECT_EQ(f.CardTitles().count(Key("Outcome")), 6);
    EXPECT_TRUE(f.board->IsReportCollapsed());

    f.ClickExpander();
    EXPECT_EQ(f.CardTitles().count(Key("Outcome")), 8);
    EXPECT_FALSE(f.board->IsReportCollapsed());
    // An error of a formerly hidden item is now on screen.
    bool seen = false;
    for (auto* l : f.board->findChildren<QLabel*>(u"InfoBoardCardValue"_s)) {
      if (l->isVisibleTo(f.board) && l->text() == u"detail-7"_s) seen = true;
    }
    EXPECT_TRUE(seen);

    // The next posting starts collapsed again.
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_CRITICAL, results);
    RenderFixture::Settle();
    EXPECT_EQ(f.CardTitles().count(Key("Outcome")), 6);
  });
}

TEST(GFUiInfoBoardRenderTest, AnItemsManyCardsExpandWithinItsGroup) {
  OnGuiThread([] {
    RenderFixture f;
    GpgOpResultInfo info;
    info.status = 1;
    info.op = GpgOperation::kVERIFY;
    for (int i = 0; i < 7; ++i) {
      GpgVerifySigInfo sig;
      sig.validity = GpgSigValidity::kFULLY_VALID;
      sig.signer.keyId = u"KEY%1"_s.arg(i);
      info.signatures.append(sig);
    }
    GpgOperaResult r(info);
    r.input_index = 0;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK, {r});
    RenderFixture::Settle();

    EXPECT_EQ(f.Cards().size(), 1 + 4);
    EXPECT_TRUE(f.board->IsReportCollapsed());
    f.ClickExpander();
    EXPECT_EQ(f.Cards().size(), 1 + 7);
    EXPECT_FALSE(f.board->IsReportCollapsed());
  });
}

TEST(GFUiInfoBoardRenderTest, CopyDetailsAndPngCarryTheWholeReport) {
  OnGuiThread([] {
    RenderFixture f;
    QList<GpgOperaResult> results;
    for (int i = 0; i < 8; ++i) {
      results.append(
          FileResult(-1, i, u"/in/f%1.txt"_s.arg(i),
                     {EngineError(u"err-%1"_s.arg(i), u"detail-%1"_s.arg(i))}));
    }
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_CRITICAL, results);
    RenderFixture::Settle();

    const auto& report = f.board->CurrentReport();
    const auto full = UI::RenderReportText(report);
    EXPECT_TRUE(full.contains(u"detail-7"_s));

    // Copy, while collapsed.
    f.board->findChild<QToolButton*>(u"copyToolButton"_s)->click();
    EXPECT_EQ(QGuiApplication::clipboard()->text(), full);

    // Details view.
    EXPECT_TRUE(f.board->findChild<QTextBrowser*>(u"infoBoard"_s)
                    ->toPlainText()
                    .contains(UI::RenderRawDetails(report)));

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const auto collapsed_png = dir.filePath(u"collapsed.png"_s);
    ASSERT_TRUE(f.board->ExportReportPng(collapsed_png));
    const auto collapsed = PngText(collapsed_png);
    EXPECT_EQ(collapsed.value(u"Comment"_s), full);
    EXPECT_EQ(collapsed.value(u"Truncated"_s), u"true"_s);
    EXPECT_EQ(collapsed.value(u"Document ID"_s), report.ref_id);
    EXPECT_EQ(collapsed.value(u"Document Hash"_s), report.report_hash);

    f.ClickExpander();
    const auto expanded_png = dir.filePath(u"expanded.png"_s);
    ASSERT_TRUE(f.board->ExportReportPng(expanded_png));
    const auto expanded = PngText(expanded_png);
    EXPECT_EQ(expanded.value(u"Truncated"_s), u"false"_s);
    EXPECT_EQ(expanded.value(u"Comment"_s), full);
    EXPECT_GT(QImageReader(expanded_png).size().height(),
              QImageReader(collapsed_png).size().height())
        << "the image is what is on screen";
  });
}

TEST(GFUiInfoBoardRenderTest, ACancelledPostingNeverRendersAnEmptyCard) {
  OnGuiThread([] {
    RenderFixture f;
    GpgOpResultInfo info;
    info.status = -1;
    info.op = GpgOperation::kENCRYPT;
    GpgOpStageError e;
    e.scope = GpgOperation::kENCRYPT;
    e.code = GPG_ERR_CANCELED;
    e.codeText = u"Operation cancelled"_s;
    info.stageErrors.append(e);
    f.board->SetInfoBoardWithOpInfo(u"# Operation Cancelled"_s,
                                    UI::kINFO_ERROR_CRITICAL, info);
    RenderFixture::Settle();

    EXPECT_EQ(f.board->CurrentStatus(), UI::kINFO_ERROR_WARN);
    auto* summary = f.board->findChild<QLabel*>(u"InfoBoardSummary"_s);
    ASSERT_NE(summary, nullptr);
    EXPECT_FALSE(summary->text().isEmpty());
    // "Cancelled" is the headline; the card does not say it a third time.
    EXPECT_EQ(f.Value(Key("Reason")), nullptr);
    EXPECT_EQ(f.Value(Key("Result"))->text(),
              UI::OutcomeText(UI::InfoBoardOutcome::kCANCELLED));
    ASSERT_NE(f.Value(Key("Engine error")), nullptr);
    auto* source = f.board->findChild<QLabel*>(u"InfoBoardCardSource"_s);
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(source->toolTip(), Key("Current document"));
  });
}

namespace {

/// A report built to stress the layout: a combined call with a long stage
/// name, a long source path, multi-line engine details, and a label far
/// longer than any English one, as a translation may be.
void PostDemandingReport(InfoBoardWidget* board) {
  GpgOpResultInfo info;
  info.status = -1;
  info.op = GpgOperation::kENCRYPT_SIGN;
  info.inputHash = QString(64, u'A');
  GpgOpStageError e;
  e.scope = GpgOperation::kENCRYPT_SIGN;
  e.code = GPG_ERR_BAD_PASSPHRASE;
  e.codeText = u"Bad passphrase"_s;
  e.engineDetails.append(
      {GpgOperation::kSIGN,
       u"gpg: signing failed: Bad passphrase\n"
       "gpg: [stdin]: sign+encrypt failed: Bad passphrase\n"_s +
           u"a-very-long-unbroken-token-"_s.repeated(6)});
  e.engineDetails.append({GpgOperation::kENCRYPT, u"recipient ok"_s});
  info.stageErrors.append(e);

  GpgOperaResult r(info, u"report.txt"_s);
  r.input_index = 0;
  r.source_path =
      u"/home/user/"_s + u"nested-directory/"_s.repeated(12) + u"report.txt"_s;

  UI::InfoBoardBatchContext ctx;
  UI::InfoBoardCard translated;
  translated.title = u"Ausführliche Angaben"_s;
  translated.AddField(
      u"Ausführliche Fehlerbeschreibung des Verschlüsselungsmoduls"_s,
      u"Wert"_s);
  ctx.extra_cards.insert(0, {translated});

  board->SetInfoBoardReport(UI::BuildReportFromResults({r}, ctx));
}

}  // namespace

TEST(GFUiInfoBoardRenderTest, FieldsStayReadableAtEveryWidth) {
  OnGuiThread([] {
    for (const int width : {340, 640, 1100}) {
      RenderFixture f;
      f.window.resize(width, 2400);
      PostDemandingReport(f.board);
      RenderFixture::Settle();
      SCOPED_TRACE(width);

      const auto rows =
          f.board->findChildren<QWidget*>(u"InfoBoardCardField"_s);
      ASSERT_FALSE(rows.isEmpty());
      for (auto* row : rows) {
        if (!row->isVisibleTo(f.board)) continue;
        const auto children =
            row->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        ASSERT_EQ(children.size(), 2);
        auto* key = qobject_cast<QLabel*>(children[0]);
        auto* value = children[1];
        ASSERT_NE(key, nullptr);

        // Never overlapping, never past the card.
        EXPECT_FALSE(key->geometry().intersects(value->geometry()))
            << key->text().toStdString();
        EXPECT_LE(value->geometry().right(), row->width());
        EXPECT_TRUE(key->alignment().testFlag(Qt::AlignLeft));

        const bool stacked = key->geometry().bottom() < value->geometry().top();
        if (width >= 640) {
          // The label column is bounded whatever the label says.
          EXPECT_FALSE(stacked);
          EXPECT_LE(key->width(),
                    key->fontMetrics().averageCharWidth() * 15 + 1);
        }
        if (width < 400) {
          EXPECT_TRUE(stacked);
        }

        // Nothing clipped: every label is as tall as its text needs.
        for (auto* l : row->findChildren<QLabel*>()) {
          if (!l->wordWrap() || l->width() <= 0) continue;
          EXPECT_GE(l->height(), l->heightForWidth(l->width()))
              << l->text().left(40).toStdString();
        }
      }

      // The source caption never takes the title's room.
      for (auto* card : f.Cards()) {
        auto* source = card->findChild<QLabel*>(u"InfoBoardCardSource"_s);
        auto* title = card->findChild<QLabel*>(u"InfoBoardCardTitle"_s);
        if (source == nullptr) continue;
        EXPECT_LE(source->width(), static_cast<int>(card->width() * 0.4) + 1);
        const QRect s(source->mapTo(card, QPoint(0, 0)), source->size());
        const QRect t(title->mapTo(card, QPoint(0, 0)), title->size());
        EXPECT_FALSE(s.intersects(t));
      }
    }
  });
}

TEST(GFUiInfoBoardRenderTest,
     MessagesAreSetForReadingAndIdentifiersMonospaced) {
  OnGuiThread([] {
    RenderFixture f;
    PostDemandingReport(f.board);
    RenderFixture::Settle();

    auto* message = f.Value(Key("Engine error"));
    auto* hash = f.Value(Key("Input SHA-256"));
    ASSERT_NE(message, nullptr);
    ASSERT_NE(hash, nullptr);
    EXPECT_NE(QFontInfo(message->font()).fixedPitch(),
              QFontInfo(hash->font()).fixedPitch());
    EXPECT_TRUE(QFontInfo(hash->font()).fixedPitch());

    // The label is short; what the error belongs to is the line under it.
    auto* note =
        message->parentWidget()->findChild<QLabel*>(u"InfoBoardCardNote"_s);
    ASSERT_NE(note, nullptr);
    EXPECT_TRUE(
        note->text().contains(GpgOperationName(GpgOperation::kENCRYPT_SIGN)));
  });
}

TEST(GFUiInfoBoardRenderTest, TheFooterShortensTheHashButKeepsItWhole) {
  OnGuiThread([] {
    RenderFixture f;
    PostDemandingReport(f.board);
    RenderFixture::Settle();

    const auto full_hash = f.board->CurrentReport().report_hash;
    auto* footer = f.board->findChild<QWidget*>(u"InfoBoardFooter"_s);
    ASSERT_NE(footer, nullptr);
    QLabel* hash = nullptr;
    for (auto* l : footer->findChildren<QLabel*>()) {
      if (l->toolTip() == full_hash) hash = l;
    }
    ASSERT_NE(hash, nullptr);
    EXPECT_FALSE(hash->text().contains(full_hash));
    EXPECT_TRUE(hash->text().contains(full_hash.left(16)));
    EXPECT_TRUE(
        UI::RenderReportText(f.board->CurrentReport()).contains(full_hash));

    // Narrow, the footer items stack instead of overlapping.
    f.window.resize(360, 2400);
    RenderFixture::Settle();
    const auto items = footer->findChildren<QLabel*>();
    for (int i = 0; i < items.size(); ++i) {
      for (int j = i + 1; j < items.size(); ++j) {
        EXPECT_FALSE(items[i]->geometry().intersects(items[j]->geometry()));
      }
    }
  });
}

TEST(GFUiInfoBoardRenderTest, EachItemOfABatchKeepsItsOwnReasonOnScreen) {
  OnGuiThread([] {
    RenderFixture f;
    f.board->SetInfoBoardFromResults(
        {}, UI::kINFO_ERROR_CRITICAL,
        {FileResult(-1, 0, u"/in/a"_s, {EngineError(u"e1"_s, u"d1"_s)}),
         FileResult(1, 1, u"/in/b"_s)});
    RenderFixture::Settle();
    // The summary counts; each item's reason is its own and stays.
    EXPECT_NE(f.Value(Key("Reason")), nullptr);
  });
}

TEST(GFUiInfoBoardRenderTest, IdentitiesThatLookLikeMarkupShowInFull) {
  OnGuiThread([] {
    RenderFixture f;
    GpgOpResultInfo info;
    info.status = 1;
    info.op = GpgOperation::kVERIFY;
    info.details = {u"Alice <alice@example.org>"_s};
    GpgVerifySigInfo sig;
    sig.validity = GpgSigValidity::kFULLY_VALID;
    sig.signer.uid = u"Alice <alice@example.org>"_s;
    info.signatures.append(sig);
    GpgOperaResult r(info);
    r.input_index = 0;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK, {r});
    RenderFixture::Settle();

    int shown = 0;
    for (auto* l : f.board->findChildren<QLabel*>()) {
      if (l->isVisibleTo(f.board) &&
          l->text() == u"Alice <alice@example.org>"_s) {
        EXPECT_EQ(l->textFormat(), Qt::PlainText);
        ++shown;
      }
    }
    EXPECT_GE(shown, 2) << "the signer chip and the signature card";
  });
}

TEST(GFUiInfoBoardRenderTest, AThemeSwitchRecoloursTheReportOnTheBoard) {
  OnGuiThread([] {
    RenderFixture f;
    f.board->SetInfoBoardFromResults(
        {}, UI::kINFO_ERROR_CRITICAL,
        {FileResult(-1, 0, u"/in/a.txt"_s, {EngineError(u"e"_s, u"d"_s)})});
    RenderFixture::Settle();

    const auto light = UI::ReportColors::From(f.board->palette());
    auto* status = f.board->findChild<QLabel*>(u"InfoBoardStatus"_s);
    const auto light_status = status->styleSheet();

    // The way the application switches theme.
    const auto original = QApplication::palette();
    QApplication::setPalette(UI::DarkFusionPalette());
    RenderFixture::Settle();

    const auto dark = UI::ReportColors::From(f.board->palette());
    EXPECT_TRUE(dark.dark);
    EXPECT_FALSE(light.dark);

    // Cards: a surface a step lighter than the charcoal panel.
    const auto cards = f.Cards();
    ASSERT_FALSE(cards.isEmpty());
    EXPECT_TRUE(cards.first()->styleSheet().contains(dark.surface.name()));
    EXPECT_GT(dark.surface.lightness(),
              f.board->palette().color(QPalette::Window).lightness());

    // Toned text follows: titles readable, labels muted, never the old ones.
    auto* title = cards.first()->findChild<QLabel*>(u"InfoBoardCardTitle"_s);
    EXPECT_TRUE(title->styleSheet().contains(dark.text.name()));
    auto* brand = f.board->findChild<QLabel*>(u"InfoBoardBrand"_s);
    EXPECT_TRUE(brand->styleSheet().contains(dark.muted.name()));
    EXPECT_FALSE(brand->styleSheet().contains(light.muted.name()));
    auto* name = f.board->findChild<QLabel*>(u"InfoBoardReportName"_s);
    EXPECT_TRUE(name->styleSheet().contains(dark.text.name()));

    // Status text takes the lighter shade on a dark panel.
    EXPECT_NE(status->styleSheet(), light_status);
    EXPECT_TRUE(status->styleSheet().contains(
        f.board->StatusColor(UI::kINFO_ERROR_CRITICAL).name()));

    // And back.
    QApplication::setPalette(original);
    RenderFixture::Settle();
    EXPECT_EQ(status->styleSheet(), light_status);
  });
}

TEST(GFUiInfoBoardRenderTest, APlainSuccessDoesNotRepeatItsStatus) {
  OnGuiThread([] {
    RenderFixture f;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK,
                                     {FileResult(1, 0, u"/in/a.txt"_s)});
    RenderFixture::Settle();
    auto* summary = f.board->findChild<QLabel*>(u"InfoBoardSummary"_s);
    EXPECT_TRUE(f.board->CurrentReport().summary_restates_headline);
    EXPECT_FALSE(summary->isVisibleTo(f.board));
    // Still in the text outputs.
    EXPECT_TRUE(UI::RenderReportText(f.board->CurrentReport())
                    .contains(f.board->CurrentReport().summary));
  });
}

TEST(GFUiInfoBoardRenderTest, TheSealShowsTheOutcomeAndNeverApprovesAFailure) {
  OnGuiThread([] {
    RenderFixture f;
    auto* seal = dynamic_cast<UI::ReportSeal*>(
        f.board->findChild<QWidget*>(u"InfoBoardSeal"_s));
    ASSERT_NE(seal, nullptr);

    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK,
                                     {FileResult(1, 0, u"/in/a"_s)});
    RenderFixture::Settle();
    EXPECT_EQ(seal->Kind(), UI::ReportSealKind::kSUCCESS);
    EXPECT_TRUE(seal->isVisibleTo(f.board));

    f.board->SetInfoBoardFromResults(
        {}, UI::kINFO_ERROR_CRITICAL,
        {FileResult(-1, 0, u"/in/a"_s, {EngineError(u"e"_s, u"d"_s)})});
    RenderFixture::Settle();
    EXPECT_EQ(seal->Kind(), UI::ReportSealKind::kFAILURE);

    GpgOpResultInfo cancelled =
        UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT, GPG_ERR_CANCELED, {});
    GpgOperaResult r(cancelled);
    r.input_index = 0;
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_WARN, {r});
    RenderFixture::Settle();
    EXPECT_EQ(seal->Kind(), UI::ReportSealKind::kCANCELLED);
    // Its inscription names the outcome, which is also written beside it.
    EXPECT_EQ(seal->Inscription(),
              UI::HeadlineText(UI::InfoBoardHeadline::kCANCELLED).toUpper());

    // Branding and a status mark, said as much; never a signature.
    EXPECT_TRUE(seal->toolTip().contains(
        UI::HeadlineText(UI::InfoBoardHeadline::kCANCELLED)));

    // The heading: the brand, untranslated, and the report's title.
    auto* brand = f.board->findChild<QLabel*>(u"InfoBoardBrand"_s);
    ASSERT_NE(brand, nullptr);
    EXPECT_EQ(brand->text(), u"GPGFRONTEND"_s);
    EXPECT_FALSE(f.board->findChild<QLabel*>(u"InfoBoardReportName"_s)
                     ->text()
                     .isEmpty());
  });
}

TEST(GFUiInfoBoardRenderTest, ANarrowPanelDropsTheSealBeforeAnyText) {
  OnGuiThread([] {
    RenderFixture f;
    f.window.resize(340, 2400);
    f.board->SetInfoBoardFromResults({}, UI::kINFO_ERROR_OK,
                                     {FileResult(1, 0, u"/in/a"_s)});
    RenderFixture::Settle();
    auto* seal = dynamic_cast<UI::ReportSeal*>(
        f.board->findChild<QWidget*>(u"InfoBoardSeal"_s));
    ASSERT_NE(seal, nullptr);
    EXPECT_FALSE(seal->isVisibleTo(f.board));
    // The status it stood for is still written out.
    EXPECT_FALSE(
        f.board->findChild<QLabel*>(u"InfoBoardStatus"_s)->text().isEmpty());
  });
}

}  // namespace GpgFrontend::Test
