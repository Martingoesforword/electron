// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/api/electron_api_spell_check_client.h"

#include <memory>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/spellcheck/renderer/spellcheck_worditerator.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/function_template.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "third_party/blink/public/web/web_text_checking_completion.h"
#include "third_party/blink/public/web/web_text_checking_result.h"
#include "third_party/icu/source/common/unicode/uscript.h"

namespace electron {

namespace api {

namespace {

bool HasWordCharacters(const std::u16string& text, int index) {
  const char16_t* data = text.data();
  int length = text.length();
  while (index < length) {
    uint32_t code = 0;
    U16_NEXT(data, index, length, code);
    UErrorCode error = U_ZERO_ERROR;
    if (uscript_getScript(code, &error) != USCRIPT_COMMON)
      return true;
  }
  return false;
}

struct Word {
  blink::WebTextCheckingResult result;
  std::u16string text;
  std::vector<std::u16string> contraction_words;
};

}  // 命名空间。

class SpellCheckClient::SpellcheckRequest {
 public:
  SpellcheckRequest(
      const std::u16string& text,
      std::unique_ptr<blink::WebTextCheckingCompletion> completion)
      : text_(text), completion_(std::move(completion)) {}
  SpellcheckRequest(const SpellcheckRequest&) = delete;
  SpellcheckRequest& operator=(const SpellcheckRequest&) = delete;
  ~SpellcheckRequest() = default;

  const std::u16string& text() const { return text_; }
  blink::WebTextCheckingCompletion* completion() { return completion_.get(); }
  std::vector<Word>& wordlist() { return word_list_; }

 private:
  std::u16string text_;          // 要在此任务中检查的文本。
  std::vector<Word> word_list_;  // 在文本中找到的单词列表。
  // 将拼写错误的范围发送到WebKit的接口。
  std::unique_ptr<blink::WebTextCheckingCompletion> completion_;
};

SpellCheckClient::SpellCheckClient(const std::string& language,
                                   v8::Isolate* isolate,
                                   v8::Local<v8::Object> provider)
    : isolate_(isolate),
      context_(isolate, isolate->GetCurrentContext()),
      provider_(isolate, provider) {
  DCHECK(!context_.IsEmpty());

  character_attributes_.SetDefaultLanguage(language);

  // 持久化该方法。
  v8::Local<v8::Function> spell_check;
  gin_helper::Dictionary(isolate, provider).Get("spellCheck", &spell_check);
  spell_check_.Reset(isolate, spell_check);
}

SpellCheckClient::~SpellCheckClient() {
  context_.Reset();
}

void SpellCheckClient::RequestCheckingOfText(
    const blink::WebString& textToCheck,
    std::unique_ptr<blink::WebTextCheckingCompletion> completionCallback) {
  std::u16string text(textToCheck.Utf16());
  // 忽略无效请求。
  if (text.empty() || !HasWordCharacters(text, 0)) {
    completionCallback->DidCancelCheckingText();
    return;
  }

  // 在开始新请求之前清理上一个请求。
  if (pending_request_param_) {
    pending_request_param_->completion()->DidCancelCheckingText();
  }

  pending_request_param_ =
      std::make_unique<SpellcheckRequest>(text, std::move(completionCallback));

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SpellCheckClient::SpellCheckText, AsWeakPtr()));
}

bool SpellCheckClient::IsSpellCheckingEnabled() const {
  return true;
}

void SpellCheckClient::ShowSpellingUI(bool show) {}

bool SpellCheckClient::IsShowingSpellingUI() {
  return false;
}

void SpellCheckClient::UpdateSpellingUIWithMisspelledWord(
    const blink::WebString& word) {}

void SpellCheckClient::SpellCheckText() {
  const auto& text = pending_request_param_->text();
  if (text.empty() || spell_check_.IsEmpty()) {
    pending_request_param_->completion()->DidCancelCheckingText();
    pending_request_param_ = nullptr;
    return;
  }

  if (!text_iterator_.IsInitialized() &&
      !text_iterator_.Initialize(&character_attributes_, true)) {
    // 我们无法初始化text_iterator_，返回拼写正确。
    VLOG(1) << "Failed to initialize SpellcheckWordIterator";
    return;
  }

  if (!contraction_iterator_.IsInitialized() &&
      !contraction_iterator_.Initialize(&character_attributes_, false)) {
    // 我们无法初始化单词迭代器，返回拼写正确。
    VLOG(1) << "Failed to initialize contraction_iterator_";
    return;
  }

  text_iterator_.SetText(text.c_str(), text.size());

  SpellCheckScope scope(*this);
  std::u16string word;
  size_t word_start;
  size_t word_length;
  std::set<std::u16string> words;
  auto& word_list = pending_request_param_->wordlist();
  Word word_entry;
  for (;;) {  // 运行到文本末尾。
    const auto status =
        text_iterator_.GetNextWord(&word, &word_start, &word_length);
    if (status == SpellcheckWordIterator::IS_END_OF_TEXT)
      break;
    if (status == SpellcheckWordIterator::IS_SKIPPABLE)
      continue;

    word_entry.result.location = base::checked_cast<int>(word_start);
    word_entry.result.length = base::checked_cast<int>(word_length);
    word_entry.text = word;
    word_entry.contraction_words.clear();

    word_list.push_back(word_entry);
    words.insert(word);
    // 如果给定词是两个或多个有效词的串联词。
    // (例如，“hello：hello”)，我们应该将其视为有效单词。
    if (IsContraction(scope, word, &word_entry.contraction_words)) {
      for (const auto& w : word_entry.contraction_words) {
        words.insert(w);
      }
    }
  }

  // 将所有单词数据发送到拼写检查器进行检查。
  SpellCheckWords(scope, words);
}

void SpellCheckClient::OnSpellCheckDone(
    const std::vector<std::u16string>& misspelled_words) {
  std::vector<blink::WebTextCheckingResult> results;
  std::unordered_set<std::u16string> misspelled(misspelled_words.begin(),
                                                misspelled_words.end());

  auto& word_list = pending_request_param_->wordlist();

  for (const auto& word : word_list) {
    if (misspelled.find(word.text) != misspelled.end()) {
      // 如果这是一个缩写，遍历各个部分并接受这个词。
      // 如果它们都没有拼错。
      if (!word.contraction_words.empty()) {
        auto all_correct = true;
        for (const auto& contraction_word : word.contraction_words) {
          if (misspelled.find(contraction_word) != misspelled.end()) {
            all_correct = false;
            break;
          }
        }
        if (all_correct)
          continue;
      }
      results.push_back(word.result);
    }
  }
  pending_request_param_->completion()->DidFinishCheckingText(results);
  pending_request_param_ = nullptr;
}

void SpellCheckClient::SpellCheckWords(const SpellCheckScope& scope,
                                       const std::set<std::u16string>& words) {
  DCHECK(!scope.spell_check_.IsEmpty());

  gin_helper::MicrotasksScope microtasks_scope(
      isolate_, v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
      isolate_,
      base::BindRepeating(&SpellCheckClient::OnSpellCheckDone, AsWeakPtr()));

  auto context = isolate_->GetCurrentContext();
  v8::Local<v8::Value> args[] = {gin::ConvertToV8(isolate_, words),
                                 templ->GetFunction(context).ToLocalChecked()};
  // 使用单词和回调函数调用javascript。
  scope.spell_check_->Call(context, scope.provider_, 2, args).IsEmpty();
}

// 返回给定字符串是否为缩写。
// 此函数是SpellcheckWordIterator类。
// 返回不在所选词典中的连接单词。
// (例如“in‘n’out”)，但每个单词都是有效的。
// 输出变量CONSIMIT_WORD将包含INSERNAL。
// 缩写中的词语。
bool SpellCheckClient::IsContraction(
    const SpellCheckScope& scope,
    const std::u16string& contraction,
    std::vector<std::u16string>* contraction_words) {
  DCHECK(contraction_iterator_.IsInitialized());

  contraction_iterator_.SetText(contraction.c_str(), contraction.length());

  std::u16string word;
  size_t word_start;
  size_t word_length;
  for (auto status =
           contraction_iterator_.GetNextWord(&word, &word_start, &word_length);
       status != SpellcheckWordIterator::IS_END_OF_TEXT;
       status = contraction_iterator_.GetNextWord(&word, &word_start,
                                                  &word_length)) {
    if (status == SpellcheckWordIterator::IS_SKIPPABLE)
      continue;

    contraction_words->push_back(word);
  }
  return contraction_words->size() > 1;
}

SpellCheckClient::SpellCheckScope::SpellCheckScope(
    const SpellCheckClient& client)
    : handle_scope_(client.isolate_),
      context_scope_(
          v8::Local<v8::Context>::New(client.isolate_, client.context_)),
      provider_(v8::Local<v8::Object>::New(client.isolate_, client.provider_)),
      spell_check_(
          v8::Local<v8::Function>::New(client.isolate_, client.spell_check_)) {}

SpellCheckClient::SpellCheckScope::~SpellCheckScope() = default;

}  // 命名空间API。

}  // 命名空间电子
