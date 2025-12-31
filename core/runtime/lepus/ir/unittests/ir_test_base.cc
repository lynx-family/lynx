// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

#include "base/include/value/table.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/lepus_date.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

static const char* kCFuncPrint = "print";
static void PrintValue(lepus::Value* val, std::ostream& output);

static void DumpTable(lepus::Value* val, std::ostream& output) {
  auto it = val->Table()->begin();
  output << "{ " << std::endl;
  for (; it != val->Table()->end(); it++) {
    output << it->first.str() << " : ";
    PrintValue(&(it->second), output);
    output << "\n";
  }
  output << "} " << std::endl;
}

static void PrintValue(lepus::Value* val, std::ostream& output) {
  switch (val->Type()) {
    case lepus::ValueType::Value_Nil:
      output << "null";
      break;
    case lepus::ValueType::Value_Undefined:
      output << "undefined";
      break;
    case lepus::ValueType::Value_Double:
      output << val->Number();
      break;
    case lepus::ValueType::Value_Int32:
      output << val->Int32();
      break;
    case lepus::ValueType::Value_Int64:
      output << val->Int64();
      break;
    case lepus::ValueType::Value_UInt32:
      output << val->UInt32();
      break;
    case lepus::ValueType::Value_UInt64:
      output << val->UInt64();
      break;
    case lepus::ValueType::Value_Bool:
      output << (val->Bool() ? "true" : "false");
      break;
    case lepus::ValueType::Value_String:
      output << "'" << val->StdString() << "'";
      break;
    case lepus::ValueType::Value_Table:
      DumpTable(val, output);
      break;
    case lepus::ValueType::Value_Array:
      output << "[";
      for (size_t i = 0; i < val->Array()->size(); i++) {
        PrintValue(const_cast<lepus::Value*>(&(val->Array()->get(i))), output);
        if (i != (val->Array()->size() - 1)) {
          output << ", ";
        }
      }
      output << "]";
      break;
    case lepus::ValueType::Value_CDate:
      lynx::fml::static_ref_ptr_cast<lepus::CDate>(val->RefCounted())
          ->print(output);
      break;
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
      output << "closure/cfunction/cpointer/refcounted" << std::endl;
      break;
    case lepus::ValueType::Value_RegExp:
      output << "regexp" << std::endl;
      output << "pattern: "
             << lynx::fml::static_ref_ptr_cast<lepus::RegExp>(val->RefCounted())
                    ->get_pattern()
                    .str()
             << std::endl;
      output << "flags: "
             << lynx::fml::static_ref_ptr_cast<lepus::RegExp>(val->RefCounted())
                    ->get_flags()
                    .str()
             << std::endl;
      break;
    case lepus::ValueType::Value_NaN:
      output << "NaN";
      break;
    default:
      output << "unknown type";
      break;
  }
}

lepus::Value Print(lepus::MTSContext* context, lepus::Value* argv, int argc) {
  long params_count = argc;
  for (long i = 0; i < params_count; i++) {
    lepus::Value v(static_cast<lepus::VMContext*>(context)->GetParam(i));
    std::ostringstream s;
    PrintValue(&v, s);
    LOGE(s.str());
  }
  return lepus::Value();
}

void IRTestBase::RegisterBuiltin(lepus::VMContext* context) {
  RegisterCFunction(context, kCFuncPrint, &Print);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
