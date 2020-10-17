/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <utils/Errors.h>
#include <vintf/Dirmap.h>
#include <vintf/FileSystem.h>

namespace android::vintf::details {

class HostFileSystem : public FileSystemImpl {
   public:
    HostFileSystem(const Dirmap& dirmap, status_t missingError);

    status_t fetch(const std::string& path, std::string* fetched,
                   std::string* error) const override;

    status_t listFiles(const std::string& path, std::vector<std::string>* out,
                       std::string* error) const override;

   private:
    std::string resolve(const std::string& path, std::string* error) const;

    Dirmap mDirMap;
    status_t mMissingError;
};

}  // namespace android::vintf::details
