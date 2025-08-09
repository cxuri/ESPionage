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
1. ESP32 clones a target Wi-Fi SSID as an **open network**.  
2. When a user connects, all traffic is redirected to the **captive portal page**.  
3. The portal displays a **"Free Bandwidth"** offer form requesting:
   - Name  
   - Mobile number  
4. Upon submission, the ESP32 logs:
   - MAC address (from connection request)  
   - Name (from form input)  
   - Mobile number (from form input)  

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

**Repo Description (for GitHub “About” section):**  
> Educational ESP32 captive portal that clones Wi-Fi networks, displays a fake free-bandwidth page, and logs MAC addresses, names, and phone numbers for analysis.
