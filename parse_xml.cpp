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

// Convert objects from and to xml.

#include <tinyxml2.h>

#include "parse_string.h"
#include "parse_xml.h"

namespace android {
namespace vintf {

// --------------- tinyxml2 details

using NodeType = tinyxml2::XMLElement;
using DocType = tinyxml2::XMLDocument;

// caller is responsible for deleteDocument() call
inline DocType *createDocument() {
    return new tinyxml2::XMLDocument();
}

// caller is responsible for deleteDocument() call
inline DocType *createDocument(const std::string &xml) {
    DocType *doc = new tinyxml2::XMLDocument();
    if (doc->Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
        return doc;
    }
    delete doc;
    return nullptr;
}

inline void deleteDocument(DocType *d) {
    delete d;
}

inline std::string printDocument(DocType *d) {
    tinyxml2::XMLPrinter p;
    d->Print(&p);
    return std::string{p.CStr()};
}

inline NodeType *createNode(const std::string &name, DocType *d) {
    return d->NewElement(name.c_str());
}

inline void appendChild(NodeType *parent, NodeType *child) {
    parent->InsertEndChild(child);
}

inline void appendChild(DocType *parent, NodeType *child) {
    parent->InsertEndChild(child);
}

inline void appendStrAttr(NodeType *e, const std::string &attrName, const std::string &attr) {
    e->SetAttribute(attrName.c_str(), attr.c_str());
}

// text -> text
inline void appendText(NodeType *parent, const std::string &text, DocType *d) {
    parent->InsertEndChild(d->NewText(text.c_str()));
}

inline std::string nameOf(NodeType *root) {
    return root->Name() == NULL ? "" : root->Name();
}

inline std::string getText(NodeType *root) {
    return root->GetText() == NULL ? "" : root->GetText();
}

inline NodeType *getChild(NodeType *parent, const std::string &name) {
    return parent->FirstChildElement(name.c_str());
}

inline NodeType *getRootChild(DocType *parent) {
    return parent->FirstChildElement();
}

inline std::vector<NodeType *> getChildren(NodeType *parent, const std::string &name) {
    std::vector<NodeType *> v;
    for (NodeType *child = parent->FirstChildElement(name.c_str());
         child != nullptr;
         child = child->NextSiblingElement(name.c_str())) {
        v.push_back(child);
    }
    return v;
}

inline bool getAttr(NodeType *root, const std::string &attrName, std::string *s) {
    const char *c = root->Attribute(attrName.c_str());
    if (c == NULL)
        return false;
    *s = c;
    return true;
}

// --------------- tinyxml2 details end.

// ---------------------- XmlNodeConverter definitions

template<typename Object>
struct XmlNodeConverter : public XmlConverter<Object> {
    XmlNodeConverter() {}
    virtual ~XmlNodeConverter() {}

    // sub-types should implement these.
    virtual void mutateNode(const Object &o, NodeType *n, DocType *d) const = 0;
    virtual bool buildObject(Object *o, NodeType *n) const = 0;
    virtual std::string elementName() const = 0;

    // convenience methods for user
    inline const std::string &lastError() const { return mLastError; }
    inline NodeType *serialize(const Object &o, DocType *d) const {
        NodeType *root = createNode(this->elementName(), d);
        this->mutateNode(o, root, d);
        return root;
    }
    inline std::string serialize(const Object &o) const {
        DocType *doc = createDocument();
        appendChild(doc, serialize(o, doc));
        std::string s = printDocument(doc);
        deleteDocument(doc);
        return s;
    }
    inline bool deserialize(Object *object, NodeType *root) const {
        if (nameOf(root) != this->elementName()) {
            return false;
        }
        return this->buildObject(object, root);
    }
    inline bool deserialize(Object *o, const std::string &xml) const {
        DocType *doc = createDocument(xml);
        bool ret = doc != nullptr
            && deserialize(o, getRootChild(doc));
        deleteDocument(doc);
        return ret;
    }
    inline NodeType *operator()(const Object &o, DocType *d) const {
        return serialize(o, d);
    }
    inline std::string operator()(const Object &o) const {
        return serialize(o);
    }
    inline bool operator()(Object *o, NodeType *node) const {
        return deserialize(o, node);
    }
    inline bool operator()(Object *o, const std::string &xml) const {
        return deserialize(o, xml);
    }

    // convenience methods for implementor.
    template <typename T>
    inline void appendAttr(NodeType *e, const std::string &attrName, const T &attr) const {
        return appendStrAttr(e, attrName, ::android::vintf::to_string(attr));
    }

    inline void appendAttr(NodeType *e, const std::string &attrName, bool attr) const {
        return appendStrAttr(e, attrName, attr ? "true" : "false");
    }

    // text -> <name>text</name>
    inline void appendTextElement(NodeType *parent, const std::string &name,
                const std::string &text, DocType *d) const {
        NodeType *c = createNode(name, d);
        appendText(c, text, d);
        appendChild(parent, c);
    }

    // text -> <name>text</name>
    template<typename Array>
    inline void appendTextElements(NodeType *parent, const std::string &name,
                const Array &array, DocType *d) const {
        for (const std::string &text : array) {
            NodeType *c = createNode(name, d);
            appendText(c, text, d);
            appendChild(parent, c);
        }
    }

    template<typename T, typename Array>
    inline void appendChildren(NodeType *parent, const XmlNodeConverter<T> &conv,
            const Array &array, DocType *d) const {
        for (const T &t : array) {
            appendChild(parent, conv(t, d));
        }
    }

    template <typename T>
    inline bool parseAttr(NodeType *root, const std::string &attrName, T *attr) const {
        std::string attrText;
        bool ret = getAttr(root, attrName, &attrText) && ::android::vintf::parse(attrText, attr);
        if (!ret) {
            mLastError = "Could not parse attr with name " + attrName;
        }
        return ret;
    }

    inline bool parseAttr(NodeType *root, const std::string &attrName, std::string *attr) const {
        bool ret = getAttr(root, attrName, attr);
        if (!ret) {
            mLastError = "Could not find attr with name " + attrName;
        }
        return ret;
    }

    inline bool parseAttr(NodeType *root, const std::string &attrName, bool *attr) const {
        std::string attrText;
        if (!getAttr(root, attrName, &attrText)) {
            mLastError = "Could not find attr with name " + attrName;
            return false;
        }
        if (attrText == "true" || attrText == "1") {
            *attr = true;
            return true;
        }
        if (attrText == "false" || attrText == "0") {
            *attr = false;
            return true;
        }
        mLastError = "Could not parse attr with name \"" + attrName
                + "\" and value \"" + attrText + "\"";
        return false;
    }

    inline bool parseTextElement(NodeType *root,
            const std::string &elementName, std::string *s) const {
        NodeType *child = getChild(root, elementName);
        if (child == nullptr) {
            mLastError = "Could not find element with name " + elementName;
            return false;
        }
        *s = getText(child);
        return true;
    }

    inline bool parseTextElements(NodeType *root, const std::string &elementName,
            std::vector<std::string> *v) const {
        auto nodes = getChildren(root, elementName);
        v->resize(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i) {
            v->at(i) = getText(nodes[i]);
        }
        return true;
    }

    template <typename T>
    inline bool parseChild(NodeType *root, const XmlNodeConverter<T> &conv, T *t) const {
        NodeType *child = getChild(root, conv.elementName());
        if (child == nullptr) {
            mLastError = "Could not find element with name " + conv.elementName();
            return false;
        }
        bool success = conv.deserialize(t, child);
        if (!success) {
            mLastError = conv.lastError();
        }
        return success;
    }

    template <typename T>
    inline bool parseChildren(NodeType *root, const XmlNodeConverter<T> &conv, std::vector<T> *v) const {
        auto nodes = getChildren(root, conv.elementName());
        v->resize(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (!conv.deserialize(&v->at(i), nodes[i])) {
                mLastError = "Could not parse element with name " + conv.elementName();
                return false;
            }
        }
        return true;
    }

    inline bool parseText(NodeType *node, std::string *s) const {
        *s = getText(node);
        return true;
    }

protected:
    mutable std::string mLastError;
};

template<typename Object>
struct XmlTextConverter : public XmlNodeConverter<Object> {
    XmlTextConverter(const std::string &elementName)
        : mElementName(elementName) {}

    virtual void mutateNode(const Object &object, NodeType *root, DocType *d) const override {
        appendText(root, ::android::vintf::to_string(object), d);
    }
    virtual bool buildObject(Object *object, NodeType *root) const override {
        std::string text = getText(root);
        bool ret = ::android::vintf::parse(text, object);
        if (!ret) {
            this->mLastError = "Could not parse " + text;
        }
        return ret;
    }
    virtual std::string elementName() const { return mElementName; };
private:
    std::string mElementName;
};

// ---------------------- XmlNodeConverter definitions end

const XmlTextConverter<Version> versionConverter{"version"};

const XmlTextConverter<VersionRange> versionRangeConverter{"version"};

const XmlTextConverter<Transport> transportConverter{"transport"};

const XmlTextConverter<KernelConfigKey> kernelConfigKeyConverter{"key"};

struct KernelConfigTypedValueConverter : public XmlNodeConverter<KernelConfigTypedValue> {
    std::string elementName() const override { return "value"; }
    void mutateNode(const KernelConfigTypedValue &object, NodeType *root, DocType *d) const override {
        appendAttr(root, "type", object.mType);
        appendText(root, ::android::vintf::to_string(object), d);
    }
    bool buildObject(KernelConfigTypedValue *object, NodeType *root) const override {
        std::string stringValue;
        if (!parseAttr(root, "type", &object->mType) ||
            !parseText(root, &stringValue)) {
            return false;
        }
        if (!::android::vintf::parseKernelConfigValue(stringValue, object)) {
            this->mLastError = "Could not parse kernel config value \"" + stringValue + "\"";
            return false;
        }
        return true;
    }
};

const KernelConfigTypedValueConverter kernelConfigTypedValueConverter{};

struct KernelConfigConverter : public XmlNodeConverter<KernelConfig> {
    std::string elementName() const override { return "config"; }
    void mutateNode(const KernelConfig &object, NodeType *root, DocType *d) const override {
        appendChild(root, kernelConfigKeyConverter(object.first, d));
        appendChild(root, kernelConfigTypedValueConverter(object.second, d));
    }
    bool buildObject(KernelConfig *object, NodeType *root) const override {
        if (   !parseChild(root, kernelConfigKeyConverter, &object->first)
            || !parseChild(root, kernelConfigTypedValueConverter, &object->second)) {
            return false;
        }
        return true;
    }
};

const KernelConfigConverter kernelConfigConverter{};

struct MatrixHalConverter : public XmlNodeConverter<MatrixHal> {
    std::string elementName() const override { return "hal"; }
    void mutateNode(const MatrixHal &hal, NodeType *root, DocType *d) const override {
        appendAttr(root, "format", hal.format);
        appendAttr(root, "optional", hal.optional);
        appendTextElement(root, "name", hal.name, d);
        appendChildren(root, versionRangeConverter, hal.versionRanges, d);
    }
    bool buildObject(MatrixHal *object, NodeType *root) const override {
        if (!parseAttr(root, "format", &object->format) ||
            !parseAttr(root, "optional", &object->optional) ||
            !parseTextElement(root, "name", &object->name) ||
            !parseChildren(root, versionRangeConverter, &object->versionRanges)) {
            return false;
        }
        return true;
    }
};

const MatrixHalConverter matrixHalConverter{};

struct MatrixKernelConverter : public XmlNodeConverter<MatrixKernel> {
    std::string elementName() const override { return "kernel"; }
    void mutateNode(const MatrixKernel &kernel, NodeType *root, DocType *d) const override {
        appendAttr(root, "version", kernel.mMinLts);
        appendChildren(root, kernelConfigConverter, kernel.mConfigs, d);
    }
    bool buildObject(MatrixKernel *object, NodeType *root) const override {
        if (!parseAttr(root, "version", &object->mMinLts) ||
            !parseChildren(root, kernelConfigConverter, &object->mConfigs)) {
            return false;
        }
        return true;
    }
};

const MatrixKernelConverter matrixKernelConverter{};

struct HalImplementationConverter : public XmlNodeConverter<HalImplementation> {
    std::string elementName() const override { return "impl"; }
    void mutateNode(const HalImplementation &impl, NodeType *root, DocType *d) const override {
        appendAttr(root, "level", impl.implLevel);
        appendText(root, impl.impl, d);
    }
    bool buildObject(HalImplementation *object, NodeType *root) const override {
        if (!parseAttr(root, "level", &object->implLevel) ||
            !parseText(root, &object->impl)) {
            return false;
        }
        return true;
    }
};

const HalImplementationConverter halImplementationConverter{};

struct ManfiestHalInterfaceConverter : public XmlNodeConverter<ManifestHalInterface> {
    std::string elementName() const override { return "interface"; }
    void mutateNode(const ManifestHalInterface &intf, NodeType *root, DocType *d) const override {
        appendTextElement(root, "name", intf.name, d);
        appendTextElements(root, "instance", intf.instances, d);
    }
    bool buildObject(ManifestHalInterface *intf, NodeType *root) const override {
        std::vector<std::string> instances;
        if (!parseTextElement(root, "name", &intf->name) ||
            !parseTextElements(root, "instance", &instances)) {
            return false;
        }
        intf->instances.clear();
        intf->instances.insert(instances.begin(), instances.end());
        if (intf->instances.size() != instances.size()) {
            this->mLastError = "Duplicated instances in " + intf->name;
            return false;
        }
        return true;
    }
};

const ManfiestHalInterfaceConverter manfiestHalInterfaceConverter{};

struct ManifestHalConverter : public XmlNodeConverter<ManifestHal> {
    std::string elementName() const override { return "hal"; }
    void mutateNode(const ManifestHal &hal, NodeType *root, DocType *d) const override {
        appendAttr(root, "format", hal.format);
        appendTextElement(root, "name", hal.name, d);
        appendChild(root, transportConverter(hal.transport, d));
        appendChild(root, halImplementationConverter(hal.impl, d));
        appendChildren(root, versionConverter, hal.versions, d);
        appendChildren(root, manfiestHalInterfaceConverter, iterateValues(hal.interfaces), d);
    }
    bool buildObject(ManifestHal *object, NodeType *root) const override {
        std::vector<ManifestHalInterface> interfaces;
        if (!parseAttr(root, "format", &object->format) ||
            !parseTextElement(root, "name", &object->name) ||
            !parseChild(root, transportConverter, &object->transport) ||
            !parseChild(root, halImplementationConverter, &object->impl) ||
            !parseChildren(root, versionConverter, &object->versions) ||
            !parseChildren(root, manfiestHalInterfaceConverter, &interfaces)) {
            return false;
        }
        object->interfaces.clear();
        for (auto &&interface : interfaces) {
            auto res = object->interfaces.emplace(interface.name,
                                                  std::move(interface));
            if (!res.second) {
                this->mLastError = "Duplicated instance entry " + res.first->first;
                return false;
            }
        }
        return object->isValid();
    }
};

// Convert ManifestHal from and to XML. Returned object is guaranteed to have
// .isValid() == true.
const ManifestHalConverter manifestHalConverter{};

const XmlTextConverter<KernelSepolicyVersion> kernelSepolicyVersionConverter{"kernel-sepolicy-version"};
const XmlTextConverter<SepolicyVersion> sepolicyVersionConverter{"sepolicy-version"};

struct SepolicyConverter : public XmlNodeConverter<Sepolicy> {
    std::string elementName() const override { return "sepolicy"; }
    void mutateNode(const Sepolicy &object, NodeType *root, DocType *d) const override {
        appendChild(root, kernelSepolicyVersionConverter(object.kernelSepolicyVersion(), d));
        appendChild(root, sepolicyVersionConverter(object.sepolicyVersion(), d));
    }
    bool buildObject(Sepolicy *object, NodeType *root) const override {
        if (!parseChild(root, kernelSepolicyVersionConverter, &object->mKernelSepolicyVersion) ||
            !parseChild(root, sepolicyVersionConverter, &object->mSepolicyVersion)) {
            return false;
        }
        return true;
    }
};
const SepolicyConverter sepolicyConverter{};

struct HalManifestConverter : public XmlNodeConverter<HalManifest> {
    std::string elementName() const override { return "manifest"; }
    void mutateNode(const HalManifest &m, NodeType *root, DocType *d) const override {
        appendAttr(root, "version", HalManifest::kVersion);
        appendChildren(root, manifestHalConverter, m.getHals(), d);
    }
    bool buildObject(HalManifest *object, NodeType *root) const override {
        std::vector<ManifestHal> hals;
        if (!parseChildren(root, manifestHalConverter, &hals)) {
            return false;
        }
        for (auto &&hal : hals) {
            if (!object->add(std::move(hal))) {
                this->mLastError = "Duplicated manifest.hal entry";
                return false;
            }
        }
        return true;
    }
};

const HalManifestConverter halManifestConverter{};

struct CompatibilityMatrixConverter : public XmlNodeConverter<CompatibilityMatrix> {
    std::string elementName() const override { return "compatibility-matrix"; }
    void mutateNode(const CompatibilityMatrix &m, NodeType *root, DocType *d) const override {
        appendAttr(root, "version", CompatibilityMatrix::kVersion);
        appendChildren(root, matrixHalConverter, iterateValues(m.mHals), d);
        appendChildren(root, matrixKernelConverter, m.mKernels, d);
        appendChild(root, sepolicyConverter(m.mSepolicy, d));
    }
    bool buildObject(CompatibilityMatrix *object, NodeType *root) const override {
        std::vector<MatrixHal> hals;
        if (!parseChildren(root, matrixHalConverter, &hals) ||
            !parseChildren(root, matrixKernelConverter, &object->mKernels) ||
            !parseChild(root, sepolicyConverter, &object->mSepolicy)) {
            return false;
        }
        for (auto &&hal : hals) {
            if (!object->add(std::move(hal))) {
                this->mLastError = "Duplicated compatibility-matrix.hal entry";
                return false;
            }
        }
        return true;
    }
};

const CompatibilityMatrixConverter compatibilityMatrixConverter{};

// Publicly available as in parse_xml.h
const XmlConverter<HalManifest> &gHalManifestConverter = halManifestConverter;
const XmlConverter<CompatibilityMatrix> &gCompatibilityMatrixConverter
        = compatibilityMatrixConverter;

// For testing in LibVintfTest
const XmlConverter<Version> &gVersionConverter = versionConverter;
const XmlConverter<KernelConfigTypedValue> &gKernelConfigTypedValueConverter
        = kernelConfigTypedValueConverter;
const XmlConverter<MatrixHal> &gMatrixHalConverter = matrixHalConverter;
const XmlConverter<HalImplementation> &gHalImplementationConverter = halImplementationConverter;

} // namespace vintf
} // namespace android