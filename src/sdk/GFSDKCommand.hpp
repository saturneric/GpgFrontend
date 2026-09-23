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

#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QList>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "GFSDKTypes.h"

/**
 * @file GFSDKCommand.hpp
 * @brief Commands as typed C++ objects.
 *
 * A command is a plain type. Everything the Host needs to know about it --
 * the registry entry, the argument and result schemas, the marshalling to and
 * from CBOR, the documentation -- is DERIVED from that type, so it is written
 * once:
 *
 * @code
 * struct PublishKey {
 *   static constexpr gf::cmd::Meta kMeta{
 *       "com.example.module.publish_key", GC_TR("Publish Key"),
 *       GC_TR("Upload the public key to the key server"),
 *       GC_TR("Key Server Operations"), GF_HOST_CAP_GPG, gf::cmd::kLongRunning};
 *   struct Args {
 *     gf::cmd::KeyRef key;
 *     static constexpr auto Fields() { return std::make_tuple(gf::cmd::F("key", &Args::key)); }
 *   };
 *   using Result = gf::cmd::Unit;
 * };
 *
 * auto Publish(const gf::cmd::CommandContext&, const PublishKey::Args&)
 *     -> gf::cmd::Outcome<gf::cmd::Unit>;
 *
 * // the one registration line, in the module's hook table:
 * gf::cmd::Bind<PublishKey, &Publish>()
 * @endcode
 *
 * This header is shared by the Host and by modules, so it depends on QtCore
 * and the C types only. It never calls the SDK: moving a call across the
 * boundary is the job of GFSDKCommandBus.hpp on the module side and of the
 * Host's registry on the other.
 *
 * Titles, descriptions and categories are untranslated source strings marked
 * with the caller's own translation macro. The Host translates them when it
 * displays them -- never the module, whose translators may not be installed
 * yet when the command is registered.
 */

namespace gf::cmd {

// ------------------------------------------------------------------ metadata

/// Execution metadata, in Meta::flags.
enum Flag : uint32_t {
  /// Runs on the GUI thread. Everything that touches a widget, the editor or
  /// a dialog. Anything else runs on a worker thread.
  kNeedsGuiThread = 1U << 0,
  /// Has an on/off state that a menu entry shows as a check mark.
  kCheckable = 1U << 1,
  /// May take a while; a UI shows progress rather than freezing on it.
  kLongRunning = 1U << 2,
  /// The Host's own: invocable from the Host UI, never by a module.
  kHostOnly = 1U << 3,
};

/// Everything about a command that is not its argument types.
struct Meta {
  const char* id;           ///< stable, "<module_id>.<name>" or "org.gpgfrontend.*"
  const char* title;        ///< untranslated source string
  const char* description;  ///< untranslated source string, may be ""
  const char* category;     ///< untranslated source string, may be ""
  uint32_t required_caps;   ///< GF_HOST_CAP_* bits the CALLER must hold
  uint32_t flags;           ///< Flag bits
};

// ------------------------------------------------------------------ fields

/// One reflected member: its wire name and where it lives.
template <typename C, typename T>
struct Field {
  const char* name;
  T C::* member;
};

/// Name a member for reflection: `F("name", &Args::name)`.
template <typename C, typename T>
constexpr auto F(const char* name, T C::* member) -> Field<C, T> {
  return {name, member};
}

/// Whether @p T declares `static constexpr auto Fields()`.
template <typename T, typename = void>
struct IsReflected : std::false_type {};
template <typename T>
struct IsReflected<T, std::void_t<decltype(T::Fields())>> : std::true_type {};

// ------------------------------------------------------------------ blobs

/**
 * @brief Where a Blob's bytes actually are.
 *
 * Abstract so that one Blob type serves both sides: a module's bytes are a
 * buffer handle it owns, the Host's are a GFBuffer. Either way the bytes
 * live in memory that is wiped when released.
 */
class BlobStorage {
 public:
  virtual ~BlobStorage() = default;
  [[nodiscard]] virtual auto Data() const -> const char* = 0;
  [[nodiscard]] virtual auto Size() const -> size_t = 0;
};

/**
 * @brief Secret or bulky bytes, carried BESIDE the CBOR, never inside it.
 *
 * In the CBOR a Blob is `{"$blob": n}`, an index into the call's buffer
 * list. Its bytes therefore never enter a QCborValue, which cannot be
 * wiped. There is deliberately no conversion to QByteArray or QString: code
 * that needs one has to copy the bytes out on purpose, where it can be seen.
 */
class Blob {
 public:
  Blob() = default;
  explicit Blob(std::shared_ptr<BlobStorage> storage)
      : storage_(std::move(storage)) {}

  [[nodiscard]] auto IsNull() const -> bool { return storage_ == nullptr; }
  [[nodiscard]] auto Data() const -> const char* {
    return storage_ ? storage_->Data() : nullptr;
  }
  [[nodiscard]] auto Size() const -> size_t {
    return storage_ ? storage_->Size() : 0;
  }
  [[nodiscard]] auto Storage() const -> const std::shared_ptr<BlobStorage>& {
    return storage_;
  }

 private:
  std::shared_ptr<BlobStorage> storage_;
};

// ------------------------------------------------------------------ targets

/// No arguments, or no result.
struct Unit {
  static constexpr auto Fields() { return std::make_tuple(); }
};

/// A document the Host has open. id 0 means "the active document".
struct DocumentRef {
  qint64 id = 0;
  qint64 generation = 0;  ///< which incarnation of that id; 0 = any
  QString type;           ///< "text", "email", ...; informational

  static constexpr auto Fields() {
    return std::make_tuple(F("id", &DocumentRef::id),
                           F("generation", &DocumentRef::generation),
                           F("type", &DocumentRef::type));
  }
};

/// A key in one of the Host's key databases.
struct KeyRef {
  qint64 channel = 0;
  QString key_id;
  QString fingerprint;
  bool has_secret = false;

  static constexpr auto Fields() {
    return std::make_tuple(F("channel", &KeyRef::channel),
                           F("key_id", &KeyRef::key_id),
                           F("fingerprint", &KeyRef::fingerprint),
                           F("has_secret", &KeyRef::has_secret));
  }
};

/// A module view mounted by the module's UI script, by its full id.
struct ViewRef {
  QString id;

  static constexpr auto Fields() { return std::make_tuple(F("id", &ViewRef::id)); }
};

// ------------------------------------------------------------------ codec

/// The side channel an encode appends Blobs to.
struct EncodeState {
  std::vector<Blob> blobs;
};

/// The side channel a decode takes Blobs from, and why it failed.
struct DecodeState {
  const std::vector<Blob>* blobs = nullptr;
  QString error;

  auto Fail(const QString& why) -> bool {
    if (error.isEmpty()) error = why;
    return false;
  }
};

template <typename T, typename = void>
struct Codec;

namespace detail {

inline auto Schema(const char* type) -> QCborMap {
  QCborMap m;
  m.insert(QStringLiteral("type"), QString::fromLatin1(type));
  return m;
}

}  // namespace detail

template <>
struct Codec<bool> {
  static auto Schema() -> QCborMap { return detail::Schema("bool"); }
  static auto Encode(bool v, EncodeState&) -> QCborValue { return v; }
  static auto Decode(const QCborValue& v, bool& out, DecodeState& st) -> bool {
    if (!v.isBool()) return st.Fail(QStringLiteral("expected a boolean"));
    out = v.toBool();
    return true;
  }
};

template <typename T>
struct Codec<T, std::enable_if_t<std::is_integral_v<T> &&
                                 !std::is_same_v<T, bool>>> {
  static auto Schema() -> QCborMap { return detail::Schema("int"); }
  static auto Encode(T v, EncodeState&) -> QCborValue {
    return static_cast<qint64>(v);
  }
  static auto Decode(const QCborValue& v, T& out, DecodeState& st) -> bool {
    if (!v.isInteger()) return st.Fail(QStringLiteral("expected an integer"));
    const auto i = v.toInteger();
    if constexpr (std::is_unsigned_v<T>) {
      if (i < 0) return st.Fail(QStringLiteral("expected a non-negative integer"));
    }
    out = static_cast<T>(i);
    return true;
  }
};

template <typename T>
struct Codec<T, std::enable_if_t<std::is_enum_v<T>>> {
  using U = std::underlying_type_t<T>;
  static auto Schema() -> QCborMap { return detail::Schema("int"); }
  static auto Encode(T v, EncodeState& st) -> QCborValue {
    return Codec<U>::Encode(static_cast<U>(v), st);
  }
  static auto Decode(const QCborValue& v, T& out, DecodeState& st) -> bool {
    U u{};
    if (!Codec<U>::Decode(v, u, st)) return false;
    out = static_cast<T>(u);
    return true;
  }
};

template <>
struct Codec<double> {
  static auto Schema() -> QCborMap { return detail::Schema("double"); }
  static auto Encode(double v, EncodeState&) -> QCborValue { return v; }
  static auto Decode(const QCborValue& v, double& out, DecodeState& st)
      -> bool {
    if (!v.isDouble() && !v.isInteger()) {
      return st.Fail(QStringLiteral("expected a number"));
    }
    out = v.toDouble();
    return true;
  }
};

template <>
struct Codec<QString> {
  static auto Schema() -> QCborMap { return detail::Schema("string"); }
  static auto Encode(const QString& v, EncodeState&) -> QCborValue { return v; }
  static auto Decode(const QCborValue& v, QString& out, DecodeState& st)
      -> bool {
    if (!v.isString()) return st.Fail(QStringLiteral("expected a string"));
    out = v.toString();
    return true;
  }
};

template <>
struct Codec<QStringList> {
  static auto Schema() -> QCborMap { return detail::Schema("string_list"); }
  static auto Encode(const QStringList& v, EncodeState&) -> QCborValue {
    QCborArray a;
    for (const auto& s : v) a.append(s);
    return a;
  }
  static auto Decode(const QCborValue& v, QStringList& out, DecodeState& st)
      -> bool {
    if (!v.isArray()) return st.Fail(QStringLiteral("expected a string list"));
    out.clear();
    for (const auto& e : v.toArray()) {
      if (!e.isString()) {
        return st.Fail(QStringLiteral("expected a string list"));
      }
      out.append(e.toString());
    }
    return true;
  }
};

/// Non-secret octets only. Anything secret is a Blob.
template <>
struct Codec<QByteArray> {
  static auto Schema() -> QCborMap { return detail::Schema("bytes"); }
  static auto Encode(const QByteArray& v, EncodeState&) -> QCborValue {
    return v;
  }
  static auto Decode(const QCborValue& v, QByteArray& out, DecodeState& st)
      -> bool {
    if (!v.isByteArray()) return st.Fail(QStringLiteral("expected bytes"));
    out = v.toByteArray();
    return true;
  }
};

template <>
struct Codec<Blob> {
  static auto Schema() -> QCborMap { return detail::Schema("blob"); }
  static auto Encode(const Blob& v, EncodeState& st) -> QCborValue {
    if (v.IsNull()) return QCborValue(QCborValue::Null);
    QCborMap m;
    m.insert(QStringLiteral("$blob"), static_cast<qint64>(st.blobs.size()));
    st.blobs.push_back(v);
    return m;
  }
  static auto Decode(const QCborValue& v, Blob& out, DecodeState& st) -> bool {
    if (v.isNull()) {
      out = Blob();
      return true;
    }
    const auto index = v.toMap().value(QStringLiteral("$blob"));
    if (!v.isMap() || !index.isInteger()) {
      return st.Fail(QStringLiteral("expected a blob reference"));
    }
    const auto i = index.toInteger();
    if (st.blobs == nullptr || i < 0 ||
        static_cast<size_t>(i) >= st.blobs->size()) {
      return st.Fail(QStringLiteral("blob reference out of range"));
    }
    out = (*st.blobs)[static_cast<size_t>(i)];
    return true;
  }
};

template <typename T>
struct Codec<std::optional<T>> {
  static auto Schema() -> QCborMap {
    auto m = Codec<T>::Schema();
    m.insert(QStringLiteral("optional"), true);
    return m;
  }
  static auto Encode(const std::optional<T>& v, EncodeState& st)
      -> QCborValue {
    if (!v.has_value()) return QCborValue(QCborValue::Null);
    return Codec<T>::Encode(*v, st);
  }
  static auto Decode(const QCborValue& v, std::optional<T>& out,
                     DecodeState& st) -> bool {
    if (v.isNull() || v.isUndefined()) {
      out.reset();
      return true;
    }
    T inner{};
    if (!Codec<T>::Decode(v, inner, st)) return false;
    out = std::move(inner);
    return true;
  }
};

template <typename T>
struct Codec<QList<T>, std::enable_if_t<!std::is_same_v<QList<T>, QStringList>>> {
  static auto Schema() -> QCborMap {
    auto m = detail::Schema("list");
    m.insert(QStringLiteral("item"), Codec<T>::Schema());
    return m;
  }
  static auto Encode(const QList<T>& v, EncodeState& st) -> QCborValue {
    QCborArray a;
    for (const auto& e : v) a.append(Codec<T>::Encode(e, st));
    return a;
  }
  static auto Decode(const QCborValue& v, QList<T>& out, DecodeState& st)
      -> bool {
    if (!v.isArray()) return st.Fail(QStringLiteral("expected a list"));
    out.clear();
    for (const auto& e : v.toArray()) {
      T item{};
      if (!Codec<T>::Decode(e, item, st)) return false;
      out.append(std::move(item));
    }
    return true;
  }
};

namespace detail {

template <typename T>
struct IsOptional : std::false_type {};
template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename Tuple, typename Fn, size_t... I>
void ForEachField(const Tuple& t, Fn&& fn, std::index_sequence<I...>) {
  (fn(std::get<I>(t)), ...);
}

template <typename T, typename Fn>
void ForEachField(Fn&& fn) {
  constexpr auto fields = T::Fields();
  using Tuple = std::remove_const_t<decltype(fields)>;
  ForEachField(fields, std::forward<Fn>(fn),
               std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

}  // namespace detail

/// A reflected struct is a CBOR map with exactly its declared fields.
template <typename T>
struct Codec<T, std::enable_if_t<IsReflected<T>::value>> {
  static auto Schema() -> QCborMap {
    auto m = detail::Schema("object");
    QCborArray fields;
    detail::ForEachField<T>([&](const auto& f) {
      using M = std::remove_cv_t<
          std::remove_reference_t<decltype(std::declval<T>().*(f.member))>>;
      QCborMap field;
      field.insert(QStringLiteral("name"), QString::fromLatin1(f.name));
      field.insert(QStringLiteral("schema"), Codec<M>::Schema());
      fields.append(field);
    });
    m.insert(QStringLiteral("fields"), fields);
    return m;
  }

  static auto Encode(const T& v, EncodeState& st) -> QCborValue {
    return EncodeMap(v, st);
  }

  static auto EncodeMap(const T& v, EncodeState& st) -> QCborMap {
    QCborMap m;
    detail::ForEachField<T>([&](const auto& f) {
      m.insert(QString::fromLatin1(f.name), Codec<std::remove_cv_t<
                                                std::remove_reference_t<
                                                    decltype(v.*(f.member))>>>::
                                                Encode(v.*(f.member), st));
    });
    return m;
  }

  /// Strict: a field this type does not declare is an error, not ignored.
  /// A caller built against a newer schema is told so, instead of having
  /// what it asked for silently dropped.
  static auto Decode(const QCborValue& v, T& out, DecodeState& st) -> bool {
    if (!v.isMap()) return st.Fail(QStringLiteral("expected an object"));
    const auto m = v.toMap();
    int known = 0;
    bool ok = true;
    detail::ForEachField<T>([&](const auto& f) {
      if (!ok) return;
      using M = std::remove_cv_t<
          std::remove_reference_t<decltype(out.*(f.member))>>;
      const auto key = QString::fromLatin1(f.name);
      if (!m.contains(key)) {
        if constexpr (!detail::IsOptional<M>::value) {
          ok = st.Fail(QStringLiteral("missing field \"%1\"").arg(key));
        }
        return;
      }
      ++known;
      if (!Codec<M>::Decode(m.value(key), out.*(f.member), st)) {
        st.error = QStringLiteral("field \"%1\": %2").arg(key, st.error);
        ok = false;
      }
    });
    if (!ok) return false;
    if (m.size() != known) {
      return st.Fail(QStringLiteral("unknown field in object"));
    }
    return true;
  }
};

// ------------------------------------------------------------------ outcome

/// The result of a synchronous handler, or of a synchronous invoke.
template <typename R>
struct Outcome {
  int status = GF_CMD_OK;
  R value{};
  QString error;

  [[nodiscard]] auto Ok() const -> bool { return status == GF_CMD_OK; }

  static auto Success(R v) -> Outcome { return {GF_CMD_OK, std::move(v), {}}; }
  static auto Failure(int status, QString why) -> Outcome {
    return {status == GF_CMD_OK ? GF_CMD_E_FAILED : status, R{},
            std::move(why)};
  }
};

/// What a finished call hands back, before it is decoded into a type.
struct RawResult {
  int status = GF_CMD_OK;
  qint64 call_id = 0;  ///< filled in by the registry when it delivers
  QCborMap result;
  std::vector<Blob> blobs;
  QString error;
};

using Completer = std::function<void(RawResult)>;

/**
 * @brief The situation a command runs in. Built by the Host, never by the
 *        caller, so a command can trust it.
 *
 * It says who asked and what the user is looking at. It never carries
 * content: a command that needs the document's bytes asks for them, under
 * its own capabilities.
 */
struct CommandContext {
  QString caller;       ///< module id, or "" for the Host itself
  qint64 caller_caps = 0;
  QString source;       ///< "host", "module", "lua:<chunk>:<id>"
  std::optional<DocumentRef> document;
  bool has_selection = false;
  std::optional<KeyRef> key;
  qint64 call_id = 0;

  /// Not serialized: set by whichever side runs the handler.
  std::function<bool()> cancelled;

  [[nodiscard]] auto Cancelled() const -> bool {
    return cancelled ? cancelled() : false;
  }

  static constexpr auto Fields() {
    return std::make_tuple(F("caller", &CommandContext::caller),
                           F("caller_caps", &CommandContext::caller_caps),
                           F("source", &CommandContext::source),
                           F("document", &CommandContext::document),
                           F("has_selection", &CommandContext::has_selection),
                           F("key", &CommandContext::key),
                           F("call_id", &CommandContext::call_id));
  }
};

/// Encode a reflected value to its CBOR map, collecting its Blobs.
template <typename T>
auto EncodeMap(const T& v, EncodeState& st) -> QCborMap {
  static_assert(IsReflected<T>::value, "arguments and results are structs");
  return Codec<T>::EncodeMap(v, st);
}

/// Decode a reflected value from a CBOR map and its Blobs.
template <typename T>
auto DecodeMap(const QCborMap& m, const std::vector<Blob>& blobs, T& out,
               QString* error = nullptr) -> bool {
  static_assert(IsReflected<T>::value, "arguments and results are structs");
  DecodeState st;
  st.blobs = &blobs;
  const auto ok = Codec<T>::Decode(QCborValue(m), out, st);
  if (!ok && error != nullptr) *error = st.error;
  return ok;
}

/**
 * @brief Finish a deferred call. One-shot: the first Ok/Fail wins, and a
 *        Reply dropped without either reports the call as failed rather
 *        than leaving the caller waiting forever.
 */
template <typename R>
class Reply {
 public:
  explicit Reply(Completer done)
      : state_(std::make_shared<State>(std::move(done))) {}

  void Ok(const R& value) const {
    auto done = state_->Take();
    if (!done) return;
    EncodeState st;
    auto m = EncodeMap(value, st);
    done(RawResult{GF_CMD_OK, 0, std::move(m), std::move(st.blobs), {}});
  }

  void Fail(int status, const QString& why) const {
    auto done = state_->Take();
    if (!done) return;
    done(RawResult{status == GF_CMD_OK ? GF_CMD_E_FAILED : status, 0, {}, {},
                   why});
  }

 private:
  struct State {
    explicit State(Completer d) : done(std::move(d)) {}
    ~State() {
      if (done) {
        done(RawResult{GF_CMD_E_FAILED, 0, {}, {},
                       QStringLiteral("the handler never replied")});
      }
    }
    State(const State&) = delete;
    auto operator=(const State&) -> State& = delete;
    auto Take() -> Completer {
      Completer d;
      std::swap(d, done);
      return d;
    }
    Completer done;
  };
  std::shared_ptr<State> state_;
};

// ------------------------------------------------------------------ descriptor

/// The descriptor of @p C: metadata plus both schemas. The one source for
/// the registry, the marshalling, the Lua conversion and the documentation.
template <typename C>
auto Describe() -> QCborMap {
  QCborMap m;
  m.insert(QStringLiteral("id"), QString::fromUtf8(C::kMeta.id));
  m.insert(QStringLiteral("title"), QString::fromUtf8(C::kMeta.title));
  m.insert(QStringLiteral("description"),
           QString::fromUtf8(C::kMeta.description));
  m.insert(QStringLiteral("category"), QString::fromUtf8(C::kMeta.category));
  m.insert(QStringLiteral("required_caps"),
           static_cast<qint64>(C::kMeta.required_caps));
  m.insert(QStringLiteral("flags"), static_cast<qint64>(C::kMeta.flags));
  m.insert(QStringLiteral("args"), Codec<typename C::Args>::Schema());
  m.insert(QStringLiteral("result"), Codec<typename C::Result>::Schema());
  return m;
}

// ------------------------------------------------------------------ binding

/**
 * @brief A command's implementation, with the types erased.
 *
 * Neither side of the boundary is visible here: the Host registers a
 * Binding directly, and a module's runtime wraps it in the C callbacks.
 */
struct Binding {
  const char* id;
  auto (*describe)() -> QCborMap;
  void (*run)(const CommandContext& ctx, QCborMap args,
              std::vector<Blob> blobs, Completer done);
  auto (*state)(const CommandContext& ctx) -> uint32_t;  ///< may be null
};

namespace detail {

template <typename C, typename = void>
struct HasState : std::false_type {};
template <typename C>
struct HasState<C, std::void_t<decltype(C::State(
                       std::declval<const CommandContext&>()))>>
    : std::true_type {};

template <typename C, auto Handler>
void Run(const CommandContext& ctx, QCborMap args, std::vector<Blob> blobs,
         Completer done) {
  using Args = typename C::Args;
  using Result = typename C::Result;

  Args decoded{};
  QString error;
  if (!DecodeMap(args, blobs, decoded, &error)) {
    done(RawResult{GF_CMD_E_BAD_ARGS, 0, {}, {}, error});
    return;
  }

  if constexpr (std::is_invocable_r_v<Outcome<Result>, decltype(Handler),
                                      const CommandContext&, const Args&>) {
    auto outcome = Handler(ctx, decoded);
    if (!outcome.Ok()) {
      done(RawResult{outcome.status, 0, {}, {}, outcome.error});
      return;
    }
    EncodeState st;
    auto m = EncodeMap(outcome.value, st);
    done(RawResult{GF_CMD_OK, 0, std::move(m), std::move(st.blobs), {}});
  } else {
    static_assert(std::is_invocable_v<decltype(Handler), const CommandContext&,
                                      Args, Reply<Result>>,
                  "a handler is Outcome<Result>(const CommandContext&, const "
                  "Args&) or void(const CommandContext&, Args, Reply<Result>)");
    Handler(ctx, std::move(decoded), Reply<Result>(std::move(done)));
  }
}

}  // namespace detail

/// The one registration helper. `Bind<MyCommand, &MyHandler>()`.
template <typename C, auto Handler>
auto Bind() -> Binding {
  static_assert(IsReflected<typename C::Args>::value,
                "C::Args needs static constexpr auto Fields()");
  static_assert(IsReflected<typename C::Result>::value,
                "C::Result needs static constexpr auto Fields()");
  Binding b{};
  b.id = C::kMeta.id;
  b.describe = &Describe<C>;
  b.run = &detail::Run<C, Handler>;
  if constexpr (detail::HasState<C>::value) {
    b.state = &C::State;
  } else {
    b.state = nullptr;
  }
  return b;
}

}  // namespace gf::cmd
