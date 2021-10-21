// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_INCLUDES_H_
#define SHELL_COMMON_NODE_INCLUDES_H_

// 包括用于使用节点API的公共头部。

#ifdef NODE_SHARED_MODE
#define BUILDING_NODE_EXTENSION
#endif

#undef debug_string    // 这在AssertMacros.h的MacOS SDK中定义。
#undef require_string  // 这在AssertMacros.h的MacOS SDK中定义。

#include "electron/push_and_undef_node_defines.h"

#include "env-inl.h"
#include "env.h"
#include "node.h"
#include "node_buffer.h"
#include "node_errors.h"
#include "node_internals.h"
#include "node_native_module_env.h"
#include "node_options-inl.h"
#include "node_options.h"
#include "node_platform.h"
#include "tracing/agent.h"

#include "electron/pop_node_defines.h"

// NODE_MODULE_CONTEXT_AWARE_X的替代。
// 允许显式注册内置模块，而不是使用。
// __ATTRIBUTE__((构造函数))。
#define NODE_LINKED_MODULE_CONTEXT_AWARE(modname, regfunc) \
  NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, nullptr, NM_F_LINKED)

#endif  // Shell_COMMON_NODE_INCLUES_H_
