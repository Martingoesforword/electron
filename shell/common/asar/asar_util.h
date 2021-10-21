// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ASAR_ASAR_UTIL_H_
#define SHELL_COMMON_ASAR_ASAR_UTIL_H_

#include <memory>
#include <string>

namespace base {
class FilePath;
}

namespace asar {

class Archive;
struct IntegrityPayload;

// 从路径获取或创建并缓存新档案。
std::shared_ptr<Archive> GetOrCreateAsarArchive(const base::FilePath& path);

// 销毁缓存的存档对象。
void ClearArchives();

// 分隔出存档的路径。
bool GetAsarArchivePath(const base::FilePath& full_path,
                        base::FilePath* asar_path,
                        base::FilePath* relative_path,
                        bool allow_root = false);

// 与base：：ReadFileToString相同，但支持asar存档。
bool ReadFileToString(const base::FilePath& path, std::string* contents);

void ValidateIntegrityOrDie(const char* data,
                            size_t size,
                            const IntegrityPayload& integrity);

}  // 命名空间asar。

#endif  // Shell_COMMON_ASAR_ASAR_UTIL_H_
