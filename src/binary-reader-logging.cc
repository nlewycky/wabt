/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/binary-reader-logging.h"

#include <cinttypes>

#include "src/stream.h"

namespace wabt {

#define INDENT_SIZE 2

#define LOGF_NOINDENT(...) stream_->Writef(__VA_ARGS__)

#define LOGF(...)               \
  do {                          \
    WriteIndent();              \
    LOGF_NOINDENT(__VA_ARGS__); \
  } while (0)

namespace {

void SPrintLimits(char* dst, size_t size, const Limits* limits) {
  int result;
  if (limits->has_max) {
    result = wabt_snprintf(dst, size, "initial: %" PRIu64 ", max: %" PRIu64,
                           limits->initial, limits->max);
  } else {
    result = wabt_snprintf(dst, size, "initial: %" PRIu64, limits->initial);
  }
  WABT_USE(result);
  assert(static_cast<size_t>(result) < size);
}

}  // end anonymous namespace

BinaryReaderLogging::BinaryReaderLogging(Stream* stream,
                                         BinaryReaderDelegate* forward)
    : stream_(stream), reader_(forward), indent_(0) {}

void BinaryReaderLogging::Indent() {
  indent_ += INDENT_SIZE;
}

void BinaryReaderLogging::Dedent() {
  indent_ -= INDENT_SIZE;
  assert(indent_ >= 0);
}

void BinaryReaderLogging::WriteIndent() {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static const size_t s_indent_len = sizeof(s_indent) - 1;
  size_t i = indent_;
  while (i > s_indent_len) {
    stream_->WriteData(s_indent, s_indent_len);
    i -= s_indent_len;
  }
  if (i > 0) {
    stream_->WriteData(s_indent, indent_);
  }
}

void BinaryReaderLogging::LogType(Type type) {
  if (IsTypeIndex(type)) {
    LOGF_NOINDENT("funcidx[%d]", static_cast<int>(type));
  } else {
    LOGF_NOINDENT("%s", GetTypeName(type));
  }
}

void BinaryReaderLogging::LogTypes(Index type_count, Type* types) {
  LOGF_NOINDENT("[");
  for (Index i = 0; i < type_count; ++i) {
    LogType(types[i]);
    if (i != type_count - 1) {
      LOGF_NOINDENT(", ");
    }
  }
  LOGF_NOINDENT("]");
}

void BinaryReaderLogging::LogTypes(TypeVector& types) {
  LogTypes(types.size(), types.data());
}

bool BinaryReaderLogging::OnError(const Error& error) {
  return reader_->OnError(error);
}

void BinaryReaderLogging::OnSetState(const State* s) {
  BinaryReaderDelegate::OnSetState(s);
  reader_->OnSetState(s);
}

Result BinaryReaderLogging::BeginModule(uint32_t version) {
  LOGF("BeginModule(version: %u)\n", version);
  Indent();
  return reader_->BeginModule(version);
}

Result BinaryReaderLogging::BeginSection(Index section_index,
                                         BinarySection section_type,
                                         Offset size) {
  return reader_->BeginSection(section_index, section_type, size);
}

Result BinaryReaderLogging::BeginCustomSection(Offset size,
                                               string_view section_name) {
  LOGF("BeginCustomSection('" PRIstringview "', size: %" PRIzd ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(section_name), size);
  Indent();
  return reader_->BeginCustomSection(size, section_name);
}

Result BinaryReaderLogging::OnType(Index index,
                                   Index param_count,
                                   Type* param_types,
                                   Index result_count,
                                   Type* result_types) {
  LOGF("OnType(index: %" PRIindex ", params: ", index);
  LogTypes(param_count, param_types);
  LOGF_NOINDENT(", results: ");
  LogTypes(result_count, result_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnType(index, param_count, param_types, result_count,
                         result_types);
}

Result BinaryReaderLogging::OnImport(Index index,
                                     string_view module_name,
                                     string_view field_name) {
  LOGF("OnImport(index: %" PRIindex ", module: \"" PRIstringview
       "\", field: \"" PRIstringview "\")\n",
       index, WABT_PRINTF_STRING_VIEW_ARG(module_name),
       WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return reader_->OnImport(index, module_name, field_name);
}

Result BinaryReaderLogging::OnImportFunc(Index import_index,
                                         string_view module_name,
                                         string_view field_name,
                                         Index func_index,
                                         Index sig_index) {
  LOGF("OnImportFunc(import_index: %" PRIindex ", func_index: %" PRIindex
       ", sig_index: %" PRIindex ")\n",
       import_index, func_index, sig_index);
  return reader_->OnImportFunc(import_index, module_name, field_name,
                               func_index, sig_index);
}

Result BinaryReaderLogging::OnImportTable(Index import_index,
                                          string_view module_name,
                                          string_view field_name,
                                          Index table_index,
                                          Type elem_type,
                                          const Limits* elem_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), elem_limits);
  LOGF("OnImportTable(import_index: %" PRIindex ", table_index: %" PRIindex
       ", elem_type: %s, %s)\n",
       import_index, table_index, GetTypeName(elem_type), buf);
  return reader_->OnImportTable(import_index, module_name, field_name,
                                table_index, elem_type, elem_limits);
}

Result BinaryReaderLogging::OnImportMemory(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index memory_index,
                                           const Limits* page_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), page_limits);
  LOGF("OnImportMemory(import_index: %" PRIindex ", memory_index: %" PRIindex
       ", %s)\n",
       import_index, memory_index, buf);
  return reader_->OnImportMemory(import_index, module_name, field_name,
                                 memory_index, page_limits);
}

Result BinaryReaderLogging::OnImportGlobal(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index global_index,
                                           Type type,
                                           bool mutable_) {
  LOGF("OnImportGlobal(import_index: %" PRIindex ", global_index: %" PRIindex
       ", type: %s, mutable: "
       "%s)\n",
       import_index, global_index, GetTypeName(type),
       mutable_ ? "true" : "false");
  return reader_->OnImportGlobal(import_index, module_name, field_name,
                                 global_index, type, mutable_);
}

Result BinaryReaderLogging::OnImportEvent(Index import_index,
                                          string_view module_name,
                                          string_view field_name,
                                          Index event_index,
                                          Index sig_index) {
  LOGF("OnImportEvent(import_index: %" PRIindex ", event_index: %" PRIindex
       ", sig_index: %" PRIindex ")\n",
       import_index, event_index, sig_index);
  return reader_->OnImportEvent(import_index, module_name, field_name,
                                event_index, sig_index);
}

Result BinaryReaderLogging::OnTable(Index index,
                                    Type elem_type,
                                    const Limits* elem_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), elem_limits);
  LOGF("OnTable(index: %" PRIindex ", elem_type: %s, %s)\n", index,
       GetTypeName(elem_type), buf);
  return reader_->OnTable(index, elem_type, elem_limits);
}

Result BinaryReaderLogging::OnMemory(Index index, const Limits* page_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), page_limits);
  LOGF("OnMemory(index: %" PRIindex ", %s)\n", index, buf);
  return reader_->OnMemory(index, page_limits);
}

Result BinaryReaderLogging::BeginGlobal(Index index, Type type, bool mutable_) {
  LOGF("BeginGlobal(index: %" PRIindex ", type: %s, mutable: %s)\n", index,
       GetTypeName(type), mutable_ ? "true" : "false");
  return reader_->BeginGlobal(index, type, mutable_);
}

Result BinaryReaderLogging::OnExport(Index index,
                                     ExternalKind kind,
                                     Index item_index,
                                     string_view name) {
  LOGF("OnExport(index: %" PRIindex ", kind: %s, item_index: %" PRIindex
       ", name: \"" PRIstringview "\")\n",
       index, GetKindName(kind), item_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnExport(index, kind, item_index, name);
}

Result BinaryReaderLogging::BeginFunctionBody(Index value, Offset size) {
  LOGF("BeginFunctionBody(%" PRIindex ", size:%" PRIzd ")\n", value, size);
  return reader_->BeginFunctionBody(value, size);
}

Result BinaryReaderLogging::OnLocalDecl(Index decl_index,
                                        Index count,
                                        Type type) {
  LOGF("OnLocalDecl(index: %" PRIindex ", count: %" PRIindex ", type: %s)\n",
       decl_index, count, GetTypeName(type));
  return reader_->OnLocalDecl(decl_index, count, type);
}

Result BinaryReaderLogging::OnBlockExpr(Type sig_type) {
  LOGF("OnBlockExpr(sig: ");
  LogType(sig_type);
  LOGF_NOINDENT(")\n");
  return reader_->OnBlockExpr(sig_type);
}

Result BinaryReaderLogging::OnBrExpr(Index depth) {
  LOGF("OnBrExpr(depth: %" PRIindex ")\n", depth);
  return reader_->OnBrExpr(depth);
}

Result BinaryReaderLogging::OnBrIfExpr(Index depth) {
  LOGF("OnBrIfExpr(depth: %" PRIindex ")\n", depth);
  return reader_->OnBrIfExpr(depth);
}

Result BinaryReaderLogging::OnBrTableExpr(Index num_targets,
                                          Index* target_depths,
                                          Index default_target_depth) {
  LOGF("OnBrTableExpr(num_targets: %" PRIindex ", depths: [", num_targets);
  for (Index i = 0; i < num_targets; ++i) {
    LOGF_NOINDENT("%" PRIindex, target_depths[i]);
    if (i != num_targets - 1) {
      LOGF_NOINDENT(", ");
    }
  }
  LOGF_NOINDENT("], default: %" PRIindex ")\n", default_target_depth);
  return reader_->OnBrTableExpr(num_targets, target_depths,
                                default_target_depth);
}

Result BinaryReaderLogging::OnF32ConstExpr(uint32_t value_bits) {
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnF32ConstExpr(%g (0x04%x))\n", value, value_bits);
  return reader_->OnF32ConstExpr(value_bits);
}

Result BinaryReaderLogging::OnF64ConstExpr(uint64_t value_bits) {
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnF64ConstExpr(%g (0x08%" PRIx64 "))\n", value, value_bits);
  return reader_->OnF64ConstExpr(value_bits);
}

Result BinaryReaderLogging::OnV128ConstExpr(v128 value_bits) {
  LOGF("OnV128ConstExpr(0x%08x 0x%08x 0x%08x 0x%08x)\n", value_bits.v[0],
       value_bits.v[1], value_bits.v[2], value_bits.v[3]);
  return reader_->OnV128ConstExpr(value_bits);
}

Result BinaryReaderLogging::OnI32ConstExpr(uint32_t value) {
  LOGF("OnI32ConstExpr(%u (0x%x))\n", value, value);
  return reader_->OnI32ConstExpr(value);
}

Result BinaryReaderLogging::OnI64ConstExpr(uint64_t value) {
  LOGF("OnI64ConstExpr(%" PRIu64 " (0x%" PRIx64 "))\n", value, value);
  return reader_->OnI64ConstExpr(value);
}

Result BinaryReaderLogging::OnIfExpr(Type sig_type) {
  LOGF("OnIfExpr(sig: ");
  LogType(sig_type);
  LOGF_NOINDENT(")\n");
  return reader_->OnIfExpr(sig_type);
}

Result BinaryReaderLogging::OnLoopExpr(Type sig_type) {
  LOGF("OnLoopExpr(sig: ");
  LogType(sig_type);
  LOGF_NOINDENT(")\n");
  return reader_->OnLoopExpr(sig_type);
}

Result BinaryReaderLogging::OnTryExpr(Type sig_type) {
  LOGF("OnTryExpr(sig: ");
  LogType(sig_type);
  LOGF_NOINDENT(")\n");
  return reader_->OnTryExpr(sig_type);
}

Result BinaryReaderLogging::OnSimdLaneOpExpr(Opcode opcode, uint64_t value) {
  LOGF("OnSimdLaneOpExpr (lane: %" PRIu64 ")\n", value);
  return reader_->OnSimdLaneOpExpr(opcode, value);
}

Result BinaryReaderLogging::OnSimdShuffleOpExpr(Opcode opcode, v128 value) {
  LOGF("OnSimdShuffleOpExpr (lane: 0x%08x %08x %08x %08x)\n", value.v[0],
       value.v[1], value.v[2], value.v[3]);
  return reader_->OnSimdShuffleOpExpr(opcode, value);
}

Result BinaryReaderLogging::BeginElemSegment(Index index,
                                             Index table_index,
                                             bool passive,
                                             Type elem_type) {
  LOGF("BeginElemSegment(index: %" PRIindex ", table_index: %" PRIindex
       ", passive: %s, elem_type: %s)\n",
       index, table_index, passive ? "true" : "false", GetTypeName(elem_type));
  return reader_->BeginElemSegment(index, table_index, passive, elem_type);
}

Result BinaryReaderLogging::OnDataSegmentData(Index index,
                                              const void* data,
                                              Address size) {
  LOGF("OnDataSegmentData(index:%" PRIindex ", size:%" PRIaddress ")\n", index,
       size);
  return reader_->OnDataSegmentData(index, data, size);
}

Result BinaryReaderLogging::OnModuleNameSubsection(Index index,
                                                   uint32_t name_type,
                                                   Offset subsection_size) {
  LOGF("OnModuleNameSubsection(index:%" PRIindex ", nametype:%u, size:%" PRIzd
       ")\n",
       index, name_type, subsection_size);
  return reader_->OnModuleNameSubsection(index, name_type, subsection_size);
}

Result BinaryReaderLogging::OnModuleName(string_view name) {
  LOGF("OnModuleName(name: \"" PRIstringview "\")\n",
       WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnModuleName(name);
}

Result BinaryReaderLogging::OnFunctionNameSubsection(Index index,
                                                     uint32_t name_type,
                                                     Offset subsection_size) {
  LOGF("OnFunctionNameSubsection(index:%" PRIindex ", nametype:%u, size:%" PRIzd
       ")\n",
       index, name_type, subsection_size);
  return reader_->OnFunctionNameSubsection(index, name_type, subsection_size);
}

Result BinaryReaderLogging::OnFunctionName(Index index, string_view name) {
  LOGF("OnFunctionName(index: %" PRIindex ", name: \"" PRIstringview "\")\n",
       index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnFunctionName(index, name);
}

Result BinaryReaderLogging::OnLocalNameSubsection(Index index,
                                                  uint32_t name_type,
                                                  Offset subsection_size) {
  LOGF("OnLocalNameSubsection(index:%" PRIindex ", nametype:%u, size:%" PRIzd
       ")\n",
       index, name_type, subsection_size);
  return reader_->OnLocalNameSubsection(index, name_type, subsection_size);
}

Result BinaryReaderLogging::OnLocalName(Index func_index,
                                        Index local_index,
                                        string_view name) {
  LOGF("OnLocalName(func_index: %" PRIindex ", local_index: %" PRIindex
       ", name: \"" PRIstringview "\")\n",
       func_index, local_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnLocalName(func_index, local_index, name);
}

Result BinaryReaderLogging::OnInitExprF32ConstExpr(Index index,
                                                   uint32_t value_bits) {
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnInitExprF32ConstExpr(index: %" PRIindex ", value: %g (0x04%x))\n",
       index, value, value_bits);
  return reader_->OnInitExprF32ConstExpr(index, value_bits);
}

Result BinaryReaderLogging::OnInitExprF64ConstExpr(Index index,
                                                   uint64_t value_bits) {
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnInitExprF64ConstExpr(index: %" PRIindex " value: %g (0x08%" PRIx64
       "))\n",
       index, value, value_bits);
  return reader_->OnInitExprF64ConstExpr(index, value_bits);
}

Result BinaryReaderLogging::OnInitExprV128ConstExpr(Index index,
                                                    v128 value_bits) {
  LOGF("OnInitExprV128ConstExpr(index: %" PRIindex
       " value: ( 0x%08x 0x%08x 0x%08x 0x%08x))\n",
       index, value_bits.v[0], value_bits.v[1], value_bits.v[2],
       value_bits.v[3]);
  return reader_->OnInitExprV128ConstExpr(index, value_bits);
}

Result BinaryReaderLogging::OnInitExprI32ConstExpr(Index index,
                                                   uint32_t value) {
  LOGF("OnInitExprI32ConstExpr(index: %" PRIindex ", value: %u)\n", index,
       value);
  return reader_->OnInitExprI32ConstExpr(index, value);
}

Result BinaryReaderLogging::OnInitExprI64ConstExpr(Index index,
                                                   uint64_t value) {
  LOGF("OnInitExprI64ConstExpr(index: %" PRIindex ", value: %" PRIu64 ")\n",
       index, value);
  return reader_->OnInitExprI64ConstExpr(index, value);
}

Result BinaryReaderLogging::OnDylinkInfo(uint32_t mem_size,
                                         uint32_t mem_align,
                                         uint32_t table_size,
                                         uint32_t table_align) {
  LOGF(
      "OnDylinkInfo(mem_size: %u, mem_align: %u, table_size: %u, table_align: "
      "%u)\n",
      mem_size, mem_align, table_size, table_align);
  return reader_->OnDylinkInfo(mem_size, mem_align, table_size, table_align);
}

Result BinaryReaderLogging::OnDylinkNeeded(string_view so_name) {
  LOGF("OnDylinkNeeded(name: " PRIstringview ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(so_name));
  return reader_->OnDylinkNeeded(so_name);
}

Result BinaryReaderLogging::OnRelocCount(Index count,
                                         Index section_index) {
  LOGF("OnRelocCount(count: %" PRIindex ", section: %" PRIindex ")\n", count,
       section_index);
  return reader_->OnRelocCount(count, section_index);
}

Result BinaryReaderLogging::OnReloc(RelocType type,
                                    Offset offset,
                                    Index index,
                                    uint32_t addend) {
  int32_t signed_addend = static_cast<int32_t>(addend);
  LOGF("OnReloc(type: %s, offset: %" PRIzd ", index: %" PRIindex
       ", addend: %d)\n",
       GetRelocTypeName(type), offset, index, signed_addend);
  return reader_->OnReloc(type, offset, index, addend);
}

Result BinaryReaderLogging::OnSymbol(Index symbol_index,
                                     SymbolType type,
                                     uint32_t flags) {
  LOGF("OnSymbol(type: %s flags: 0x%x)\n", GetSymbolTypeName(type), flags);
  return reader_->OnSymbol(symbol_index, type, flags);
}

Result BinaryReaderLogging::OnDataSymbol(Index index,
                                         uint32_t flags,
                                         string_view name,
                                         Index segment,
                                         uint32_t offset,
                                         uint32_t size) {
  LOGF("OnDataSymbol(name: " PRIstringview " flags: 0x%x)\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags);
  return reader_->OnDataSymbol(index, flags, name, segment, offset, size);
}

Result BinaryReaderLogging::OnFunctionSymbol(Index index,
                                             uint32_t flags,
                                             string_view name,
                                             Index func_index) {
  LOGF("OnFunctionSymbol(name: " PRIstringview " flags: 0x%x index: %" PRIindex
       ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags, func_index);
  return reader_->OnFunctionSymbol(index, flags, name, func_index);
}

Result BinaryReaderLogging::OnGlobalSymbol(Index index,
                                           uint32_t flags,
                                           string_view name,
                                           Index global_index) {
  LOGF("OnGlobalSymbol(name: " PRIstringview " flags: 0x%x index: %" PRIindex
       ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags, global_index);
  return reader_->OnGlobalSymbol(index, flags, name, global_index);
}

Result BinaryReaderLogging::OnSectionSymbol(Index index,
                                            uint32_t flags,
                                            Index section_index) {
  LOGF("OnSectionSymbol(flags: 0x%x index: %" PRIindex ")\n", flags,
       section_index);
  return reader_->OnSectionSymbol(index, flags, section_index);
}

Result BinaryReaderLogging::OnEventSymbol(Index index,
                                          uint32_t flags,
                                          string_view name,
                                          Index event_index) {
  LOGF("OnEventSymbol(name: " PRIstringview " flags: 0x%x index: %" PRIindex
           ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags, event_index);
  return reader_->OnEventSymbol(index, flags, name, event_index);
}

Result BinaryReaderLogging::OnSegmentInfo(Index index,
                                          string_view name,
                                          uint32_t alignment,
                                          uint32_t flags) {
  LOGF("OnSegmentInfo(%d name: " PRIstringview
       ", alignment: %d, flags: 0x%x)\n",
       index, WABT_PRINTF_STRING_VIEW_ARG(name), alignment, flags);
  return reader_->OnSegmentInfo(index, name, alignment, flags);
}

Result BinaryReaderLogging::OnInitFunction(uint32_t priority,
                                           Index func_index) {
  LOGF("OnInitFunction(%d priority: %d)\n", func_index, priority);
  return reader_->OnInitFunction(priority, func_index);
}

Result BinaryReaderLogging::OnComdatBegin(string_view name,
                                          uint32_t flags,
                                          Index count) {
  LOGF("OnComdatBegin(" PRIstringview ", flags: %d, count: %" PRIindex ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags, count);
  return reader_->OnComdatBegin(name, flags, count);
}

Result BinaryReaderLogging::OnComdatEntry(ComdatType kind, Index index) {
  LOGF("OnComdatEntry(kind: %d, index: %" PRIindex ")\n", kind, index);
  return reader_->OnComdatEntry(kind, index);
}

#define DEFINE_BEGIN(name)                        \
  Result BinaryReaderLogging::name(Offset size) { \
    LOGF(#name "(%" PRIzd ")\n", size);           \
    Indent();                                     \
    return reader_->name(size);                   \
  }

#define DEFINE_END(name)               \
  Result BinaryReaderLogging::name() { \
    Dedent();                          \
    LOGF(#name "\n");                  \
    return reader_->name();            \
  }

#define DEFINE_INDEX(name)                        \
  Result BinaryReaderLogging::name(Index value) { \
    LOGF(#name "(%" PRIindex ")\n", value);       \
    return reader_->name(value);                  \
  }

#define DEFINE_INDEX_DESC(name, desc)                 \
  Result BinaryReaderLogging::name(Index value) {     \
    LOGF(#name "(" desc ": %" PRIindex ")\n", value); \
    return reader_->name(value);                      \
  }

#define DEFINE_INDEX_INDEX(name, desc0, desc1)                           \
  Result BinaryReaderLogging::name(Index value0, Index value1) {         \
    LOGF(#name "(" desc0 ": %" PRIindex ", " desc1 ": %" PRIindex ")\n", \
         value0, value1);                                                \
    return reader_->name(value0, value1);                                \
  }

#define DEFINE_INDEX_INDEX_BOOL(name, desc0, desc1, desc2)                     \
  Result BinaryReaderLogging::name(Index value0, Index value1, bool value2) {  \
    LOGF(#name "(" desc0 ": %" PRIindex ", " desc1 ": %" PRIindex              \
               ", " desc2 ": %s)\n",                                           \
         value0, value1, value2 ? "true" : "false");                           \
    return reader_->name(value0, value1, value2);                              \
  }

#define DEFINE_OPCODE(name)                                            \
  Result BinaryReaderLogging::name(Opcode opcode) {                    \
    LOGF(#name "(\"%s\" (%u))\n", opcode.GetName(), opcode.GetCode()); \
    return reader_->name(opcode);                                      \
  }

#define DEFINE_LOAD_STORE_OPCODE(name)                                      \
  Result BinaryReaderLogging::name(Opcode opcode, uint32_t alignment_log2,  \
                                   Address offset) {                        \
    LOGF(#name "(opcode: \"%s\" (%u), align log2: %u, offset: %" PRIaddress \
               ")\n",                                                       \
         opcode.GetName(), opcode.GetCode(), alignment_log2, offset);       \
    return reader_->name(opcode, alignment_log2, offset);                   \
  }

#define DEFINE0(name)                  \
  Result BinaryReaderLogging::name() { \
    LOGF(#name "\n");                  \
    return reader_->name();            \
  }

DEFINE_END(EndModule)

DEFINE_END(EndCustomSection)

DEFINE_BEGIN(BeginTypeSection)
DEFINE_INDEX(OnTypeCount)
DEFINE_END(EndTypeSection)

DEFINE_BEGIN(BeginImportSection)
DEFINE_INDEX(OnImportCount)
DEFINE_END(EndImportSection)

DEFINE_BEGIN(BeginFunctionSection)
DEFINE_INDEX(OnFunctionCount)
DEFINE_INDEX_INDEX(OnFunction, "index", "sig_index")
DEFINE_END(EndFunctionSection)

DEFINE_BEGIN(BeginTableSection)
DEFINE_INDEX(OnTableCount)
DEFINE_END(EndTableSection)

DEFINE_BEGIN(BeginMemorySection)
DEFINE_INDEX(OnMemoryCount)
DEFINE_END(EndMemorySection)

DEFINE_BEGIN(BeginGlobalSection)
DEFINE_INDEX(OnGlobalCount)
DEFINE_INDEX(BeginGlobalInitExpr)
DEFINE_INDEX(EndGlobalInitExpr)
DEFINE_INDEX(EndGlobal)
DEFINE_END(EndGlobalSection)

DEFINE_BEGIN(BeginExportSection)
DEFINE_INDEX(OnExportCount)
DEFINE_END(EndExportSection)

DEFINE_BEGIN(BeginStartSection)
DEFINE_INDEX(OnStartFunction)
DEFINE_END(EndStartSection)

DEFINE_BEGIN(BeginCodeSection)
DEFINE_INDEX(OnFunctionBodyCount)
DEFINE_INDEX(EndFunctionBody)
DEFINE_INDEX(OnLocalDeclCount)
DEFINE_LOAD_STORE_OPCODE(OnAtomicLoadExpr);
DEFINE_LOAD_STORE_OPCODE(OnAtomicRmwExpr);
DEFINE_LOAD_STORE_OPCODE(OnAtomicRmwCmpxchgExpr);
DEFINE_LOAD_STORE_OPCODE(OnAtomicStoreExpr);
DEFINE_LOAD_STORE_OPCODE(OnAtomicWaitExpr);
DEFINE_LOAD_STORE_OPCODE(OnAtomicNotifyExpr);
DEFINE_INDEX_INDEX(OnBrOnExnExpr, "depth", "event_index");
DEFINE_OPCODE(OnBinaryExpr)
DEFINE_INDEX_DESC(OnCallExpr, "func_index")
DEFINE_INDEX_INDEX(OnCallIndirectExpr, "sig_index", "table_index")
DEFINE0(OnCatchExpr);
DEFINE_OPCODE(OnCompareExpr)
DEFINE_OPCODE(OnConvertExpr)
DEFINE0(OnDropExpr)
DEFINE0(OnElseExpr)
DEFINE0(OnEndExpr)
DEFINE_INDEX_DESC(OnGlobalGetExpr, "index")
DEFINE_INDEX_DESC(OnGlobalSetExpr, "index")
DEFINE_LOAD_STORE_OPCODE(OnLoadExpr);
DEFINE_INDEX_DESC(OnLocalGetExpr, "index")
DEFINE_INDEX_DESC(OnLocalSetExpr, "index")
DEFINE_INDEX_DESC(OnLocalTeeExpr, "index")
DEFINE0(OnMemoryCopyExpr)
DEFINE_INDEX(OnDataDropExpr)
DEFINE0(OnMemoryFillExpr)
DEFINE0(OnMemoryGrowExpr)
DEFINE_INDEX(OnMemoryInitExpr)
DEFINE0(OnMemorySizeExpr)
DEFINE0(OnTableCopyExpr)
DEFINE_INDEX(OnElemDropExpr)
DEFINE_INDEX(OnTableInitExpr)
DEFINE_INDEX(OnTableSetExpr)
DEFINE_INDEX(OnTableGetExpr)
DEFINE_INDEX(OnTableGrowExpr)
DEFINE_INDEX(OnTableSizeExpr)
DEFINE0(OnRefNullExpr)
DEFINE0(OnRefIsNullExpr)
DEFINE0(OnNopExpr)
DEFINE0(OnRethrowExpr);
DEFINE_INDEX_DESC(OnReturnCallExpr, "func_index")

DEFINE_INDEX_INDEX(OnReturnCallIndirectExpr, "sig_index", "table_index")
DEFINE0(OnReturnExpr)
DEFINE0(OnSelectExpr)
DEFINE_LOAD_STORE_OPCODE(OnStoreExpr);
DEFINE_INDEX_DESC(OnThrowExpr, "event_index")
DEFINE0(OnUnreachableExpr)
DEFINE_OPCODE(OnUnaryExpr)
DEFINE_OPCODE(OnTernaryExpr)
DEFINE_END(EndCodeSection)

DEFINE_BEGIN(BeginElemSection)
DEFINE_INDEX(OnElemSegmentCount)
DEFINE_INDEX(BeginElemSegmentInitExpr)
DEFINE_INDEX(EndElemSegmentInitExpr)
DEFINE_INDEX_INDEX(OnElemSegmentElemExprCount, "index", "count")
DEFINE_INDEX(OnElemSegmentElemExpr_RefNull)
DEFINE_INDEX_INDEX(OnElemSegmentElemExpr_RefFunc, "index", "func_index")
DEFINE_INDEX(EndElemSegment)
DEFINE_END(EndElemSection)

DEFINE_BEGIN(BeginDataSection)
DEFINE_INDEX(OnDataSegmentCount)
DEFINE_INDEX_INDEX_BOOL(BeginDataSegment, "index", "memory_index", "passive")
DEFINE_INDEX(BeginDataSegmentInitExpr)
DEFINE_INDEX(EndDataSegmentInitExpr)
DEFINE_INDEX(EndDataSegment)
DEFINE_END(EndDataSection)

DEFINE_BEGIN(BeginDataCountSection)
DEFINE_INDEX(OnDataCount)
DEFINE_END(EndDataCountSection)

DEFINE_BEGIN(BeginNamesSection)
DEFINE_INDEX(OnFunctionNamesCount)
DEFINE_INDEX(OnLocalNameFunctionCount)
DEFINE_INDEX_INDEX(OnLocalNameLocalCount, "index", "count")
DEFINE_END(EndNamesSection)

DEFINE_BEGIN(BeginRelocSection)
DEFINE_END(EndRelocSection)
DEFINE_INDEX_INDEX(OnInitExprGlobalGetExpr, "index", "global_index")

DEFINE_BEGIN(BeginDylinkSection)
DEFINE_INDEX(OnDylinkNeededCount)
DEFINE_END(EndDylinkSection)

DEFINE_BEGIN(BeginLinkingSection)
DEFINE_INDEX(OnSymbolCount)
DEFINE_INDEX(OnSegmentInfoCount)
DEFINE_INDEX(OnInitFunctionCount)
DEFINE_INDEX(OnComdatCount)
DEFINE_END(EndLinkingSection)

DEFINE_BEGIN(BeginEventSection);
DEFINE_INDEX(OnEventCount);
DEFINE_INDEX_INDEX(OnEventType, "index", "sig_index")
DEFINE_END(EndEventSection);

// We don't need to log these (the individual opcodes are logged instead), but
// we still need to forward the calls.
Result BinaryReaderLogging::OnOpcode(Opcode opcode) {
  return reader_->OnOpcode(opcode);
}

Result BinaryReaderLogging::OnOpcodeBare() {
  return reader_->OnOpcodeBare();
}

Result BinaryReaderLogging::OnOpcodeIndex(Index value) {
  return reader_->OnOpcodeIndex(value);
}

Result BinaryReaderLogging::OnOpcodeIndexIndex(Index value, Index value2) {
  return reader_->OnOpcodeIndexIndex(value, value2);
}

Result BinaryReaderLogging::OnOpcodeUint32(uint32_t value) {
  return reader_->OnOpcodeUint32(value);
}

Result BinaryReaderLogging::OnOpcodeUint32Uint32(uint32_t value,
                                                 uint32_t value2) {
  return reader_->OnOpcodeUint32Uint32(value, value2);
}

Result BinaryReaderLogging::OnOpcodeUint64(uint64_t value) {
  return reader_->OnOpcodeUint64(value);
}

Result BinaryReaderLogging::OnOpcodeF32(uint32_t value) {
  return reader_->OnOpcodeF32(value);
}

Result BinaryReaderLogging::OnOpcodeF64(uint64_t value) {
  return reader_->OnOpcodeF64(value);
}

Result BinaryReaderLogging::OnOpcodeV128(v128 value) {
  return reader_->OnOpcodeV128(value);
}

Result BinaryReaderLogging::OnOpcodeBlockSig(Type sig_type) {
  return reader_->OnOpcodeBlockSig(sig_type);
}

Result BinaryReaderLogging::OnEndFunc() {
  return reader_->OnEndFunc();
}

}  // namespace wabt
