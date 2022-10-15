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
#include "Apex.h"

#include <android-base/logging.h>

#include "constants-private.h"

namespace android {
namespace vintf {
namespace details {

using android::apex::info::ApexInfoCache;
using android::apex::info::ApexType;

Apex::Apex(const std::string& apex_info_file)
    : apex_(std::make_unique<ApexInfoCache>(apex_info_file)) {}

std::vector<std::string> Apex::DeviceVintfDirs() {
    std::vector<std::string> vendor;
    std::vector<std::string> odm;
    // Update cache
    auto ret = apex_->Update();
    if (!ret.ok()) {
        LOG(INFO) << "Could not update APEX info " << ret.error();
        // Continue using existing data in cache
    }
    for (const auto& obj : apex_->Info()) {
        switch (obj.Type()) {
            case (ApexType::kVendor):
                vendor.emplace_back(obj.Path() + "/" + details::kVintfSubDir);
                break;
            case (ApexType::kOdm):
                odm.emplace_back(obj.Path() + "/" + details::kVintfSubDir);
                break;
            default:
                break;
        }
    }
    if (vendor.empty()) {
        return odm;
    }

    std::move(odm.begin(), odm.end(), std::back_inserter(vendor));
    return vendor;
}

bool Apex::HasUpdate() const {
    // Currently only support updating vendor apex.
    auto ret = apex_->HasNewData();
    if (!ret.ok()) {
        LOG(INFO) << "Could not determine new APEX data" << std::endl;
        return false;
    }
    return *ret;
}

}  // namespace details
}  // namespace vintf
}  // namespace android
