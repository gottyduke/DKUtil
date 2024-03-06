# Macros

| Macro | Level                                         |
| ----- | --------------------------------------------- |
| TRACE | `trace`, enabled on debug builds              |
| DEBUG | `debug`, enabled on debug builds              |
| INFO  | `info`, enabled on all builds                 |
| WARN  | `warn`, enabled on all builds                 |
| ERROR | `err`, message box pop-up, returnable         |
| FATAL | `critical`, message box pop-up, halts process |

## Logging Level

To change the current logging level, use `Logger::SetLevel`:

```cpp
DKUtil::Logger::SetLevel(spdlog::level::level_enums);
```

Convenient switches for disabling/enabling all DEBUG macros:

```cpp
ENABLE_DEBUG
// debug logs from here will be printed
DISABLE_DEBUG
// debug logs from here will be omitted
```

## Formatting

`DKUtil::Logger` uses `spdlog` as backend, thus powered by `fmt`, for detailed syntax, refer to [fmtlib syntax](https://fmt.dev/latest/syntax.html).

```cpp
INFO("{} - {} = {}", 10, 5, 10 - 5);
INFO("addr: {:X}", &pointer);
```
