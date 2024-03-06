# File Helpers

`DKUtil::Config` provides some helper functions for files:

+ To get full path of config file, relative or not:
```cpp
std::string GetPath(const std::string_view a_file);
```

+ To list all files fulfilling the criterion:
```cpp
template <bool RECURSIVE = false>
std::vector<std::string> GetAllFiles(
    std::string_view a_dir = {}, 
    std::string_view a_ext = {}, 
    std::string_view a_prefix = {}, 
    std::string_view a_suffix = {});
```