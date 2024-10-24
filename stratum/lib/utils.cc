// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/lib/utils.h"

#include <cxxabi.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>  // IWYU pragma: keep
#include <regex>
#include <string>

#include "absl/strings/str_split.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "stratum/lib/macros.h"
#include "stratum/public/lib/error.h"

namespace stratum {

using ::google::protobuf::util::MessageDifferencer;

::util::Status WriteProtoToBinFile(const ::google::protobuf::Message& message,
                                   const std::string& filename) {
  std::string buffer;
  if (!message.SerializeToString(&buffer)) {
    return MAKE_ERROR(ERR_INVALID_PARAM)
           << "Failed to convert proto to bin string buffer: "
           << message.ShortDebugString();
  }
  RETURN_IF_ERROR(WriteStringToFile(buffer, filename));

  return ::util::OkStatus();
}

::util::Status ReadProtoFromBinFile(const std::string& filename,
                                    ::google::protobuf::Message* message) {
  std::string buffer;
  RETURN_IF_ERROR(ReadFileToString(filename, &buffer));
  if (!message->ParseFromString(buffer)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Failed to parse the binary content of "
                                    << filename << " to proto.";
  }

  return ::util::OkStatus();
}

::util::Status WriteProtoToTextFile(const ::google::protobuf::Message& message,
                                    const std::string& filename) {
  std::string text;
  RETURN_IF_ERROR(PrintProtoToString(message, &text));
  RETURN_IF_ERROR(WriteStringToFile(text, filename));

  return ::util::OkStatus();
}

::util::Status ReadProtoFromTextFile(const std::string& filename,
                                     ::google::protobuf::Message* message) {
  std::string text;
  RETURN_IF_ERROR(ReadFileToString(filename, &text));
  RETURN_IF_ERROR(ParseProtoFromString(text, message));

  return ::util::OkStatus();
}

::util::Status PrintProtoToString(const ::google::protobuf::Message& message,
                                  std::string* text) {
  if (!::google::protobuf::TextFormat::PrintToString(message, text)) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Failed to print proto to string: " << message.ShortDebugString();
  }

  return ::util::OkStatus();
}

::util::Status ParseProtoFromString(const std::string& text,
                                    ::google::protobuf::Message* message) {
  if (!::google::protobuf::TextFormat::ParseFromString(text, message)) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Failed to parse proto from the following string: " << text;
  }

  return ::util::OkStatus();
}

::util::Status WriteStringToFile(const std::string& buffer,
                                 const std::string& filename, bool append) {
  std::ofstream outfile;
  outfile.open(filename.c_str(), append
                                     ? std::ofstream::out | std::ofstream::app
                                     : std::ofstream::out);
  if (!outfile.is_open()) {
    return MAKE_ERROR(ERR_INTERNAL) << "Error when opening " << filename << ".";
  }
  outfile << buffer;
  outfile.close();

  return ::util::OkStatus();
}

::util::Status ReadFileToString(const std::string& filename,
                                std::string* buffer) {
  if (!PathExists(filename)) {
    return MAKE_ERROR(ERR_FILE_NOT_FOUND) << filename << " not found.";
  }
  if (IsDir(filename)) {
    return MAKE_ERROR(ERR_FILE_NOT_FOUND) << filename << " is a dir.";
  }

  std::ifstream infile;
  infile.open(filename.c_str());
  if (!infile.is_open()) {
    return MAKE_ERROR(ERR_INTERNAL) << "Error when opening " << filename << ".";
  }

  std::string contents((std::istreambuf_iterator<char>(infile)),
                       (std::istreambuf_iterator<char>()));
  buffer->append(contents);
  infile.close();

  return ::util::OkStatus();
}

std::string StringToHex(const std::string& str) {
  static const char* const characters = "0123456789ABCDEF";
  std::string hex_str;
  const size_t size = str.size();
  hex_str.reserve(2 * size);
  for (size_t i = 0; i < size; ++i) {
    const unsigned char c = str[i];
    hex_str.push_back(characters[c >> 4]);
    hex_str.push_back(characters[c & 0xF]);
  }
  return hex_str;
}

::util::Status RecursivelyCreateDir(const std::string& dir) {
  RET_CHECK(!dir.empty());
  std::vector<std::string> dirs = absl::StrSplit(dir, '/');
  std::string path_to_make = "/";
  for (auto& dir_name : dirs) {
    if (dir_name.empty()) {
      continue;
    }
    absl::StrAppend(&path_to_make, dir_name);

    if (PathExists(path_to_make)) {
      if (!IsDir(path_to_make)) {
        return MAKE_ERROR(ERR_INVALID_PARAM)
               << path_to_make << " is not a dir.";
      }
    } else {
      int ret = mkdir(path_to_make.c_str(), 0755);
      if (ret != 0) {
        return MAKE_ERROR(ERR_INTERNAL) << "Can not make dir " << path_to_make
                                        << ": " << strerror(errno);
      }
    }

    absl::StrAppend(&path_to_make, "/");
  }

  return ::util::OkStatus();
}

::util::Status RemoveFile(const std::string& path) {
  RET_CHECK(!path.empty());
  RET_CHECK(PathExists(path)) << path << " does not exist.";
  RET_CHECK(!IsDir(path)) << path << " is a dir.";
  int ret = remove(path.c_str());
  if (ret != 0) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Failed to remove '" << path << "'. Return value: " << ret << ".";
  }

  return ::util::OkStatus();
}

// FIXME these are redefinitions of inline methods in the .h file
/* START GOOGLE ONLY
bool PathExists(const std::string& path) {
  struct stat stbuf;
  return (stat(path.c_str(), &stbuf) >= 0);
}

bool IsDir(const std::string& path) {
  struct stat stbuf;
  if (stat(path.c_str(), &stbuf) < 0) {
    return false;
  }
  return S_ISDIR(stbuf.st_mode);
}

std::string DirName(const std::string& path) {
  char* path_str = strdup(path.c_str());
  std::string dir = dirname(path_str);
  free(path_str);
  return dir;
}

std::string BaseName(const std::string& path) {
  char* path_str = strdup(path.c_str());
  std::string base = basename(path_str);
  free(path_str);
  return base;
}
   END GOOGLE ONLY */

// TODO(unknown): At the moment this function will not work well for
// complex messages with repeated fields or maps. Find a better way.
static bool ProtoLess(const google::protobuf::Message& m1,
                      const google::protobuf::Message& m2) {
  return m1.SerializeAsString() < m2.SerializeAsString();
}

// FIXME this are redefinitions of inline methods in the .h file
/* START GOOGLE ONLY
bool ProtoEqual(const google::protobuf::Message& m1, const
google::protobuf::Message& m2) { MessageDifferencer differencer;
  differencer.set_repeated_field_comparison(MessageDifferencer::AS_SET);
  return differencer.Compare(m1, m2);
}
   END GOOGLE ONLY */

// TODO(unknown): At the moment this function will not work well for
// complex messages with repeated fields or maps. Find a better way.
static size_t ProtoHash(const google::protobuf::Message& m) {
  std::hash<std::string> string_hasher;
  std::string s;
  m.SerializeToString(&s);
  return string_hasher(s);
}

std::string Demangle(const char* mangled) {
  int status;
  char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
  if (demangled) {
    std::string d(demangled);
    free(demangled);
    return d;
  } else {
    return mangled;
  }
}

::util::Status CreatePipeForSignalHandling(int* read_fd, int* write_fd) {
  static_assert(sizeof(int) <= PIPE_BUF,
                "PIPE_BUF is smaller than the number of bytes that can be "
                "written atomically to a pipe.");
  int pipe_fds[2];  // [0] is read side, [1] is write side.
  RET_CHECK(pipe(pipe_fds) == 0)
      << "Could not create pipe: " << strerror(errno) << ".";
  // Set write side to non-blocking mode.
  int flags = fcntl(pipe_fds[1], F_GETFL);
  RET_CHECK(flags != -1) << "Could not read file descriptor flags: "
                         << strerror(errno) << ".";
  RET_CHECK(fcntl(pipe_fds[1], F_SETFL, flags | O_NONBLOCK) != -1)
      << "Could not set file descriptor flags: " << strerror(errno) << ".";
  *read_fd = pipe_fds[0];
  *write_fd = pipe_fds[1];

  return ::util::OkStatus();
}

::util::Status ValidateYangHexString(const std::string& input) {
  const std::regex pattern("^([0-9a-fA-F]{2}(:[0-9a-fA-F]{2})*)$");
  if (regex_match(input, pattern)) {
    return ::util::OkStatus();
  } else {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Format error. Does not conform to IETF YANG hex string type";
  }
}

}  // namespace stratum
