# ESP32 Captive Portal Logger

## 📌 Project Description
An **ESP32-based captive portal** that clones an existing Wi-Fi network (SSID) and creates an open-access point.  
When users connect, they are redirected to a portal promising free bandwidth in exchange for basic details.  
The system logs the device's:
- **MAC address**
- **Mobile number**
- **Name**

This project is intended for **educational and research purposes** to demonstrate captive portal behavior, SSID cloning, and basic logging techniques.

---

## ⚙️ Current Limitations
- 🚫 **No actual bandwidth delivery** — the "free internet" is only a prompt.  
- 🔄 **Single SSID cloning** — cannot mimic multiple networks simultaneously.  
- 📄 **Basic logging only** — no advanced analytics or cross-referencing.  

---

## 🛠️ Tech Stack
- **ESP32** microcontroller  
- Captive portal HTTP server  
- MAC address & form data logger  

---

## 📂 How It Works
1. ESP32 clones a target Wi-Fi SSID from:
   - The **strongest available nearby network**, or  
   - A **custom SSID set via serial commands**.  
2. Creates an **open network** with the cloned SSID.  
3. When a user connects, they are redirected to `/login` — the captive portal page.  
4. The portal displays a **"Free Bandwidth"** offer form requesting:
   - Name  
   - Mobile number  
5. Upon submission, the ESP32 logs:
   - MAC address (from connection request)  
   - Name (from form input)  
   - Mobile number (from form input)  

---

## 💻 Usage
1. **Flash the firmware** to your ESP32 using PlatformIO or Arduino IDE.  
2. After boot, the ESP32 will:
   - Automatically scan and clone the **strongest nearby Wi-Fi SSID**, OR  
   - Use a custom SSID if set via serial commands.  
3. Connect to the cloned SSID (open network).  
4. Open any webpage — you will be redirected to `192.168.4.1/login`.  
5. To access the **admin panel** for logs and settings, navigate to `192.168.4.1/admin`:  
   - **Username:** `admin`  
   - **Password:** `scamaware`  
6. Admin panel features:
   - View captured logs (MAC, name, phone)  
   - Clear stored data  
   - Change SSID target via serial or UI  
7. To stop the portal, simply power down the ESP32.

---

## 📜 Disclaimer
> ⚠️ **This project is for educational purposes only.**  
> Unauthorized collection or use of personal data may be illegal in your jurisdiction.  
> Always obtain permission before deploying in real environments.

---

## 🚀 Future Improvements
- Add multi-SSID cloning support  
- Implement secure storage for captured data  
- Integrate data visualization dashboard  
- Add optional real bandwidth delivery feature  

---
