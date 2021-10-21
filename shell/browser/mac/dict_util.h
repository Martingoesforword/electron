// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MAC_DICT_UTIL_H_
#define SHELL_BROWSER_MAC_DICT_UTIL_H_

#import <Foundation/Foundation.h>

namespace base {
class ListValue;
class DictionaryValue;
}  // 命名空间库。

namespace electron {

NSArray* ListValueToNSArray(const base::ListValue& value);
base::ListValue NSArrayToListValue(NSArray* arr);
NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value);
base::DictionaryValue NSDictionaryToDictionaryValue(NSDictionary* dict);

}  // 命名空间电子。

#endif  // Shell_Browser_MAC_DICT_UTIL_H_
