# trace模块

## 简介

trace模块提供了抓取trace文件的功能。trace文件能够捕获并存储init进程在运行时的详细信息，这些信息在调试、性能分析、问题排查和系统监控中非常有用。

## 启用trace功能

要启用trace文件的抓取功能，需要设置系统变量persist.init.trace.enabled的值为1。当该变量设置为1时，设备在启动阶段会自动抓取trace文件，直到系统启动完成，init进程的信息抓取完毕。

## Init trace 与 Boot trace 的关系

- **Init trace**：由 init 内的 **`init_trace` 插件**按 **`/system/etc/init_trace.cfg`** 配置，直接操作 **tracefs**（buffer、时钟、内核/用户态 tag 等），将数据落盘到 **`/data/service/el0/startup/init/init_trace.log`**（或压缩 **`init_trace.zip`**）。总开关为 **`persist.init.trace.enabled=1`**，通过 **`init_trace start` / `stop` / `1`**（中断）等命令驱动。
- **Boot trace**：由 init **`fork` 子进程**执行 **`/system/bin/hitrace boot-trace`**，属于 **hitrace** 自带的启动阶段采集能力；次数与状态由 **`persist.hitrace.boot_trace.count`**（1～100 有效）与 **`debug.hitrace.boot_trace.active`** 控制。配置与默认输出位于 **`/data/log/hitrace/`**（例如 **`boot_trace.cfg`**、**`boot_trace_default.sys`**）。
- **互斥策略**：当 **`persist.init.trace.enabled=1`**（Init trace 总开关打开）时，**不执行任何 Boot trace 相关逻辑**（不读改 count/active、不 `fork`、不拉起 `hitrace boot-trace`），避免两套能力同时占用 ftrace/参数路径。仅当 Init trace 总开关**未**打开时，才根据 **`persist.hitrace.boot_trace.count`** 等在 **`INIT_POST_PERSIST_PARAM_LOAD`** 或 **`init_trace boot_trace`** 命令路径下尝试 Boot trace。

## 获取trace文件

系统启动完成后，trace文件会生成并存储在目录/data/service/el0/startup/init下，文件名为init_trace.log。开发者可以通过以下命令将trace文件从设备下载到本地电脑：
```bash
hdc file recv /data/service/el0/startup/init/init_trace.log D:\xxxxxx
```
其中，D:\xxxxxx是文件下载到电脑的目标路径。

## trace文件的分析

获取trace文件后，可以使用华为开发者联盟提供的工具进行打开与分析。具体使用方法可以参考以下链接：
[trace分析工具使用总结](https://tinyurl.com/fb7hbasw)。
通过分析trace文件，开发者可以深入了解init进程的运行情况，从而进行性能优化和问题排查。
