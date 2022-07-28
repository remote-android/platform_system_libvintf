/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <string>
#include <vector>

#include "apex_info_cache.h"

namespace android {
namespace vintf {

// APEX VINTF interface
class ApexInterface {
   public:
    virtual ~ApexInterface() = default;
    // Check if there is an update for the given type of APEX files in the system
    virtual bool HasUpdate() const = 0;
    // Get device VINTF directories
    virtual std::vector<std::string> DeviceVintfDirs() = 0;
};

namespace details {

// Provide ApexInterface using ApexInfoCache
class Apex : public ApexInterface {
   public:
    Apex(const std::string& apex_info_file = android::apex::info::kApexInfoFile);
    virtual bool HasUpdate() const override;
    virtual std::vector<std::string> DeviceVintfDirs() override;

   private:
    std::unique_ptr<android::apex::info::ApexInfoCache> apex_;
};

}  // namespace details
}  // namespace vintf
}  // namespace android
