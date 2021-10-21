// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_safe_storage.h"

#include <string>
#include <vector>

#include "components/os_crypt/os_crypt.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/platform_util.h"

namespace electron {

namespace safestorage {

static const char* kEncryptionVersionPrefixV10 = "v10";
static const char* kEncryptionVersionPrefixV11 = "v11";

#if DCHECK_IS_ON()
static bool electron_crypto_ready = false;

void SetElectronCryptoReady(bool ready) {
  electron_crypto_ready = ready;
}
#endif

bool IsEncryptionAvailable() {
  return OSCrypt::IsEncryptionAvailable();
}

v8::Local<v8::Value> EncryptString(v8::Isolate* isolate,
                                   const std::string& plaintext) {
  if (!OSCrypt::IsEncryptionAvailable()) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString. "
        "Encryption is not available.");
    return v8::Local<v8::Value>();
  }

  std::string ciphertext;
  bool encrypted = OSCrypt::EncryptString(plaintext, &ciphertext);

  if (!encrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while encrypting the text provided to "
        "safeStorage.encryptString.");
    return v8::Local<v8::Value>();
  }

  return node::Buffer::Copy(isolate, ciphertext.c_str(), ciphertext.size())
      .ToLocalChecked();
}

std::string DecryptString(v8::Isolate* isolate, v8::Local<v8::Value> buffer) {
  if (!OSCrypt::IsEncryptionAvailable()) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString. "
        "Decryption is not available.");
    return "";
  }

  if (!node::Buffer::HasInstance(buffer)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Expected the first argument of decryptString() to be a buffer");
    return "";
  }

  // 确保在Mac或Linux上抛出错误。
  // 解密失败，而不是静默失败。
  const char* data = node::Buffer::Data(buffer);
  auto size = node::Buffer::Length(buffer);
  std::string ciphertext(data, size);
  if (ciphertext.empty()) {
    return "";
  }

  if (ciphertext.find(kEncryptionVersionPrefixV10) != 0 &&
      ciphertext.find(kEncryptionVersionPrefixV11) != 0) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString. "
        "Ciphertext does not appear to be encrypted.");
    return "";
  }

  std::string plaintext;
  bool decrypted = OSCrypt::DecryptString(ciphertext, &plaintext);
  if (!decrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString.");
    return "";
  }
  return plaintext;
}

}  // 命名空间安全存储。

}  // 命名空间电子

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("isEncryptionAvailable",
                 &electron::safestorage::IsEncryptionAvailable);
  dict.SetMethod("encryptString", &electron::safestorage::EncryptString);
  dict.SetMethod("decryptString", &electron::safestorage::DecryptString);
}

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_safe_storage, Initialize)
