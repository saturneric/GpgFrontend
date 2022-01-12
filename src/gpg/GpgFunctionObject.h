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

#ifndef GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H
#define GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>

#include "GpgConstants.h"

namespace GpgFrontend {

template <typename T>
class SingletonFunctionObject {
 public:
  static T& GetInstance(
      int channel = GpgFrontend::GPGFRONTEND_DEFAULT_CHANNEL) {
    static_assert(std::is_base_of<SingletonFunctionObject<T>, T>::value,
                  "T not derived from SingletonFunctionObject<T>");

    auto _p_pbj = find_object_in_channel(channel);
    if (_p_pbj == nullptr)
      return *set_object_in_channel(channel, std::make_unique<T>(channel));
    else
      return *_p_pbj;
  }

  static T& CreateInstance(int channel,
                           std::function<std::unique_ptr<T>(void)> factory) {
    static_assert(std::is_base_of<SingletonFunctionObject<T>, T>::value,
                  "T not derived from SingletonFunctionObject<T>");

    auto _p_pbj = find_object_in_channel(channel);
    if (_p_pbj == nullptr)
      return *set_object_in_channel(channel, std::move(factory()));
    else
      return *_p_pbj;
  }

  static T& CreateInstance(int channel, std::unique_ptr<T> p_obj = nullptr) {
    static_assert(std::is_base_of<SingletonFunctionObject<T>, T>::value,
                  "T not derived from SingletonFunctionObject<T>");

    auto _p_pbj = find_object_in_channel(channel);
    if (_p_pbj == nullptr)
      return *set_object_in_channel(channel, std::move(p_obj));
    else
      return *_p_pbj;
  }

  static T& ReleaseChannel(int channel) {
    decltype(_instances_map.end()) _it;
    {
      std::shared_lock lock(_instances_mutex);
      _it = _instances_map.find(channel);
    }
    if (_it != _instances_map.end()) _instances_map.erase(_it);
    DLOG(INFO) << "channel" << channel << "released";
  }

  static int GetDefaultChannel() { return _default_channel; }

  [[nodiscard]] int GetChannel() const { return channel_; }

  SingletonFunctionObject(T&&) = delete;

  SingletonFunctionObject(const T&) = delete;

  void operator=(const T&) = delete;

 protected:
  SingletonFunctionObject() = default;

  explicit SingletonFunctionObject(int channel) : channel_(channel) {}

  virtual ~SingletonFunctionObject() = default;

  void SetChannel(int channel) { this->channel_ = channel; }

 private:
  int channel_ = _default_channel;
  static int _default_channel;
  static std::mutex _instance_mutex;
  static std::shared_mutex _instances_mutex;
  static std::unique_ptr<T> _instance;
  static std::map<int, std::unique_ptr<T>> _instances_map;

  static T* find_object_in_channel(int channel) {
    // read _instances_map
    decltype(_instances_map.end()) _it;
    {
      std::shared_lock lock(_instances_mutex);
      _it = _instances_map.find(channel);
      if (_it == _instances_map.end())
        return nullptr;
      else
        return _it->second.get();
    }
  }

  static T* set_object_in_channel(int channel, std::unique_ptr<T> p_obj) {
    {
      if (p_obj == nullptr) p_obj = std::make_unique<T>();
      T* obj = p_obj.get();
      obj->SetChannel(channel);
      {
        std::unique_lock lock(_instances_mutex);
        _instances_map.insert({channel, std::move(p_obj)});
      }
      return obj;
    }
  }
};

template <typename T>
int SingletonFunctionObject<T>::_default_channel =
    GpgFrontend::GPGFRONTEND_DEFAULT_CHANNEL;

template <typename T>
std::mutex SingletonFunctionObject<T>::_instance_mutex;

template <typename T>
std::shared_mutex SingletonFunctionObject<T>::_instances_mutex;

template <typename T>
std::unique_ptr<T> SingletonFunctionObject<T>::_instance = nullptr;

template <typename T>
std::map<int, std::unique_ptr<T>> SingletonFunctionObject<T>::_instances_map;

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H
