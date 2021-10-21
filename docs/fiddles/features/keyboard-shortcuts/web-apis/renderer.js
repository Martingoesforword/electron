function handleKeyPress (event) {
  // 您可以在这里放置代码来处理按键。
  document.getElementById("last-keypress").innerText = event.key
  console.log(`You pressed ${event.key}`)
}

window.addEventListener('keyup', handleKeyPress, true)
