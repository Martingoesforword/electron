// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_API_ELECTRON_API_SPELL_CHECK_CLIENT_H_
#define SHELL_RENDERER_API_ELECTRON_API_SPELL_CHECK_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/spellcheck/renderer/spellcheck_worditerator.h"
#include "third_party/blink/public/platform/web_spell_check_panel_host_client.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_text_check_client.h"
#include "v8/include/v8.h"

namespace blink {
struct WebTextCheckingResult;
class WebTextCheckingCompletion;
}  // 命名空间闪烁。

namespace electron {

namespace api {

class SpellCheckClient : public blink::WebSpellCheckPanelHostClient,
                         public blink::WebTextCheckClient,
                         public base::SupportsWeakPtr<SpellCheckClient> {
 public:
  SpellCheckClient(const std::string& language,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> provider);
  ~SpellCheckClient() override;

 private:
  class SpellcheckRequest;
  // BLINK：：WebTextCheckClient：
  void RequestCheckingOfText(const blink::WebString& textToCheck,
                             std::unique_ptr<blink::WebTextCheckingCompletion>
                                 completionCallback) override;
  bool IsSpellCheckingEnabled() const override;

  // Blink：：WebSpellCheckPanelHostClient：
  void ShowSpellingUI(bool show) override;
  bool IsShowingSpellingUI() override;
  void UpdateSpellingUIWithMisspelledWord(
      const blink::WebString& word) override;

  struct SpellCheckScope {
    v8::HandleScope handle_scope_;
    v8::Context::Scope context_scope_;
    v8::Local<v8::Object> provider_;
    v8::Local<v8::Function> spell_check_;

    explicit SpellCheckScope(const SpellCheckClient& client);
    ~SpellCheckScope();
  };

  // 运行单词迭代器并发出请求。
  // 添加到JS API，用于检查当前。
  // 请求。
  void SpellCheckText();

  // 调用JavaScript检查单词的拼写。
  // Javascript函数将回调OnSpellCheckDone。
  // 所有拼写错误的单词的结果。
  void SpellCheckWords(const SpellCheckScope& scope,
                       const std::set<std::u16string>& words);

  // 返回给定词是否为有效词的缩写。
  // (例如，“WORD：WORD”)。
  // 输出变量CONSIMIT_WORD将包含INSERNAL。
  // 缩写中的词语。
  bool IsContraction(const SpellCheckScope& scope,
                     const std::u16string& contraction,
                     std::vector<std::u16string>* contraction_words);

  // JS API的回调，它返回拼写错误的单词列表。
  void OnSpellCheckDone(const std::vector<std::u16string>& misspelled_words);

  // 表示用于筛选出。
  // 此拼写检查对象不支持。
  SpellcheckCharAttribute character_attributes_;

  // 表示此拼写检查器中使用的单词迭代器。文本迭代器|。
  // 将WebKit提供的文本拆分为单词、缩写或连接。
  // 字里行间。|contract_iterator_|用于拆分由以下项提取的连接单词。
  // |text_iterator_|转换为Word组件，这样我们就可以处理连接在一起的单词。
  // 只由正确的词作为正确的词组成。
  SpellcheckWordIterator text_iterator_;
  SpellcheckWordIterator contraction_iterator_;

  // 挂起的后台拼写检查请求的参数。
  // (当WebKit发送两个或更多请求时，我们会取消之前的。
  // 请求，因此我们不必使用矢量。)。
  std::unique_ptr<SpellcheckRequest> pending_request_param_;

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Object> provider_;
  v8::Global<v8::Function> spell_check_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckClient);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_RENDERER_API_ELECTRON_API_SPELL_CHECK_CLIENT_H_
