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

#include "ui/function/InfoBoardReport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "ui/function/InfoBoardCardConverter.h"

// Translation contexts are spelled out as literals at every call: lupdate
// reads nothing else. Strings that existed before this model keep the context
// they were translated in (InfoBoardWidget), so their translations stay.

namespace GpgFrontend::UI {

namespace {

// --- outcome ranks ----------------------------------------------------------

/// Display order of object groups: what needs attention first.
auto TaskRank(InfoBoardOutcome outcome) -> int {
  switch (outcome) {
    case InfoBoardOutcome::kFAILED:
      return 0;
    case InfoBoardOutcome::kNOT_SUPPORTED:
      return 1;
    case InfoBoardOutcome::kCANCELLED:
      return 2;
    case InfoBoardOutcome::kNOT_REPORTED:
      return 3;
    case InfoBoardOutcome::kNOT_SAVED:
      return 4;
    case InfoBoardOutcome::kWARNING:
      return 5;
    case InfoBoardOutcome::kSUCCEEDED:
    default:
      return 6;
  }
}

auto KindRank(InfoBoardCardKind kind) -> int { return static_cast<int>(kind); }

auto OutcomeFromStatus(InfoBoardStatus status) -> InfoBoardOutcome {
  switch (status) {
    case kINFO_ERROR_OK:
      return InfoBoardOutcome::kSUCCEEDED;
    case kINFO_ERROR_WARN:
      return InfoBoardOutcome::kWARNING;
    case kINFO_ERROR_CRITICAL:
    default:
      return InfoBoardOutcome::kFAILED;
  }
}

auto HeadlineId(InfoBoardHeadline headline) -> QString {
  switch (headline) {
    case InfoBoardHeadline::kSUCCEEDED:
      return u"succeeded"_s;
    case InfoBoardHeadline::kCOMPLETED_WITH_WARNINGS:
      return u"completed_with_warnings"_s;
    case InfoBoardHeadline::kCANCELLED:
      return u"cancelled"_s;
    case InfoBoardHeadline::kPARTIALLY_COMPLETED:
      return u"partially_completed"_s;
    case InfoBoardHeadline::kINCOMPLETE:
      return u"incomplete"_s;
    case InfoBoardHeadline::kNOT_SUPPORTED:
      return u"not_supported"_s;
    case InfoBoardHeadline::kFAILED:
      return u"failed"_s;
    case InfoBoardHeadline::kPARTIALLY_FAILED:
      return u"partially_failed"_s;
    case InfoBoardHeadline::kINFORMATION:
    default:
      return u"information"_s;
  }
}

auto OperationId(GpgOperation op) -> QString {
  switch (op) {
    case GpgOperation::kENCRYPT:
      return u"encrypt"_s;
    case GpgOperation::kDECRYPT:
      return u"decrypt"_s;
    case GpgOperation::kSIGN:
      return u"sign"_s;
    case GpgOperation::kVERIFY:
      return u"verify"_s;
    case GpgOperation::kENCRYPT_SIGN:
      return u"encrypt_sign"_s;
    case GpgOperation::kDECRYPT_VERIFY:
      return u"decrypt_verify"_s;
    default:
      return u"none"_s;
  }
}

auto FateId(OutputFate fate) -> QString {
  switch (fate) {
    case OutputFate::kNOT_APPLICABLE:
      return u"not_applicable"_s;
    case OutputFate::kNOT_PRODUCED:
      return u"not_produced"_s;
    case OutputFate::kWITHHELD:
      return u"withheld"_s;
    case OutputFate::kAPPLIED:
      return u"applied"_s;
    case OutputFate::kAPPLY_FAILED:
      return u"apply_failed"_s;
    case OutputFate::kCOMMITTED:
      return u"committed"_s;
    case OutputFate::kCOMMIT_FAILED:
      return u"commit_failed"_s;
    case OutputFate::kNOT_COMMITTED:
      return u"not_committed"_s;
    case OutputFate::kSTAGED_DISCARDED:
      return u"staged_discarded"_s;
    case OutputFate::kDISCARD_FAILED:
      return u"discard_failed"_s;
    case OutputFate::kUNKNOWN:
    default:
      return u"unknown"_s;
  }
}

auto ActionId(OutputAction action) -> QString {
  switch (action) {
    case OutputAction::kAPPLY_TO_DOCUMENT:
      return u"apply_to_document"_s;
    case OutputAction::kDISCARD_STAGED:
      return u"discard_staged"_s;
    case OutputAction::kCOMMIT_FILE:
    default:
      return u"commit_file"_s;
  }
}

// --- sources ----------------------------------------------------------------

auto GenericSource() -> InfoBoardSource {
  const auto label =
      QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport", "Result");
  return {InfoBoardSource::kGENERIC, u"result"_s, label, label};
}

auto OperationSource(const QString& operation) -> InfoBoardSource {
  if (operation.trimmed().isEmpty()) return GenericSource();
  return {InfoBoardSource::kOPERATION, u"op:"_s + operation, operation,
          operation};
}

auto ResultSource(const GpgOperaResult& result) -> InfoBoardSource {
  if (!result.source_path.isEmpty() || !result.tag.isEmpty()) {
    const auto full =
        result.source_path.isEmpty() ? result.tag : result.source_path;
    const auto label =
        result.tag.isEmpty() ? QFileInfo(full).fileName() : result.tag;
    return {InfoBoardSource::kFILE,
            u"file:%1:%2"_s.arg(result.input_index).arg(full), label, full};
  }
  return CurrentDocumentSource(result.input_index);
}

/// Distinct labels for files that share a name: the shortest trailing part of
/// their paths that tells them apart, and a count for the very same path.
void DisambiguateLabels(QList<InfoBoardObject>& objects) {
  QHash<QString, QList<qsizetype>> by_label;
  for (qsizetype i = 0; i < objects.size(); ++i) {
    if (objects[i].source.kind != InfoBoardSource::kFILE) continue;
    by_label[objects[i].source.label].append(i);
  }

  for (auto it = by_label.cbegin(); it != by_label.cend(); ++it) {
    const auto& members = it.value();
    if (members.size() < 2) continue;

    // Grow the suffix until every distinct path has a distinct one.
    QHash<qsizetype, QString> labels;
    for (int depth = 2; depth < 64; ++depth) {
      labels.clear();
      QHash<QString, QString> owner;  // label -> full path
      bool unique = true;
      for (const auto i : members) {
        const auto parts =
            QDir::fromNativeSeparators(objects[i].source.full).split(u'/');
        const auto suffix =
            parts.mid(std::max<qsizetype>(0, parts.size() - depth)).join(u'/');
        labels[i] = suffix;
        const auto prev = owner.value(suffix);
        if (!prev.isEmpty() && prev != objects[i].source.full) unique = false;
        owner[suffix] = objects[i].source.full;
      }
      if (unique) break;
    }

    // The same path more than once: number the repeats.
    QHash<QString, int> seen;
    for (const auto i : members) {
      auto label = labels.value(i, objects[i].source.label);
      const int n = ++seen[objects[i].source.full];
      if (n > 1) label += u" (%1)"_s.arg(n);
      objects[i].source.label = label;
    }
  }

  for (auto& object : objects) {
    for (auto& card : object.cards) card.source = object.source;
  }
}

// --- words ------------------------------------------------------------------

auto OutputText(OutputFate fate, const QString& path) -> QString {
  switch (fate) {
    case OutputFate::kAPPLIED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Applied to the current document");
    case OutputFate::kAPPLY_FAILED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not applied to the document");
    case OutputFate::kCOMMITTED:
      return path.isEmpty()
                 ? QCoreApplication::translate(
                       "GpgFrontend::UI::InfoBoardReport", "Written")
                 : QCoreApplication::translate(
                       "GpgFrontend::UI::InfoBoardReport", "Written to %1")
                       .arg(path);
    case OutputFate::kCOMMIT_FAILED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not written: saving failed");
    case OutputFate::kNOT_COMMITTED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Not written: saving stopped before this file");
    case OutputFate::kSTAGED_DISCARDED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Not saved: the staged output was removed");
    case OutputFate::kDISCARD_FAILED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Not saved: the staged output could not be removed");
    case OutputFate::kWITHHELD:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Withheld: a failed result's output is not used");
    case OutputFate::kNOT_PRODUCED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not produced");
    case OutputFate::kNOT_APPLICABLE:
    case OutputFate::kUNKNOWN:
    default:
      return {};
  }
}

auto ActionText(OutputAction action) -> QString {
  switch (action) {
    case OutputAction::kAPPLY_TO_DOCUMENT:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "apply to document");
    case OutputAction::kDISCARD_STAGED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "remove staged output");
    case OutputAction::kCOMMIT_FILE:
    default:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "save file");
  }
}

/// "Encrypt + Sign call" for an error known only at the combined call.
auto ScopeText(GpgOperation scope) -> QString {
  const auto name = GpgOperationName(scope);
  if (scope == GpgOperation::kENCRYPT_SIGN ||
      scope == GpgOperation::kDECRYPT_VERIFY) {
    return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                       "%1 call")
        .arg(name);
  }
  return name;
}

auto CodeText(unsigned int code) -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                     "code %1")
      .arg(code);
}

/// "Encrypt + Sign call · code 99": what an error belongs to, as the line
/// under its message.
auto ErrorNote(const QString& scope, std::optional<unsigned int> code)
    -> QString {
  QStringList parts;
  if (!scope.isEmpty()) parts << scope;
  if (code.has_value() && *code != 0) parts << CodeText(*code);
  return parts.join(u" · "_s);
}

auto ValiditySentence(GpgSigValidity validity) -> QString {
  switch (validity) {
    case GpgSigValidity::kKEY_MISSING:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signer's public key is not in this key database, so the "
          "signature could not be checked.");
    case GpgSigValidity::kINVALID:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signature does not match the data.");
    case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signature is valid, but the signer's key is not fully "
          "trusted.");
    case GpgSigValidity::kVALID_WITH_ISSUES:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signature is valid, but the signer's key has issues.");
    case GpgSigValidity::kKEY_REVOKED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "The signing key has been revoked.");
    case GpgSigValidity::kSIG_EXPIRED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "The signature has expired.");
    case GpgSigValidity::kKEY_EXPIRED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "The signing key has expired.");
    case GpgSigValidity::kKEY_NOT_AUTHORIZED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signing key is not authorized to make signatures.");
    case GpgSigValidity::kERROR:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signature could not be verified because of an error.");
    case GpgSigValidity::kFULLY_VALID:
      return {};
    case GpgSigValidity::kUNKNOWN:
    default:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The signature has an unrecognized status.");
  }
}

auto HasEngineCode(const QList<GpgOpStageError>& errors, unsigned int code)
    -> bool {
  return std::any_of(
      errors.cbegin(), errors.cend(), [code](const GpgOpStageError& e) -> bool {
        return e.origin == GpgErrorOrigin::kENGINE && e.code == code;
      });
}

auto HasEngineError(const QList<GpgOpStageError>& errors) -> bool {
  return std::any_of(
      errors.cbegin(), errors.cend(), [](const GpgOpStageError& e) -> bool {
        return e.origin == GpgErrorOrigin::kENGINE && e.code != 0;
      });
}

/// The explanation of an object's outcome. Additive: it opens with what
/// happened and adds what the domain data says; it never replaces an error.
auto ComposeReason(const InfoBoardObject& object, const GpgOpResultInfo* info,
                   const QString& engine, bool* restates_outcome = nullptr)
    -> QString {
  QStringList parts;
  // Set for the sentences that only restate the outcome the headline names.
  bool restating = false;

  const auto output_failed = [&](OutputAction action) -> bool {
    return std::any_of(
        object.output_errors.cbegin(), object.output_errors.cend(),
        [action](const OutputStageError& e) { return e.action == action; });
  };

  switch (object.task_outcome) {
    case InfoBoardOutcome::kCANCELLED:
      parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                           "The operation was cancelled.");
      restating = true;
      break;
    case InfoBoardOutcome::kNOT_SUPPORTED:
      parts << (engine.isEmpty()
                    ? QCoreApplication::translate(
                          "GpgFrontend::UI::InfoBoardReport",
                          "The active engine does not support this "
                          "operation.")
                    : QCoreApplication::translate(
                          "GpgFrontend::UI::InfoBoardReport",
                          "The active engine (%1) does not support this "
                          "operation.")
                          .arg(engine));
      break;
    case InfoBoardOutcome::kNOT_REPORTED:
      parts << QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "No result was received for this item before the report was "
          "issued.");
      break;
    case InfoBoardOutcome::kNOT_SAVED:
      parts << QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "The engine finished, but the output was not saved because another "
          "item in the batch did not succeed.");
      break;
    case InfoBoardOutcome::kFAILED:
      if (object.engine_outcome == InfoBoardOutcome::kSUCCEEDED ||
          object.engine_outcome == InfoBoardOutcome::kWARNING) {
        if (output_failed(OutputAction::kAPPLY_TO_DOCUMENT)) {
          parts << QCoreApplication::translate(
              "GpgFrontend::UI::InfoBoardReport",
              "The engine finished, but the output could not be applied to "
              "the document.");
        } else if (output_failed(OutputAction::kDISCARD_STAGED)) {
          parts << QCoreApplication::translate(
              "GpgFrontend::UI::InfoBoardReport",
              "The output was not saved, and its staged copy could not be "
              "removed.");
        } else {
          parts << QCoreApplication::translate(
              "GpgFrontend::UI::InfoBoardReport",
              "The engine finished, but the output could not be saved.");
        }
      } else if (HasEngineError(object.stage_errors)) {
        parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "The engine reported an error.");
        restating = true;
      } else if (!object.stage_errors.isEmpty()) {
        parts << QCoreApplication::translate(
            "GpgFrontend::UI::InfoBoardReport",
            "The engine returned no usable result.");
        restating = true;
      } else if (info == nullptr || info->description.isEmpty()) {
        parts << QCoreApplication::translate(
            "GpgFrontend::UI::InfoBoardReport",
            "The operation failed without a reported reason.");
      }
      break;
    case InfoBoardOutcome::kWARNING:
      if (HasEngineError(object.stage_errors)) {
        parts << QCoreApplication::translate(
            "GpgFrontend::UI::InfoBoardReport",
            "The engine reported an error, but the output was kept.");
      }
      break;
    case InfoBoardOutcome::kSUCCEEDED:
    default:
      break;
  }

  if (info != nullptr) {
    // Verify: why each signature is not simply good.
    QList<GpgSigValidity> explained;
    for (const auto& sig : info->signatures) {
      const auto validity =
          sig.rejected ? GpgSigValidity::kINVALID : sig.validity;
      if (validity == GpgSigValidity::kFULLY_VALID) continue;
      if (!explained.contains(validity)) {
        explained.append(validity);
        parts << ValiditySentence(validity);
      }
      for (const auto& w : sig.warnings) {
        if (!w.isEmpty() && !parts.contains(w)) parts << w;
      }
    }

    if (!info->invalidSigners.isEmpty()) {
      const int unusable = static_cast<int>(info->invalidSigners.size());
      parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                           "%n key(s) could not be used.",
                                           nullptr, unusable);
    }

    // On success the description is the summary, not a reason.
    if (object.task_outcome != InfoBoardOutcome::kSUCCEEDED &&
        !info->description.isEmpty() && !parts.contains(info->description)) {
      parts << info->description;
    }
  }

  if (parts.isEmpty() && object.task_outcome == InfoBoardOutcome::kWARNING) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Review the items below.");
  }

  if (restates_outcome != nullptr) {
    *restates_outcome = restating && parts.size() == 1;
  }
  return parts.join(u' ');
}

auto ComposeSuggestion(const InfoBoardObject& object,
                       const GpgOpResultInfo* info) -> QString {
  switch (object.task_outcome) {
    case InfoBoardOutcome::kCANCELLED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Run the operation again when ready.");
    case InfoBoardOutcome::kNOT_SUPPORTED:
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Use a key database with an engine that supports this operation.");
    default:
      break;
  }

  if (!object.output_errors.isEmpty() &&
      std::any_of(object.output_errors.cbegin(), object.output_errors.cend(),
                  [](const OutputStageError& e) {
                    return e.action != OutputAction::kAPPLY_TO_DOCUMENT;
                  })) {
    return QCoreApplication::translate(
        "GpgFrontend::UI::InfoBoardReport",
        "Check that the destination is writable and has free space, then "
        "run the operation again.");
  }

  if (info != nullptr) {
    const bool key_missing =
        std::any_of(info->signatures.cbegin(), info->signatures.cend(),
                    [](const GpgVerifySigInfo& s) {
                      return s.validity == GpgSigValidity::kKEY_MISSING;
                    });
    if (key_missing) {
      return QCoreApplication::translate(
          "GpgFrontend::UI::InfoBoardReport",
          "Import the signer's public key and verify again.");
    }
  }
  return {};
}

/// The Outcome card: every fact about the object, as labelled rows.
auto MakeOutcomeCard(const InfoBoardObject& object) -> InfoBoardCard {
  InfoBoardCard card;
  card.kind = InfoBoardCardKind::kOUTCOME;
  card.title = QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                           "Outcome");
  card.outcome = object.task_outcome;
  card.source = object.source;

  switch (object.task_outcome) {
    case InfoBoardOutcome::kSUCCEEDED:
      card.status = kINFO_ERROR_OK;
      break;
    case InfoBoardOutcome::kFAILED:
    case InfoBoardOutcome::kNOT_SUPPORTED:
      card.status = kINFO_ERROR_CRITICAL;
      break;
    default:
      card.status = kINFO_ERROR_WARN;
      break;
  }

  using Group = InfoBoardFieldGroup;
  const InfoBoardFieldMeta primary{{}, false, Group::kPRIMARY};
  const InfoBoardFieldMeta follow_up{{}, false, Group::kFOLLOW_UP};

  // What happened.
  card.AddField(
      QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport", "Result"),
      OutcomeText(object.task_outcome), primary);
  if (object.engine_outcome != object.task_outcome &&
      object.task_outcome != InfoBoardOutcome::kNOT_REPORTED) {
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Engine result"),
                  OutcomeText(object.engine_outcome), primary);
  }
  if (!object.reason.isEmpty()) {
    card.AddField(
        QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                    "Reason"),
        object.reason,
        {{}, false, Group::kPRIMARY, InfoBoardFieldRole::kREASON, false});
  }
  if (!object.message.isEmpty()) {
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Message"),
                  object.message, primary);
  }

  // Why, in the words of whoever said it.
  AppendObjectErrorFields(card, object);

  // What became of the output, and what to do.
  const auto output = OutputText(object.output, object.output_path);
  if (!output.isEmpty()) {
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Output"),
                  output, follow_up);
  }
  if (!object.suggestion.isEmpty()) {
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Suggestion"),
                  object.suggestion, follow_up);
  }

  if (!object.input_hash.isEmpty()) {
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Input SHA-256"),
                  object.input_hash, {{}, true, Group::kDETAIL});
  }
  return card;
}

/// Outcome card first, then by kind; incoming order kept within a kind.
void SortCards(QList<InfoBoardCard>& cards) {
  std::stable_sort(cards.begin(), cards.end(),
                   [](const InfoBoardCard& a, const InfoBoardCard& b) {
                     return KindRank(a.kind) < KindRank(b.kind);
                   });
}

void SortObjects(QList<InfoBoardObject>& objects) {
  std::stable_sort(
      objects.begin(), objects.end(),
      [](const InfoBoardObject& a, const InfoBoardObject& b) {
        const auto ra = TaskRank(a.task_outcome);
        const auto rb = TaskRank(b.task_outcome);
        if (ra != rb) return ra < rb;
        const auto ia = a.input_index < 0 ? INT_MAX : a.input_index;
        const auto ib = b.input_index < 0 ? INT_MAX : b.input_index;
        return ia < ib;
      });
}

auto ChipsTitle(std::optional<GpgOperation> op) -> QString {
  // Sentence case, like every field label; the converter's own words.
  if (op == GpgOperation::kDECRYPT || op == GpgOperation::kDECRYPT_VERIFY) {
    return QCoreApplication::translate("QObject", "Recipient");
  }
  if (op == GpgOperation::kSIGN || op == GpgOperation::kVERIFY ||
      op == GpgOperation::kENCRYPT_SIGN) {
    return QCoreApplication::translate("QObject", "Signer");
  }
  return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                     "Details");
}

auto StandardSuccess(const QString& operation) -> QString {
  return operation.trimmed().isEmpty()
             ? QCoreApplication::translate("GpgFrontend::UI::InfoBoardWidget",
                                           "The operation completed "
                                           "successfully.")
             : QCoreApplication::translate("GpgFrontend::UI::InfoBoardWidget",
                                           "%1 completed successfully.")
                   .arg(operation);
}

/// Statements about outputs, each made only when every object supports it.
auto OutputClaims(const InfoBoardReport& report,
                  const InfoBoardBatchContext& context) -> QStringList {
  QStringList claims;
  if (report.objects.isEmpty()) return claims;

  const auto all = [&](auto pred) -> bool {
    return std::all_of(report.objects.cbegin(), report.objects.cend(), pred);
  };
  const auto any = [&](auto pred) -> bool {
    return std::any_of(report.objects.cbegin(), report.objects.cend(), pred);
  };
  const bool unresolved = any([](const InfoBoardObject& o) {
    return o.task_outcome == InfoBoardOutcome::kNOT_REPORTED ||
           o.output == OutputFate::kUNKNOWN;
  });

  if (any([](const InfoBoardObject& o) {
        return o.output == OutputFate::kDISCARD_FAILED;
      })) {
    claims << QCoreApplication::translate(
        "GpgFrontend::UI::InfoBoardReport",
        "Some staged output could not be removed; its location is listed "
        "below.");
  }

  if (context.file_target && !unresolved) {
    const bool nothing_written = all([](const InfoBoardObject& o) {
      switch (o.output) {
        case OutputFate::kSTAGED_DISCARDED:
        case OutputFate::kNOT_PRODUCED:
        case OutputFate::kWITHHELD:
        case OutputFate::kNOT_COMMITTED:
        case OutputFate::kCOMMIT_FAILED:
        case OutputFate::kNOT_APPLICABLE:
          return true;
        default:
          return false;
      }
    });
    const bool all_written = all([](const InfoBoardObject& o) {
      return o.output == OutputFate::kCOMMITTED;
    });
    if (all_written) {
      claims << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                            "All output files were written.");
    } else if (nothing_written) {
      claims << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                            "No output files were written.");
    }
  }

  if (context.document_target && !unresolved &&
      all([](const InfoBoardObject& o) {
        return o.output != OutputFate::kAPPLIED;
      })) {
    claims << QCoreApplication::translate(
        "GpgFrontend::UI::InfoBoardReport",
        "The current document was not changed.");
  }

  return claims;
}

/// The Outcome card's Reason is said by the summary or the headline.
void MarkReasonSummarized(InfoBoardObject& object) {
  if (object.cards.isEmpty()) return;
  auto& card = object.cards.first();
  if (card.kind != InfoBoardCardKind::kOUTCOME) return;
  card.field_meta.resize(card.fields.size());
  for (auto& meta : card.field_meta) {
    if (meta.role == InfoBoardFieldRole::kREASON) meta.summarized = true;
  }
}

/// Severity, headline, counts and summary, from the objects.
void Aggregate(InfoBoardReport& report, const InfoBoardBatchContext& context,
               const QString& single_description) {
  InfoBoardOutcomeCounts counts;
  for (const auto& o : report.objects) counts.Add(o.task_outcome);

  report.severity = AggregateSeverity(counts);
  report.headline = AggregateHeadline(counts);
  report.count_summary =
      counts.Total() > 1 ? CountSummaryText(counts) : QString();

  QStringList summary;
  const auto claims = OutputClaims(report, context);

  // A plain success says nothing the headline does not: the standard
  // sentence is kept for the text outputs, but marked so the screen can
  // leave it out. What the outcome did to the output is never redundant.
  const bool plain_success =
      counts.succeeded == counts.Total() &&
      (counts.Total() > 1 || single_description.isEmpty());
  if (plain_success) {
    if (claims.isEmpty()) {
      report.summary =
          counts.Total() == 1
              ? StandardSuccess(report.operation_label)
              : QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                            "%1 of %2 items completed.")
                    .arg(counts.Completed())
                    .arg(counts.Total());
      report.summary_restates_headline = true;
    } else {
      report.summary = claims.join(u' ');
    }
    return;
  }

  if (counts.Total() == 1) {
    const auto& only = report.objects.first();
    if (only.task_outcome == InfoBoardOutcome::kSUCCEEDED) {
      summary << single_description;
    } else {
      // Say what the outcome meant for the document or output when that is
      // recorded; a reason that only restates the headline adds nothing
      // there. Either way the summary or the headline now carries the
      // reason, so the card need not repeat it on screen.
      if (only.reason_restates_outcome && !claims.isEmpty()) {
        summary << claims;
      } else if (!only.reason.isEmpty()) {
        summary << only.reason << claims;
      } else {
        summary << HeadlineText(report.headline) + u"."_s << claims;
      }
      MarkReasonSummarized(report.objects.first());
      report.summary = summary.join(u' ');
      return;
    }
  } else {
    summary << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                           "%1 of %2 items completed.")
                   .arg(counts.Completed())
                   .arg(counts.Total());
    InfoBoardOutcomeCounts rest = counts;
    rest.succeeded = 0;
    rest.warning = 0;
    if (rest.Total() > 0) summary << CountSummaryText(rest) + u"."_s;
  }
  summary << claims;
  report.summary = summary.join(u' ');
}

/// The engine's operation shared by all objects, if they share one.
auto CommonOperation(const QList<GpgOpResultInfo>& infos)
    -> std::optional<GpgOperation> {
  std::optional<GpgOperation> op;
  for (const auto& info : infos) {
    if (info.op == GpgOperation::kNONE) continue;
    if (!op.has_value()) {
      op = info.op;
    } else if (*op != info.op) {
      return std::nullopt;
    }
  }
  return op;
}

auto FieldValue(const InfoBoardCard& card, const QString& key) -> QString {
  for (const auto& f : card.fields) {
    if (f.first == key) return f.second;
  }
  return {};
}

}  // namespace

auto CurrentDocumentSource(int input_index) -> InfoBoardSource {
  const auto label = QCoreApplication::translate(
      "GpgFrontend::UI::InfoBoardReport", "Current document");
  return {InfoBoardSource::kDOCUMENT, u"document:%1"_s.arg(input_index), label,
          label};
}

// ---------------------------------------------------------------------------
// Semantics
// ---------------------------------------------------------------------------

auto NormalizeEngineOutcome(int status, const QList<GpgOpStageError>& errors)
    -> NormalizedEngineOutcome {
  if (status > 0) {
    if (HasEngineError(errors)) return {InfoBoardOutcome::kWARNING, true};
    return {InfoBoardOutcome::kSUCCEEDED, false};
  }
  if (status == 0) return {InfoBoardOutcome::kWARNING, false};

  bool any_code = false;
  bool only_stop_codes = true;
  for (const auto& e : errors) {
    if (e.origin != GpgErrorOrigin::kENGINE || e.code == 0) continue;
    any_code = true;
    if (e.code != GPG_ERR_CANCELED && e.code != GPG_ERR_NOT_SUPPORTED) {
      only_stop_codes = false;
    }
  }
  if (!any_code || !only_stop_codes) return {InfoBoardOutcome::kFAILED, false};
  if (HasEngineCode(errors, GPG_ERR_NOT_SUPPORTED)) {
    return {InfoBoardOutcome::kNOT_SUPPORTED, false};
  }
  return {InfoBoardOutcome::kCANCELLED, false};
}

auto ResolveTaskOutcome(InfoBoardOutcome engine, OutputFate fate)
    -> InfoBoardOutcome {
  if (engine != InfoBoardOutcome::kSUCCEEDED &&
      engine != InfoBoardOutcome::kWARNING) {
    return engine;
  }
  switch (fate) {
    case OutputFate::kAPPLY_FAILED:
    case OutputFate::kCOMMIT_FAILED:
    case OutputFate::kDISCARD_FAILED:
      return InfoBoardOutcome::kFAILED;
    case OutputFate::kSTAGED_DISCARDED:
    case OutputFate::kNOT_COMMITTED:
      return InfoBoardOutcome::kNOT_SAVED;
    default:
      return engine;
  }
}

void InfoBoardOutcomeCounts::Add(InfoBoardOutcome outcome) {
  switch (outcome) {
    case InfoBoardOutcome::kSUCCEEDED:
      ++succeeded;
      break;
    case InfoBoardOutcome::kWARNING:
      ++warning;
      break;
    case InfoBoardOutcome::kFAILED:
      ++failed;
      break;
    case InfoBoardOutcome::kCANCELLED:
      ++cancelled;
      break;
    case InfoBoardOutcome::kNOT_SUPPORTED:
      ++not_supported;
      break;
    case InfoBoardOutcome::kNOT_REPORTED:
      ++not_reported;
      break;
    case InfoBoardOutcome::kNOT_SAVED:
      ++not_saved;
      break;
  }
}

auto InfoBoardOutcomeCounts::Total() const -> int {
  return succeeded + warning + failed + cancelled + not_supported +
         not_reported + not_saved;
}

auto AggregateSeverity(const InfoBoardOutcomeCounts& c) -> InfoBoardStatus {
  if (c.Total() == 0) return kINFO_ERROR_NEUTRAL;
  if (c.failed + c.not_supported > 0) return kINFO_ERROR_CRITICAL;
  if (c.warning + c.cancelled + c.not_reported + c.not_saved > 0) {
    return kINFO_ERROR_WARN;
  }
  return kINFO_ERROR_OK;
}

auto AggregateHeadline(const InfoBoardOutcomeCounts& c) -> InfoBoardHeadline {
  if (c.Total() == 0) return InfoBoardHeadline::kINFORMATION;

  const int fu = c.failed + c.not_supported;
  const int sw = c.succeeded + c.warning;
  const int nd = c.not_reported + c.not_saved;

  if (fu == 0) {
    if (c.cancelled > 0) {
      return sw == 0 ? InfoBoardHeadline::kCANCELLED
                     : InfoBoardHeadline::kPARTIALLY_COMPLETED;
    }
    if (nd > 0) return InfoBoardHeadline::kINCOMPLETE;
    return c.warning > 0 ? InfoBoardHeadline::kCOMPLETED_WITH_WARNINGS
                         : InfoBoardHeadline::kSUCCEEDED;
  }
  if (c.failed == 0 && sw + c.cancelled + nd == 0) {
    return InfoBoardHeadline::kNOT_SUPPORTED;
  }
  return sw == 0 ? InfoBoardHeadline::kFAILED
                 : InfoBoardHeadline::kPARTIALLY_FAILED;
}

// ---------------------------------------------------------------------------
// Words
// ---------------------------------------------------------------------------

auto OutcomeText(InfoBoardOutcome outcome) -> QString {
  switch (outcome) {
    case InfoBoardOutcome::kSUCCEEDED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Succeeded");
    case InfoBoardOutcome::kWARNING:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Completed with warnings");
    case InfoBoardOutcome::kFAILED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Failed");
    case InfoBoardOutcome::kCANCELLED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Cancelled");
    case InfoBoardOutcome::kNOT_SUPPORTED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not supported");
    case InfoBoardOutcome::kNOT_REPORTED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not reported");
    case InfoBoardOutcome::kNOT_SAVED:
    default:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not saved");
  }
}

auto HeadlineText(InfoBoardHeadline headline) -> QString {
  switch (headline) {
    case InfoBoardHeadline::kSUCCEEDED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Succeeded");
    case InfoBoardHeadline::kCOMPLETED_WITH_WARNINGS:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Completed with warnings");
    case InfoBoardHeadline::kCANCELLED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Cancelled");
    case InfoBoardHeadline::kPARTIALLY_COMPLETED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Partially completed");
    case InfoBoardHeadline::kINCOMPLETE:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Incomplete");
    case InfoBoardHeadline::kNOT_SUPPORTED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Not supported");
    case InfoBoardHeadline::kFAILED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Failed");
    case InfoBoardHeadline::kPARTIALLY_FAILED:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "Partially failed");
    case InfoBoardHeadline::kINFORMATION:
    default:
      return QCoreApplication::translate("GpgFrontend::UI::InfoBoardWidget",
                                         "Information");
  }
}

auto CountSummaryText(const InfoBoardOutcomeCounts& c) -> QString {
  QStringList parts;
  if (c.Completed() > 0) {
    parts << (c.warning > 0
                  ? QCoreApplication::translate(
                        "GpgFrontend::UI::InfoBoardReport",
                        "%1 completed (%2 with warnings)")
                        .arg(c.Completed())
                        .arg(c.warning)
                  : QCoreApplication::translate(
                        "GpgFrontend::UI::InfoBoardReport", "%1 completed")
                        .arg(c.Completed()));
  }
  if (c.not_saved > 0) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "%1 not saved")
                 .arg(c.not_saved);
  }
  if (c.failed > 0) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "%1 failed")
                 .arg(c.failed);
  }
  if (c.cancelled > 0) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "%1 cancelled")
                 .arg(c.cancelled);
  }
  if (c.not_supported > 0) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "%1 not supported")
                 .arg(c.not_supported);
  }
  if (c.not_reported > 0) {
    parts << QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                         "%1 not reported")
                 .arg(c.not_reported);
  }
  return parts.join(u" · "_s);
}

auto SeverityId(InfoBoardStatus status) -> QString {
  switch (status) {
    case kINFO_ERROR_OK:
      return u"ok"_s;
    case kINFO_ERROR_WARN:
      return u"warn"_s;
    case kINFO_ERROR_CRITICAL:
      return u"critical"_s;
    default:
      return u"neutral"_s;
  }
}

auto SourceKindId(InfoBoardSource::Kind kind) -> QString {
  switch (kind) {
    case InfoBoardSource::kFILE:
      return u"file"_s;
    case InfoBoardSource::kDOCUMENT:
      return u"document"_s;
    case InfoBoardSource::kOPERATION:
      return u"operation"_s;
    case InfoBoardSource::kGENERIC:
    default:
      return u"generic"_s;
  }
}

auto OutcomeId(InfoBoardOutcome outcome) -> QString {
  switch (outcome) {
    case InfoBoardOutcome::kSUCCEEDED:
      return u"succeeded"_s;
    case InfoBoardOutcome::kWARNING:
      return u"warning"_s;
    case InfoBoardOutcome::kFAILED:
      return u"failed"_s;
    case InfoBoardOutcome::kCANCELLED:
      return u"cancelled"_s;
    case InfoBoardOutcome::kNOT_SUPPORTED:
      return u"not_supported"_s;
    case InfoBoardOutcome::kNOT_REPORTED:
      return u"not_reported"_s;
    case InfoBoardOutcome::kNOT_SAVED:
    default:
      return u"not_saved"_s;
  }
}

auto OutcomeFromId(const QString& id) -> std::optional<InfoBoardOutcome> {
  for (const auto o :
       {InfoBoardOutcome::kSUCCEEDED, InfoBoardOutcome::kWARNING,
        InfoBoardOutcome::kFAILED, InfoBoardOutcome::kCANCELLED,
        InfoBoardOutcome::kNOT_SUPPORTED, InfoBoardOutcome::kNOT_REPORTED,
        InfoBoardOutcome::kNOT_SAVED}) {
    if (OutcomeId(o) == id) return o;
  }
  return std::nullopt;
}

auto CardKindId(InfoBoardCardKind kind) -> QString {
  switch (kind) {
    case InfoBoardCardKind::kOUTCOME:
      return u"outcome"_s;
    case InfoBoardCardKind::kCODEC:
      return u"codec"_s;
    case InfoBoardCardKind::kSIGNATURE:
      return u"signature"_s;
    case InfoBoardCardKind::kNEW_SIGNATURE:
      return u"new_signature"_s;
    case InfoBoardCardKind::kINVALID_SIGNER:
      return u"invalid_signer"_s;
    case InfoBoardCardKind::kMESSAGE_METADATA:
      return u"message_metadata"_s;
    case InfoBoardCardKind::kRECIPIENT:
      return u"recipient"_s;
    case InfoBoardCardKind::kFILE_HASH:
      return u"file_hash"_s;
    case InfoBoardCardKind::kGENERIC:
    default:
      return u"generic"_s;
  }
}

auto CardKindFromId(const QString& id) -> InfoBoardCardKind {
  for (int i = 0; i <= static_cast<int>(InfoBoardCardKind::kGENERIC); ++i) {
    const auto kind = static_cast<InfoBoardCardKind>(i);
    if (CardKindId(kind) == id) return kind;
  }
  return InfoBoardCardKind::kGENERIC;
}

void AppendObjectErrorFields(InfoBoardCard& card,
                             const InfoBoardObject& object) {
  // The labels stay short and the same everywhere; which stage an error
  // belongs to, and its code, go in the line under the message.
  const bool name_scope =
      object.stage_errors.size() > 1 ||
      std::any_of(object.stage_errors.cbegin(), object.stage_errors.cend(),
                  [](const GpgOpStageError& e) {
                    return e.scope == GpgOperation::kENCRYPT_SIGN ||
                           e.scope == GpgOperation::kDECRYPT_VERIFY ||
                           e.engineDetails.size() > 1;
                  });
  const auto diagnostic = [](const QString& note) -> InfoBoardFieldMeta {
    return {note, false, InfoBoardFieldGroup::kDIAGNOSTIC};
  };

  for (const auto& e : object.stage_errors) {
    // Scope, then the engine that said it (only as recorded), then the code.
    QStringList who;
    if (name_scope) who << ScopeText(e.scope);
    if (e.origin == GpgErrorOrigin::kENGINE && !object.engine.isEmpty()) {
      who << object.engine;
    }
    const auto note = ErrorNote(who.join(u" · "_s), e.code);
    if (e.origin == GpgErrorOrigin::kENGINE) {
      card.AddField(QCoreApplication::translate(
                        "GpgFrontend::UI::InfoBoardReport", "Engine error"),
                    e.codeText, diagnostic(note));
    } else {
      card.AddField(
          QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                      "Application error"),
          e.codeText, diagnostic(note));
    }
    for (const auto& detail : e.engineDetails) {
      if (detail.second.isEmpty()) continue;
      card.AddField(
          QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                      "Engine detail"),
          detail.second,
          diagnostic(name_scope ? GpgOperationName(detail.first) : QString()));
    }
  }

  for (const auto& e : object.output_errors) {
    const auto message =
        e.origin == OutputErrorOrigin::kSYSTEM
            ? QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                          "System: %1")
                  .arg(e.message)
            : e.message;
    QStringList note{ActionText(e.action)};
    if (!e.path.isEmpty()) note << e.path;
    if (e.code.has_value()) note << CodeText(static_cast<unsigned>(*e.code));
    card.AddField(QCoreApplication::translate(
                      "GpgFrontend::UI::InfoBoardReport", "Output error"),
                  message, diagnostic(note.join(u" · "_s)));
  }
}

// ---------------------------------------------------------------------------
// Builders
// ---------------------------------------------------------------------------

namespace {

/// One object from an analysed result and what the call site knows of it.
auto ObjectFromResult(const GpgOperaResult& result,
                      const InfoBoardBatchContext& context, QStringList& notes)
    -> InfoBoardObject {
  const auto& info = result.op_info;

  InfoBoardObject object;
  object.input_index = result.input_index;
  object.source = context.expected.value(result.input_index);
  if (object.source.IsEmpty()) object.source = ResultSource(result);

  const auto engine = NormalizeEngineOutcome(info.status, info.stageErrors);
  if (engine.conflict) {
    notes << u"%1: engine reported an error but the output was kept "
             "(status %2)"_s.arg(object.source.full)
                 .arg(info.status);
  }
  object.engine_outcome = engine.outcome;
  object.output = context.fates.value(result.input_index, OutputFate::kUNKNOWN);
  object.output_errors = context.output_errors.value(result.input_index);
  object.output_path = context.output_paths.value(result.input_index);
  object.task_outcome =
      ResolveTaskOutcome(object.engine_outcome, object.output);
  object.stage_errors = info.stageErrors;
  object.input_hash = info.inputHash;
  object.raw_report = info.report;
  object.engine = info.engine;
  object.reason = ComposeReason(object, &info, info.engine,
                                &object.reason_restates_outcome);
  object.suggestion = ComposeSuggestion(object, &info);

  object.cards.append(MakeOutcomeCard(object));
  object.cards.append(context.extra_cards.value(result.input_index));
  object.cards.append(convert_op_info_to_cards(info));
  for (auto& card : object.cards) card.source = object.source;
  SortCards(object.cards);
  return object;
}

/// An expected input that never reported a result.
auto MissingObject(int index, const InfoBoardSource& source,
                   const InfoBoardBatchContext& context) -> InfoBoardObject {
  InfoBoardObject object;
  object.input_index = index;
  object.source = source;
  object.engine_outcome = InfoBoardOutcome::kNOT_REPORTED;
  object.task_outcome = InfoBoardOutcome::kNOT_REPORTED;
  // Whatever cleanup reported, a result that has not arrived may still be
  // writing: nothing is claimed about its output.
  object.output = OutputFate::kUNKNOWN;
  object.output_errors = context.output_errors.value(index);
  object.reason = ComposeReason(object, nullptr, {});
  object.cards.append(MakeOutcomeCard(object));
  return object;
}

void Finish(InfoBoardReport& report, const InfoBoardBatchContext& context,
            const QString& single_description) {
  DisambiguateLabels(report.objects);
  SortObjects(report.objects);
  Aggregate(report, context, single_description);
}

}  // namespace

auto BuildReportFromText(const QString& text, InfoBoardStatus status)
    -> InfoBoardReport {
  InfoBoardReport report;
  const auto body = text.trimmed();

  if (status == kINFO_ERROR_NEUTRAL) {
    report.severity = kINFO_ERROR_NEUTRAL;
    report.headline = InfoBoardHeadline::kINFORMATION;
    report.summary = body;
    return report;
  }

  InfoBoardObject object;
  object.source = GenericSource();
  object.engine_outcome = OutcomeFromStatus(status);
  object.task_outcome = object.engine_outcome;
  object.message = body;
  if (object.task_outcome == InfoBoardOutcome::kFAILED) {
    object.reason = QCoreApplication::translate(
        "GpgFrontend::UI::InfoBoardReport",
        "The operation failed. The message below is all that was reported.");
  } else if (object.task_outcome == InfoBoardOutcome::kWARNING) {
    object.reason = QCoreApplication::translate(
        "GpgFrontend::UI::InfoBoardReport",
        "The operation completed with warnings. The message below is all "
        "that was reported.");
  }
  object.cards.append(MakeOutcomeCard(object));
  report.objects.append(object);

  Finish(report, {}, body);
  return report;
}

auto BuildReportFromCards(const QString& text, InfoBoardStatus status,
                          const QList<InfoBoardCard>& cards,
                          const QString& operation, const QString& description,
                          const QString& details_title,
                          const QStringList& details_items) -> InfoBoardReport {
  InfoBoardReport report;
  report.operation_label = operation;
  report.chips = details_items;
  report.chips_title =
      details_title.isEmpty() ? ChipsTitle(std::nullopt) : details_title;

  const auto fallback = OperationSource(operation);

  // Group by identity, keeping the order groups first appear in.
  QList<InfoBoardObject> objects;
  QHash<QString, qsizetype> index_of;
  for (auto card : cards) {
    if (card.source.IsEmpty()) card.source = fallback;
    if (card.source.label.isEmpty()) card.source.label = card.source.id;
    if (card.source.full.isEmpty()) card.source.full = card.source.label;
    auto it = index_of.find(card.source.id);
    if (it == index_of.end()) {
      InfoBoardObject object;
      object.source = card.source;
      objects.append(object);
      it = index_of.insert(card.source.id, objects.size() - 1);
    }
    objects[*it].cards.append(card);
  }
  if (objects.isEmpty() && status != kINFO_ERROR_NEUTRAL) {
    InfoBoardObject object;
    object.source = fallback;
    objects.append(object);
  }

  const auto reason_text = description.isEmpty() ? text.trimmed() : description;

  if (status == kINFO_ERROR_NEUTRAL) {
    // Information: shown as given, nothing to explain.
    for (auto& object : objects) SortCards(object.cards);
    report.objects = objects;
    DisambiguateLabels(report.objects);
    report.severity = kINFO_ERROR_NEUTRAL;
    report.headline = InfoBoardHeadline::kINFORMATION;
    report.summary = reason_text;
    return report;
  }

  for (auto& object : objects) {
    // The first valid structured Outcome card decides; later or malformed
    // ones are kept, as ordinary cards.
    std::optional<qsizetype> outcome_at;
    for (qsizetype i = 0; i < object.cards.size(); ++i) {
      auto& card = object.cards[i];
      if (card.kind != InfoBoardCardKind::kOUTCOME) continue;
      if (card.outcome.has_value() && !outcome_at.has_value()) {
        outcome_at = i;
        continue;
      }
      report.notes << (card.outcome.has_value()
                           ? u"%1: a second Outcome card is shown as an "
                             "ordinary card"_s.arg(object.source.full)
                           : u"%1: an Outcome card without an outcome is shown "
                             "as an ordinary card"_s.arg(object.source.full));
      card.kind = InfoBoardCardKind::kGENERIC;
      card.outcome.reset();
    }

    if (outcome_at.has_value()) {
      auto& card = object.cards[*outcome_at];
      object.engine_outcome = *card.outcome;
      object.task_outcome = *card.outcome;
      object.reason =
          FieldValue(card, QCoreApplication::translate(
                               "GpgFrontend::UI::InfoBoardReport", "Reason"));
      if (FieldValue(card, QCoreApplication::translate(
                               "GpgFrontend::UI::InfoBoardReport", "Result"))
              .isEmpty()) {
        card.PrependField(QCoreApplication::translate(
                              "GpgFrontend::UI::InfoBoardReport", "Result"),
                          OutcomeText(*card.outcome),
                          {{}, false, InfoBoardFieldGroup::kPRIMARY});
      }
    } else {
      object.engine_outcome = OutcomeFromStatus(status);
      object.task_outcome = object.engine_outcome;
      object.reason = object.task_outcome == InfoBoardOutcome::kSUCCEEDED
                          ? QString()
                          : reason_text;
      if (object.reason.isEmpty()) {
        object.reason = ComposeReason(object, nullptr, {});
      }
      object.cards.prepend(MakeOutcomeCard(object));
    }
    SortCards(object.cards);
  }

  // The caller's text is the engine-style report of what it posted: the
  // single object's own, or the whole posting's when there are several.
  if (objects.size() == 1) {
    objects.first().raw_report = text.trimmed();
  } else {
    report.raw_text = text.trimmed();
  }

  report.objects = objects;
  Finish(report, {}, description);

  if (report.severity != status) {
    report.notes << u"posting status %1 differs from the structured outcomes "
                    "(%2); the structured outcomes win"_s
                        .arg(SeverityId(status))
                        .arg(SeverityId(report.severity));
  }
  return report;
}

auto BuildReportFromResults(const QList<GpgOperaResult>& results,
                            const InfoBoardBatchContext& context)
    -> InfoBoardReport {
  InfoBoardReport report;

  QList<GpgOpResultInfo> infos;
  QSet<int> reported;
  QStringList chips;
  for (const auto& result : results) {
    infos.append(result.op_info);
    if (result.input_index >= 0) reported.insert(result.input_index);
    report.objects.append(ObjectFromResult(result, context, report.notes));
    for (const auto& d : result.op_info.details) {
      if (!d.isEmpty() && !chips.contains(d)) chips.append(d);
    }
  }
  for (auto it = context.expected.cbegin(); it != context.expected.cend();
       ++it) {
    if (!reported.contains(it.key())) {
      report.objects.append(MissingObject(it.key(), it.value(), context));
    }
  }

  report.op = CommonOperation(infos);
  if (report.op.has_value()) {
    report.operation_label = GpgOperationName(*report.op);
  } else {
    for (const auto& info : infos) {
      if (!info.operation.isEmpty()) {
        report.operation_label = info.operation;
        break;
      }
    }
  }
  for (const auto& info : infos) {
    if (!info.engine.isEmpty()) {
      report.engine = info.engine;
      break;
    }
  }
  report.chips = chips;
  report.chips_title = ChipsTitle(report.op);

  const auto description =
      results.size() == 1 ? results.first().op_info.description : QString();
  Finish(report, context, description);
  return report;
}

auto BuildOpInfoCards(const GpgOpResultInfo& info,
                      const InfoBoardSource& source) -> QList<InfoBoardCard> {
  GpgOperaResult result(info);
  InfoBoardBatchContext context;
  QStringList notes;
  auto object = ObjectFromResult(result, context, notes);
  for (auto& card : object.cards) card.source = source;
  return object.cards;
}

// ---------------------------------------------------------------------------
// Hash
// ---------------------------------------------------------------------------

auto CanonicalReportBytes(const InfoBoardReport& report) -> QByteArray {
  QJsonArray objects;
  for (const auto& o : report.objects) {
    QJsonArray stage_errors;
    for (const auto& e : o.stage_errors) {
      QJsonArray details;
      for (const auto& d : e.engineDetails) {
        details.append(QJsonArray{OperationId(d.first), d.second});
      }
      stage_errors.append(QJsonObject{
          {u"scope"_s, OperationId(e.scope)},
          {u"origin"_s, e.origin == GpgErrorOrigin::kENGINE ? u"engine"_s
                                                            : u"application"_s},
          {u"code"_s, static_cast<qint64>(e.code)},
          {u"codeText"_s, e.codeText},
          {u"engineDetails"_s, details}});
    }
    QJsonArray output_errors;
    for (const auto& e : o.output_errors) {
      output_errors.append(QJsonObject{
          {u"action"_s, ActionId(e.action)},
          {u"path"_s, e.path},
          {u"code"_s, e.code.has_value() ? QJsonValue(*e.code) : QJsonValue()},
          {u"message"_s, e.message},
          {u"origin"_s, e.origin == OutputErrorOrigin::kSYSTEM
                            ? u"system"_s
                            : u"application"_s}});
    }
    QJsonArray cards;
    for (const auto& c : o.cards) {
      QJsonArray fields;
      for (qsizetype i = 0; i < c.fields.size(); ++i) {
        const auto meta = c.MetaAt(i);
        fields.append(QJsonArray{c.fields[i].first, c.fields[i].second,
                                 meta.note, meta.technical,
                                 static_cast<int>(meta.group)});
      }
      cards.append(
          QJsonObject{{u"kind"_s, CardKindId(c.kind)},
                      {u"outcome"_s, c.outcome.has_value()
                                         ? QJsonValue(OutcomeId(*c.outcome))
                                         : QJsonValue()},
                      {u"title"_s, c.title},
                      {u"status"_s, SeverityId(c.status)},
                      {u"source"_s, c.source.id},
                      {u"fields"_s, fields}});
    }
    objects.append(QJsonObject{
        {u"source"_s, QJsonObject{{u"kind"_s, SourceKindId(o.source.kind)},
                                  {u"id"_s, o.source.id},
                                  {u"label"_s, o.source.label},
                                  {u"full"_s, o.source.full}}},
        {u"input_index"_s, o.input_index},
        {u"engine_outcome"_s, OutcomeId(o.engine_outcome)},
        {u"task_outcome"_s, OutcomeId(o.task_outcome)},
        {u"output"_s, FateId(o.output)},
        {u"output_path"_s, o.output_path},
        {u"stage_errors"_s, stage_errors},
        {u"output_errors"_s, output_errors},
        {u"reason"_s, o.reason},
        {u"suggestion"_s, o.suggestion},
        {u"message"_s, o.message},
        {u"input_hash"_s, o.input_hash},
        {u"cards"_s, cards}});
  }

  const QJsonObject root{
      {u"v"_s, u"gf-report-1"_s},
      {u"issued_at"_s, report.issued_at.toUTC().toString(Qt::ISODate)},
      {u"severity"_s, SeverityId(report.severity)},
      {u"headline"_s, HeadlineId(report.headline)},
      {u"op"_s, report.op.has_value() ? OperationId(*report.op) : u"none"_s},
      {u"operation_label"_s, report.operation_label},
      {u"engine"_s, report.engine},
      {u"count_summary"_s, report.count_summary},
      {u"summary"_s, report.summary},
      {u"chips_title"_s, report.chips_title},
      {u"chips"_s, QJsonArray::fromStringList(report.chips)},
      {u"objects"_s, objects}};
  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void FinalizeReport(InfoBoardReport& report, const QDateTime& issued_at) {
  // Whole seconds: the canonical form must not depend on sub-second noise.
  report.issued_at = issued_at.toUTC();
  report.issued_at.setTime(QTime(report.issued_at.time().hour(),
                                 report.issued_at.time().minute(),
                                 report.issued_at.time().second()));

  const auto digest = QCryptographicHash::hash(CanonicalReportBytes(report),
                                               QCryptographicHash::Sha256);
  report.report_hash = QString::fromLatin1(digest.toHex()).toUpper();

  const auto abbr = [&]() -> QString {
    switch (report.severity) {
      case kINFO_ERROR_OK:
        return u"OK"_s;
      case kINFO_ERROR_WARN:
        return u"WN"_s;
      case kINFO_ERROR_CRITICAL:
        return u"FL"_s;
      default:
        return u"GP"_s;
    }
  }();
  report.ref_id = u"REF-%1-%2-%3"_s.arg(abbr)
                      .arg(report.issued_at.toString(u"yyyyMMdd"_s))
                      .arg(report.report_hash.left(8));
}

// ---------------------------------------------------------------------------
// Text outputs
// ---------------------------------------------------------------------------

namespace {

/// Rows as "Key:  value" with the short values aligned in one column. The
/// Status Panel is narrow: a value that would not fit beside its key (a
/// digest, a long message, several lines) goes under it instead, a step in,
/// so a wrap starts near the margin rather than mid-column. Values are never
/// changed, only placed.
auto AlignedRows(const QList<std::tuple<QString, QString, QString>>& rows,
                 int indent) -> QStringList {
  constexpr int kMaxKey = 18;
  constexpr int kInlineWidth = 56;  // a narrow panel's comfortable line

  const auto fits = [&](const QString& key, const QString& value) {
    return !value.contains(u'\n') &&
           indent + std::min<int>(key.size() + 1, kMaxKey) + 2 + value.size() <=
               kInlineWidth;
  };
  int key_width = 0;
  for (const auto& [key, value, note] : rows) {
    if (fits(key, value)) {
      key_width = std::max(key_width, static_cast<int>(key.size()) + 1);
    }
  }
  key_width = std::min(key_width, kMaxKey);

  const QString pad(indent, u' ');
  const QString under(indent + 2, u' ');
  QStringList lines;
  for (const auto& [key, value, note] : rows) {
    // Bracketed, so a note never reads as another line of the value.
    const auto note_line = note.isEmpty() ? QString() : u"[%1]"_s.arg(note);
    if (fits(key, value)) {
      const auto label = (key + u':').leftJustified(key_width);
      const QString hang(indent + key_width + 2, u' ');
      lines << pad + label + u"  "_s + value;
      if (!note_line.isEmpty()) {
        // A long note goes under the key too.
        lines << (hang.size() + note_line.size() <= kInlineWidth ? hang
                                                                 : under) +
                     note_line;
      }
      continue;
    }
    lines << pad + key + u':';
    for (const auto& part : value.split(u'\n')) lines << under + part;
    if (!note_line.isEmpty()) lines << under + note_line;
  }
  return lines;
}

auto Underlined(const QString& title, QChar rule) -> QStringList {
  // A rule as long as the title, but never wider than a narrow panel.
  constexpr int kMaxRule = 40;
  return {title, QString(std::min<int>(title.size(), kMaxRule), rule)};
}

/// One card as text: its title, then its fields.
auto CardText(const InfoBoardCard& card, int indent) -> QStringList {
  QStringList lines{QString(indent, u' ') + card.title};
  QList<std::tuple<QString, QString, QString>> rows;
  for (qsizetype i = 0; i < card.fields.size(); ++i) {
    if (card.fields[i].second.isEmpty()) continue;
    rows.append(
        {card.fields[i].first, card.fields[i].second, card.MetaAt(i).note});
  }
  lines << AlignedRows(rows, indent + 2);
  return lines;
}

/**
 * @brief An engine's own report, tidied for reading.
 *
 * Only the Markdown it is written in is touched (heading markers, list
 * dashes, separator lines, doubled spaces), never its words: headings lose
 * their markers and step in by level, list items keep a dash under them, and
 * rules of dashes go. What the engine said stays as it said it.
 */
auto TidyEngineReport(const QString& raw, int indent) -> QStringList {
  QStringList lines;
  bool pending_blank = false;
  int section_depth = 0;  // the depth of the heading the lines are under
  for (const auto& source : raw.split(u'\n')) {
    const auto trimmed = source.trimmed();
    if (trimmed.isEmpty()) {
      pending_blank = !lines.isEmpty();
      continue;
    }
    const bool rule =
        trimmed.size() >= 3 &&
        std::all_of(trimmed.cbegin(), trimmed.cend(), [](QChar ch) {
          return ch == u'-' || ch == u'=' || ch == u'_';
        });
    if (rule) {
      pending_blank = !lines.isEmpty();
      continue;
    }

    // Nesting from the line's own leading spaces.
    int lead = 0;
    while (lead < source.size() && source[lead] == u' ') ++lead;
    const int nest = lead / 2;

    QString text = trimmed.simplified();
    int depth = 0;
    bool heading = false;
    if (text.startsWith(u"### "_s)) {
      text = text.mid(4), depth = 2, heading = true;
    } else if (text.startsWith(u"## "_s)) {
      text = text.mid(3), depth = 1, heading = true;
    } else if (text.startsWith(u"# "_s)) {
      text = text.mid(2), depth = 0, heading = true;
    }
    if (heading) {
      if (text.endsWith(u':')) text.chop(1);
      // " - Success" after a heading reads better as a colon.
      text.replace(u") - "_s, u"): "_s);
      if (!lines.isEmpty()) lines << QString();
      lines << QString(indent + (depth * 2), u' ') + text;
      section_depth = depth;
      pending_blank = false;
      continue;
    }

    if (pending_blank) lines << QString();
    pending_blank = false;
    if (text.startsWith(u"- "_s) || text.startsWith(u"* "_s)) {
      text = u"- "_s + text.mid(2);
    }
    lines << QString(indent + ((section_depth + 1 + nest) * 2), u' ') + text;
  }
  return lines;
}

/// The report's head: what this is, what ran, how it ended, which report.
auto HeaderText(const InfoBoardReport& report) -> QStringList {
  const auto title =
      u"GPGFRONTEND \u00B7 "_s +
      QCoreApplication::translate("GpgFrontend::UI::InfoBoardWidget",
                                  "SECURITY OPERATION REPORT");
  QStringList lines = Underlined(title, u'=');

  QList<std::tuple<QString, QString, QString>> rows;
  if (!report.operation_label.isEmpty()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Operation"),
                 report.operation_label,
                 {}});
  }
  rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                           "Outcome"),
               HeadlineText(report.headline),
               {}});
  if (!report.count_summary.isEmpty()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Items"),
                 report.count_summary,
                 {}});
  }
  if (!report.engine.isEmpty()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Engine"),
                 report.engine,
                 {}});
  }
  if (!report.chips.isEmpty()) {
    rows.append({report.chips_title, report.chips.join(u"\n"_s), {}});
  }
  if (!report.ref_id.isEmpty()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Reference"),
                 report.ref_id,
                 {}});
  }
  if (report.issued_at.isValid()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Issued"),
                 QLocale().toString(report.issued_at.toLocalTime(),
                                    QLocale::ShortFormat),
                 {}});
  }
  if (!report.report_hash.isEmpty()) {
    rows.append({QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport",
                                             "Report hash"),
                 report.report_hash,
                 {}});
  }
  lines << AlignedRows(rows, 0);

  if (!report.summary.isEmpty()) lines << QString() << report.summary;
  if (!report.raw_text.isEmpty()) lines << QString() << report.raw_text;
  return lines;
}

/// The whole report as text; with @p engine_reports, each item's engine
/// report under its cards.
auto ReportText(const InfoBoardReport& report, bool engine_reports) -> QString {
  QStringList lines = HeaderText(report);
  const auto count = report.objects.size();
  for (qsizetype i = 0; i < count; ++i) {
    const auto& object = report.objects[i];
    lines << QString();
    const auto heading =
        count > 1 ? u"%1/%2  %3"_s.arg(i + 1).arg(count).arg(object.source.full)
                  : object.source.full;
    lines << Underlined(heading, u'-');
    for (qsizetype c = 0; c < object.cards.size(); ++c) {
      if (c > 0) lines << QString();
      lines << CardText(object.cards[c], 2);
    }
    if (engine_reports && !object.raw_report.trimmed().isEmpty()) {
      lines << QString()
            << u"  "_s +
                   QCoreApplication::translate(
                       "GpgFrontend::UI::InfoBoardReport", "Engine report");
      lines << TidyEngineReport(object.raw_report, 4);
    }
  }
  return lines.join(u'\n');
}

}  // namespace

auto RenderReportText(const InfoBoardReport& report) -> QString {
  return ReportText(report, false);
}

auto RenderRawDetails(const InfoBoardReport& report) -> QString {
  return ReportText(report, true);
}

}  // namespace GpgFrontend::UI
