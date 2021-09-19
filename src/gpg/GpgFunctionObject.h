/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include <easyloggingpp/easylogging++.h>

namespace GpgFrontend {

template <typename T>
class SingletonFunctionObject {
 public:
  static T& GetInstance(int channel = 0) {
    DLOG(INFO) << "SingletonFunctionObject GetInstance Calling "
               << typeid(T).name() << " channel " << channel;
    if (!channel) {
      std::lock_guard<std::mutex> guard(_instance_mutex);
      if (_instance == nullptr)
        _instance = std::make_unique<T>();
      return *_instance;
    } else {
      // read _instances_map
      decltype(_instances_map.end()) _it;
      {
        std::shared_lock lock(_instances_mutex);
        _it = _instances_map.find(channel);
      }
      if (_it != _instances_map.end())
        return *_it->second;
      else
        return CreateInstance(channel);
    }
  }

  static T& CreateInstance(int channel, std::unique_ptr<T> p_obj = nullptr) {
    DLOG(INFO) << "SingletonFunctionObject CreateInstance Calling "
               << typeid(T).name() << " channel " << channel;
    if (!channel)
      return *_instance;

    // read _instances_map
    decltype(_instances_map.end()) _it;
    {
      std::shared_lock lock(_instances_mutex);
      _it = _instances_map.find(channel);
    }
    if (_it == _instances_map.end()) {
      {
        std::lock_guard<std::mutex> guard(_default_channel_mutex);
        int tmp = channel;
        std::swap(_default_channel, tmp);
        if (p_obj == nullptr)
          p_obj = std::make_unique<T>();
        std::swap(_default_channel, tmp);
      }
      T* obj = p_obj.get();

      // change _instances_map
      {
        std::unique_lock lock(_instances_mutex);
        _instances_map.insert({channel, std::move(p_obj)});
      }
      return *obj;
    } else {
      return *_it->second;
    }
  }

  static int GetDefaultChannel() { return _default_channel; }

  int GetChannel() const { return channel_; }

  SingletonFunctionObject(T&&) = delete;

  SingletonFunctionObject(const T&) = delete;

  void operator=(const T&) = delete;

 protected:
  SingletonFunctionObject() {}

  SingletonFunctionObject(int channel) : channel_(channel) {}

  virtual ~SingletonFunctionObject() = default;

 private:
  int channel_ = _default_channel;
  static int _default_channel;
  static std::mutex _default_channel_mutex;
  static std::mutex _instance_mutex;
  static std::shared_mutex _instances_mutex;
  static std::unique_ptr<T> _instance;
  static std::map<int, std::unique_ptr<T>> _instances_map;
};

template <typename T>
int SingletonFunctionObject<T>::_default_channel = 0;

template <typename T>
std::mutex SingletonFunctionObject<T>::_default_channel_mutex;

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
