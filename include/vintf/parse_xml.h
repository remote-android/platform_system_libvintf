/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_VINTF_PARSE_XML_H
#define ANDROID_VINTF_PARSE_XML_H

#include "CompatibilityMatrix.h"
#include "HalManifest.h"
#include "SerializeFlags.h"

namespace android {
namespace vintf {

template<typename Object>
struct XmlConverter {
    XmlConverter() {}
    virtual ~XmlConverter() {}

    // Serialize an object to XML.
    virtual std::string operator()(
        const Object& o, SerializeFlags::Type flags = SerializeFlags::EVERYTHING) const = 0;

    // Deserialize an XML to object. Return whether it is successful.
    // Sets error message to optional "error" out parameter (which can be null).
    virtual bool operator()(Object* o, const std::string& xml,
                            std::string* error = nullptr) const = 0;
};

extern XmlConverter<HalManifest>& gHalManifestConverter;

extern XmlConverter<CompatibilityMatrix>& gCompatibilityMatrixConverter;

} // namespace vintf
} // namespace android

#endif // ANDROID_VINTF_PARSE_XML_H
