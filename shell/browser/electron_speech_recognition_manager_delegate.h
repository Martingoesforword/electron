// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_ELECTRON_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/browser/speech_recognition_manager_delegate.h"

namespace electron {

class ElectronSpeechRecognitionManagerDelegate
    : public content::SpeechRecognitionManagerDelegate,
      public content::SpeechRecognitionEventListener {
 public:
  ElectronSpeechRecognitionManagerDelegate();
  ~ElectronSpeechRecognitionManagerDelegate() override;

  // 内容：：SpeechRecognitionEventListener：
  void OnRecognitionStart(int session_id) override;
  void OnAudioStart(int session_id) override;
  void OnEnvironmentEstimationComplete(int session_id) override;
  void OnSoundStart(int session_id) override;
  void OnSoundEnd(int session_id) override;
  void OnAudioEnd(int session_id) override;
  void OnRecognitionEnd(int session_id) override;
  void OnRecognitionResults(
      int session_id,
      const std::vector<blink::mojom::SpeechRecognitionResultPtr>&) override;
  void OnRecognitionError(
      int session_id,
      const blink::mojom::SpeechRecognitionError& error) override;
  void OnAudioLevelsChange(int session_id,
                           float volume,
                           float noise_volume) override;

  // 内容：：SpeechRecognitionManagerDelegate：
  void CheckRecognitionIsAllowed(
      int session_id,
      base::OnceCallback<void(bool ask_user, bool is_allowed)> callback)
      override;
  content::SpeechRecognitionEventListener* GetEventListener() override;
  bool FilterProfanities(int render_process_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronSpeechRecognitionManagerDelegate);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
