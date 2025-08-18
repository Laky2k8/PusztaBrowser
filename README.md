<img width="1298" height="287" alt="image" src="https://github.com/user-attachments/assets/d9f6bc4c-9971-4d0e-b088-ddf002e0993c" />

# Puszta Browser
<br>
Website: https://puszta.laky2k8.hu/
<br>
<br>

A couple months ago, I started wondering *"How do these web browsers we use every day even work?"*. <br>
Then I stumbled upon [this wonderful tutorial](https://browser.engineering/), and figured I could give it a shot!
Ofcourse me being me, I didn't just follow the tutorial like I should've - No siree! Instead I decided to follow it in C++ via GLFW and FreeType for text rendering and OpenSSL for... well, SSL. 
<br> This was a great mistake (C++ is *not* a fun language), but in the end I managed to create this:
<br>A fully functioning DIY web browser/HTML renderer! And it runs on Windows 7 - Not even Chrome or Firefox can do that anymore ;)

### But what amazing features does this browser have?
I'm glad you asked! This browser has cutting edge features like:
- HTTP 1.1 support
- HTTPS support (with full SSL!)
- Redirects
- Browser history (we don't save it though - your secrets are safe with us 😉)
- Full Unicode support!
- Variable fonts
- Local file loading (file:///)
- view-source
- An HTML renderer capable of rendering most webpages
- A modern and stylish UI with dark AND light mode via ImGui

Check out the rest of the wonderful features on [the Puszta Browser website](https://puszta.laky2k8.hu/).
<br>
<br>
# How to build
<br>
Set up GLFW, GLAD, ImGui, FreeTyoe and OpenSSL in any development environment you like. Then just build the project!
<br>

**WARNING: I only tested the code on Windows. I can not guarantee that it will build on Linux or Mac OS.**
<br>
<br>
# How to run
<br>
Just download the release zip, unzip it and run <code>PusztaBrowser.exe</code>. <br>
You can customize the font used by replacing <code>Regular.ttf</code> and <code>Italic.ttf</code>.
<br>
<br>
Here is it running on some platforms: <br><br>
<img width="1282" height="759" alt="image" src="https://github.com/user-attachments/assets/19465304-bae4-497e-acfb-6da4798cce49" /><br>
Windows 10/11
<br>
<br>
<img width="1915" height="1077" alt="image" src="https://github.com/user-attachments/assets/b953611d-298a-4880-816b-4d230fed1491" /><br>
Windows 7
<br>
<br>
<img width="2557" height="1302" alt="image" src="https://github.com/user-attachments/assets/6ebd4c92-7288-42da-b26a-abf9e1a3a2d9" /><br>
Linux Mint (via Wine, no native Linux build yet, coming soon)





