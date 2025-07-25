//===-- ProtocolUtils.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ProtocolUtils.h"
#include "LLDBUtils.h"

#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBFormat.h"
#include "lldb/API/SBMutex.h"
#include "lldb/API/SBStream.h"
#include "lldb/API/SBTarget.h"
#include "lldb/API/SBThread.h"
#include "lldb/Host/PosixApi.h" // Adds PATH_MAX for windows
#include <optional>

using namespace lldb_dap::protocol;
namespace lldb_dap {

static bool ShouldDisplayAssemblySource(
    lldb::SBAddress address,
    lldb::StopDisassemblyType stop_disassembly_display) {
  if (stop_disassembly_display == lldb::eStopDisassemblyTypeNever)
    return false;

  if (stop_disassembly_display == lldb::eStopDisassemblyTypeAlways)
    return true;

  // A line entry of 0 indicates the line is compiler generated i.e. no source
  // file is associated with the frame.
  auto line_entry = address.GetLineEntry();
  auto file_spec = line_entry.GetFileSpec();
  if (!file_spec.IsValid() || line_entry.GetLine() == 0 ||
      line_entry.GetLine() == LLDB_INVALID_LINE_NUMBER)
    return true;

  if (stop_disassembly_display == lldb::eStopDisassemblyTypeNoSource &&
      !file_spec.Exists()) {
    return true;
  }

  return false;
}

std::optional<protocol::Source> CreateSource(const lldb::SBFileSpec &file) {
  if (!file.IsValid())
    return std::nullopt;

  protocol::Source source;
  if (const char *name = file.GetFilename())
    source.name = name;
  char path[PATH_MAX] = "";
  if (file.GetPath(path, sizeof(path)) &&
      lldb::SBFileSpec::ResolvePath(path, path, PATH_MAX))
    source.path = path;
  return source;
}

bool IsAssemblySource(const protocol::Source &source) {
  // According to the specification, a source must have either `path` or
  // `sourceReference` specified. We use `path` for sources with known source
  // code, and `sourceReferences` when falling back to assembly.
  return source.sourceReference.value_or(LLDB_DAP_INVALID_SRC_REF) >
         LLDB_DAP_INVALID_SRC_REF;
}

bool DisplayAssemblySource(lldb::SBDebugger &debugger,
                           lldb::SBAddress address) {
  const lldb::StopDisassemblyType stop_disassembly_display =
      GetStopDisassemblyDisplay(debugger);
  return ShouldDisplayAssemblySource(address, stop_disassembly_display);
}

std::string GetLoadAddressString(const lldb::addr_t addr) {
  return "0x" + llvm::utohexstr(addr, false, 16);
}

protocol::Thread CreateThread(lldb::SBThread &thread, lldb::SBFormat &format) {
  std::string name;
  lldb::SBStream stream;
  if (format && thread.GetDescriptionWithFormat(format, stream).Success()) {
    name = stream.GetData();
  } else {
    llvm::StringRef thread_name(thread.GetName());
    llvm::StringRef queue_name(thread.GetQueueName());

    if (!thread_name.empty()) {
      name = thread_name.str();
    } else if (!queue_name.empty()) {
      auto kind = thread.GetQueue().GetKind();
      std::string queue_kind_label = "";
      if (kind == lldb::eQueueKindSerial)
        queue_kind_label = " (serial)";
      else if (kind == lldb::eQueueKindConcurrent)
        queue_kind_label = " (concurrent)";

      name = llvm::formatv("Thread {0} Queue: {1}{2}", thread.GetIndexID(),
                           queue_name, queue_kind_label)
                 .str();
    } else {
      name = llvm::formatv("Thread {0}", thread.GetIndexID()).str();
    }
  }
  return protocol::Thread{thread.GetThreadID(), name};
}

std::vector<protocol::Thread> GetThreads(lldb::SBProcess process,
                                         lldb::SBFormat &format) {
  lldb::SBMutex lock = process.GetTarget().GetAPIMutex();
  std::lock_guard<lldb::SBMutex> guard(lock);

  std::vector<protocol::Thread> threads;

  const uint32_t num_threads = process.GetNumThreads();
  threads.reserve(num_threads);
  for (uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
    lldb::SBThread thread = process.GetThreadAtIndex(thread_idx);
    threads.emplace_back(CreateThread(thread, format));
  }
  return threads;
}

protocol::ExceptionBreakpointsFilter
CreateExceptionBreakpointFilter(const ExceptionBreakpoint &bp) {
  protocol::ExceptionBreakpointsFilter filter;
  filter.filter = bp.GetFilter();
  filter.label = bp.GetLabel();
  filter.description = bp.GetLabel();
  filter.defaultState = ExceptionBreakpoint::kDefaultValue;
  filter.supportsCondition = true;
  return filter;
}

} // namespace lldb_dap
