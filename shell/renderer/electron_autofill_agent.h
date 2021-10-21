// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_ELECTRON_AUTOFILL_AGENT_H_
#define SHELL_RENDERER_ELECTRON_AUTOFILL_AGENT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/web_autofill_client.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_input_element.h"

namespace electron {

class AutofillAgent : public content::RenderFrameObserver,
                      public blink::WebAutofillClient,
                      public mojom::ElectronAutofillAgent {
 public:
  explicit AutofillAgent(content::RenderFrame* frame,
                         blink::AssociatedInterfaceRegistry* registry);
  ~AutofillAgent() override;

  void BindReceiver(
      mojo::PendingAssociatedReceiver<mojom::ElectronAutofillAgent> receiver);

  // 内容：：RenderFrameWatch：
  void OnDestruct() override;

  void DidChangeScrollOffset() override;
  void FocusedElementChanged(const blink::WebElement&) override;
  void DidCompleteFocusChangeInFrame() override;
  void DidReceiveLeftMouseDownOrGestureTapInNode(
      const blink::WebNode&) override;

 private:
  struct ShowSuggestionsOptions {
    ShowSuggestionsOptions();
    bool autofill_on_empty_values;
    bool requires_caret_at_end;
  };

  // BLINK：：WebAutofulClient：
  void TextFieldDidEndEditing(const blink::WebInputElement&) override;
  void TextFieldDidChange(const blink::WebFormControlElement&) override;
  void TextFieldDidChangeImpl(const blink::WebFormControlElement&);
  void TextFieldDidReceiveKeyDown(const blink::WebInputElement&,
                                  const blink::WebKeyboardEvent&) override;
  void OpenTextDataListChooser(const blink::WebInputElement&) override;
  void DataListOptionsChanged(const blink::WebInputElement&) override;

  // Mojom：：电子自动填充剂。
  void AcceptDataListSuggestion(const std::u16string& suggestion) override;

  bool IsUserGesture() const;
  void HidePopup();
  void ShowPopup(const blink::WebFormControlElement&,
                 const std::vector<std::u16string>&,
                 const std::vector<std::u16string>&);
  void ShowSuggestions(const blink::WebFormControlElement& element,
                       const ShowSuggestionsOptions& options);

  void DoFocusChangeComplete();

  const mojo::AssociatedRemote<mojom::ElectronAutofillDriver>&
  GetAutofillDriver();
  mojo::AssociatedRemote<mojom::ElectronAutofillDriver> autofill_driver_;

  // 上次单击焦点节点时为True。
  bool focused_node_was_last_clicked_ = false;

  // 当焦点改变时，它被设置为False，然后很快又被设置回True。
  // 之后。这有助于跟踪事件是否在节点。
  // 是否已经聚焦，或者是否导致焦点改变。
  bool was_focused_before_now_ = false;

  mojo::AssociatedReceiver<mojom::ElectronAutofillAgent> receiver_{this};

  base::WeakPtrFactory<AutofillAgent> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(AutofillAgent);
};

}  // 命名空间电子。

#endif  // 外壳渲染器电子自动填充代理H_
