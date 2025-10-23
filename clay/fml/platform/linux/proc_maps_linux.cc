// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/linux/proc_maps_linux.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/include/string/string_utils.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"

#if defined(OS_LINUX) || defined(OS_ANDROID)
#include <inttypes.h>
#endif

namespace fml {

// Scans |proc_maps| starting from |pos| returning true if the gate VMA was
// found, otherwise returns false.
static bool ContainsGateVMA(std::string* proc_maps, size_t pos) {
#if defined(ARCH_CPU_ARM_FAMILY)
  // The gate VMA on ARM kernels is the interrupt vectors page.
  return proc_maps->find(" [vectors]\n", pos) != std::string::npos;
#elif defined(ARCH_CPU_X86_64)
  // The gate VMA on x86 64-bit kernels is the virtual system call page.
  return proc_maps->find(" [vsyscall]\n", pos) != std::string::npos;
#else
  // Otherwise assume there is no gate VMA in which case we shouldn't
  // get duplicate entires.
  return false;
#endif
}

bool ReadProcMaps(std::string* proc_maps) {
  // seq_file only writes out a page-sized amount on each call. Refer to header
  // file for details.
  const long kReadSize = sysconf(_SC_PAGESIZE);  // NOLINT

  // base::ScopedFD fd(HANDLE_EINTR(open("/proc/self/maps", O_RDONLY)));
  int fd = open("/proc/self/maps", O_RDONLY);
  if (fd == -1) {
    FML_DLOG(ERROR) << "Couldn't open /proc/self/maps";
    return false;
  }
  // if (!fd.is_valid()) {
  //   FML_DLOG(ERROR) << "Couldn't open /proc/self/maps";
  //   return false;
  // }
  proc_maps->clear();

  while (true) {
    // To avoid a copy, resize |proc_maps| so read() can write directly into it.
    // Compute |buffer| afterwards since resize() may reallocate.
    size_t pos = proc_maps->size();
    proc_maps->resize(pos + kReadSize);
    void* buffer = &(*proc_maps)[pos];

    // ssize_t bytes_read = HANDLE_EINTR(read(fd, buffer, kReadSize));
    ssize_t bytes_read = read(fd, buffer, kReadSize);
    if (bytes_read < 0) {
      FML_DLOG(ERROR) << "Couldn't read /proc/self/maps";
      proc_maps->clear();
      return false;
    }

    // ... and don't forget to trim off excess bytes.
    proc_maps->resize(pos + bytes_read);

    if (bytes_read == 0) break;

    // The gate VMA is handled as a special case after seq_file has finished
    // iterating through all entries in the virtual memory table.
    //
    // Unfortunately, if additional entries are added at this point in time
    // seq_file gets confused and the next call to read() will return duplicate
    // entries including the gate VMA again.
    //
    // Avoid this by searching for the gate VMA and breaking early.
    if (ContainsGateVMA(proc_maps, pos)) break;
  }

  // Close FD.
  close(fd);
  return true;
}

bool ParseProcMaps(const std::string& input,
                   std::vector<MappedMemoryRegion>* regions_out) {
  FML_CHECK(regions_out);
  std::vector<MappedMemoryRegion> regions;

  // This isn't async safe nor terribly efficient, but it doesn't need to be at
  // this point in time.
  std::vector<std::string> lines =
      lynx::base::SplitString<std::string, std::string>(input, "\n", true);

  lines.push_back("");
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string tmp = lynx::base::TrimString(lines[i]);
    // Due to splitting on '\n' the last line should be empty.
    if (i == lines.size() - 1) {
      if (!tmp.empty()) {
        FML_DLOG(ERROR) << "Last line not empty";
        return false;
      }
      break;
    }

    MappedMemoryRegion region;
    const char* line = tmp.c_str();
    char permissions[5] = {'\0'};  // Ensure NUL-terminated string.
    uint8_t dev_major = 0;
    uint8_t dev_minor = 0;
    long inode = 0;  // NOLINT
    int path_index = 0;

    // Sample format from man 5 proc:
    //
    // address           perms offset  dev   inode   pathname
    // 08048000-08056000 r-xp 00000000 03:0c 64593   /usr/sbin/gpm
    //
    // The final %n term captures the offset in the input string, which is used
    // to determine the path name. It *does not* increment the return value.
    // Refer to man 3 sscanf for details.
    if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4c %llx %hhx:%hhx %ld %n",
               &region.start, &region.end, permissions, &region.offset,
               &dev_major, &dev_minor, &inode, &path_index) < 7) {
      FML_DLOG(ERROR) << "sscanf failed for line: " << line;
      return false;
    }

    region.permissions = 0;

    if (permissions[0] == 'r')
      region.permissions |= MappedMemoryRegion::READ;
    else if (permissions[0] != '-')
      return false;

    if (permissions[1] == 'w')
      region.permissions |= MappedMemoryRegion::WRITE;
    else if (permissions[1] != '-')
      return false;

    if (permissions[2] == 'x')
      region.permissions |= MappedMemoryRegion::EXECUTE;
    else if (permissions[2] != '-')
      return false;

    if (permissions[3] == 'p')
      region.permissions |= MappedMemoryRegion::PRIVATE;
    else if (permissions[3] != 's' && permissions[3] != 'S')  // Shared memory.
      return false;

    // Pushing then assigning saves us a string copy.
    regions.push_back(region);
    regions.back().path.assign(line + path_index);
  }

  regions_out->swap(regions);
  return true;
}

}  // namespace fml
