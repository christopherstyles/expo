/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ABI43_0_0TurboModuleBinding.h"

#include <stdexcept>
#include <string>

#include <ABI43_0_0ReactCommon/ABI43_0_0LongLivedObject.h>
#include <ABI43_0_0cxxreact/ABI43_0_0SystraceSection.h>

using namespace ABI43_0_0facebook;

namespace ABI43_0_0facebook {
namespace ABI43_0_0React {

/**
 * Public API to install the TurboModule system.
 */
TurboModuleBinding::TurboModuleBinding(
    const TurboModuleProviderFunctionType &&moduleProvider)
    : moduleProvider_(std::move(moduleProvider)) {}

void TurboModuleBinding::install(
    jsi::Runtime &runtime,
    const TurboModuleProviderFunctionType &&moduleProvider) {
  runtime.global().setProperty(
      runtime,
      "__turboModuleProxy",
      jsi::Function::createFromHostFunction(
          runtime,
          jsi::PropNameID::forAscii(runtime, "__turboModuleProxy"),
          1,
          [binding =
               std::make_shared<TurboModuleBinding>(std::move(moduleProvider))](
              jsi::Runtime &rt,
              const jsi::Value &thisVal,
              const jsi::Value *args,
              size_t count) {
            return binding->jsProxy(rt, thisVal, args, count);
          }));
}

TurboModuleBinding::~TurboModuleBinding() {
  LongLivedObjectCollection::get().clear();
}

std::shared_ptr<TurboModule> TurboModuleBinding::getModule(
    const std::string &name,
    const jsi::Value *schema) {
  std::shared_ptr<TurboModule> module = nullptr;
  {
    SystraceSection s("TurboModuleBinding::getModule", "module", name);
    module = moduleProvider_(name, schema);
  }
  return module;
}

jsi::Value TurboModuleBinding::jsProxy(
    jsi::Runtime &runtime,
    const jsi::Value &thisVal,
    const jsi::Value *args,
    size_t count) {
  if (count < 1) {
    throw std::invalid_argument(
        "__turboModuleProxy must be called with at least 1 argument");
  }
  std::string moduleName = args[0].getString(runtime).utf8(runtime);
  jsi::Value nullSchema = jsi::Value::undefined();

  std::shared_ptr<TurboModule> module =
      (count >= 2 ? getModule(moduleName, &args[1])
                  : getModule(moduleName, &nullSchema));

  if (module == nullptr) {
    return jsi::Value::null();
  }

  return jsi::Object::createFromHostObject(runtime, std::move(module));
}

} // namespace ABI43_0_0React
} // namespace ABI43_0_0facebook
