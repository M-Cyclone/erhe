#ifndef file_hpp_erhe_toolkit
#define file_hpp_erhe_toolkit

#include <filesystem>
#include <optional>
#include <string>

namespace erhe::toolkit
{

// return value will be empty if file does not exist, or is not regular file, or is empty
auto read(const std::filesystem::path& path)
-> std::optional<std::string>;

} // namespace erhe::toolkit

#endif
