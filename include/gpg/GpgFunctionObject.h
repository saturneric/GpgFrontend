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

#include "GpgFrontend.h"

namespace GpgFrontend {
    template<typename T>
    class SingletonFunctionObject {
    public:

        static T &getInstance()  {
            std::lock_guard<std::mutex> guard(_instance_mutex);
            if(_instance == nullptr) _instance = std::make_unique<T>();
            return *_instance;
        }

        SingletonFunctionObject(T&&) = delete;

        SingletonFunctionObject(const T&) = delete;

        void operator= (const T&) = delete;

    protected:

        SingletonFunctionObject() = default;

        virtual ~SingletonFunctionObject() = default;

    private:

        static std::mutex _instance_mutex;
        static std::unique_ptr<T> _instance;
    };

    template<typename T>
    std::mutex SingletonFunctionObject<T>::_instance_mutex;

    template<typename T>
    std::unique_ptr<T> SingletonFunctionObject<T>::_instance = nullptr;

}




#endif //GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H
