// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_MOUSE_UTIL_H_
#define SHELL_COMMON_MOUSE_UTIL_H_

#include <string>
#include "content/common/cursors/webcursor.h"
#include "ipc/ipc_message_macros.h"

// 类似于铬源中已有的IPC宏。
// 我们需要它们来监听光标更改IPC消息，同时仍然。
// 让Chrome通过设置HANDLED=FALSE来处理实际的光标更改。
#define IPC_MESSAGE_HANDLER_CODE(msg_class, member_func, code) \
  IPC_MESSAGE_FORWARD_CODE(msg_class, this,                    \
                           _IpcMessageHandlerClass::member_func, code)

#define IPC_MESSAGE_FORWARD_CODE(msg_class, obj, member_func, code) \
  case msg_class::ID: {                                             \
    if (!msg_class::Dispatch(&ipc_message__, obj, this, param__,    \
                             &member_func))                         \
      ipc_message__.set_dispatch_error();                           \
    code;                                                           \
  } break;

namespace electron {

// 以字符串形式返回游标的类型。
std::string CursorTypeToString(const ui::Cursor& cursor);

}  // 命名空间电子。

#endif  // SHELL_COMMON_MOOSE_UTIL_H_
