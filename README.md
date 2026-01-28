# WebShell

WebShell is a basic, lightweight WebView2 Wrapper inspired by Tauri. This contains some basic api functions, along with a simple config. (NOT intended for production, just something I made for fun).

I'll try to update this as much as I can, it's not my biggest priority lol.

# How to use?

You can either download the source code or clone the repository. Click the `code` dropdown and you can recieve instructions on how to do either.

After installing and building the project (you need Visual Studio and C++ Build Tools), you can either run it directly (goes to bing.com) or you can run it with arguments.

# Arguments

- `-config=true` Tells the app to look for a config in the exe folder (This will be changed to a different, better approach soon)
- `-shadow=false` Removes the shadow on frameless windows. (It's usually not there, but it doesn't create it regardless).
- `-url=""` Tells the app what URL to visit. Example: `-url="http://127.0.0.1:5500"`

# API Usage

## Below is a simple HTML file that contains all the API features (as of now).

```
<body>
  <script>
    (function () {
      const queue = [];
      window.webshell = {
        minimize: () => window.webshell.send("min"),
        maximize: () => window.webshell.send("max"),
        close: () => window.webshell.send("close"),
        send: (m) => {
          if (window.chrome?.webview) {
            window.chrome.webview.postMessage(m);
          } else {
            console.warn("Not ready. Queuing message:", m);
            queue.push(m);
          }
        },
      };

      const check = setInterval(() => {
        if (window.chrome?.webview) {
          console.log("Active. Processing queue...");
          while (queue.length) window.webshell.send(queue.shift());
          clearInterval(check);
        }
      }, 100);

      window.addEventListener("mousedown", (e) => {
        if (e.target.closest("[data-webshell-drag-region]"))
          window.webshell.send("drag");
      });
    })();
  </script>
  <div
    style="
      height: 40px;
      background: #333;
      color: white;
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 0 10px;
    "
  >
    <span
      data-webshell-drag-region
      style="
        flex: 1;
        cursor: move;
        height: 100%;
        display: flex;
        align-items: center;
      "
    >
      WebShell
    </span>
    <div style="display: flex; gap: 10px">
      <button
        onclick="webshell.minimize()"
        style="
          cursor: pointer;
          background: #555;
          color: white;
          border: none;
          padding: 5px 10px;
        "
      >
        _
      </button>
      <button
        onclick="webshell.maximize()"
        style="
          cursor: pointer;
          background: #555;
          color: white;
          border: none;
          padding: 5px 10px;
        "
      >
        [ ]
      </button>
      <button
        onclick="webshell.close()"
        style="
          cursor: pointer;
          background: #c00;
          color: white;
          border: none;
          padding: 5px 10px;
        "
      >
        X
      </button>
    </div>
  </div>
</body>

```

# Configuration

If you are using the `-config=true` arg, make a file called `webshell_config.json` in the exe folder.

Here are all the options, aswell as an example config.

- `title`: The title of the window
- `width`: The width of the window
- `height`: The height of the window
- `frame`: Determines if WebShell should create a frame
- `resizable`: Determines if the window should be resizable
- `maximizable`: Determines if the window should be maximizable
- `fullscreen`: Determines if the window is fullscreen
- `devtools`: Determines if devtools can be used/opened
- `shadow`: Determines if a window shadow gets created

Below is an example configuration for the app. I'm planning on adding the URL here (so it's not needed in the args).

```
{
  "title": "My WebShell App",
  "width": 1380,
  "height": 720,
  "frame": false,
  "resizable": true,
  "maximizable": true,
  "fullscreen": false,
  "devtools": true,
  "shadow": true
}
```

# Contributions

If you would like to contribute, make a pull request!
