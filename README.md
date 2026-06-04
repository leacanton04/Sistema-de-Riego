Sistema automatico de riego con ESP32 y sensores
# 🌱 Monitor de Riego Inteligente con ESP32 (PFA_ESP32)

Un sistema de monitoreo y riego automatizado basado en un microcontrolador ESP32. Este proyecto recopila datos de humedad del suelo, temperatura y humedad ambiente, los muestra en una pantalla LCD local y levanta un **Servidor Web Asíncrono** interno. 

La interfaz web, accesible desde cualquier dispositivo en la misma red Wi-Fi, utiliza la librería **JustGage** para mostrar gráficos dinámicos y permite controlar la bomba de agua de forma remota.

---

## 🚀 Características Principales

* **Servidor Web Autónomo:** La página web (HTML/CSS/JS) está alojada en la memoria interna del ESP32 utilizando el sistema de archivos **LittleFS**.
* **Gráficos Dinámicos:** Visualización en vivo (actualización cada 2 segundos vía Fetch API) de los sensores sin recargar la página.
* **Control Remoto y Local:** La bomba de agua se puede accionar físicamente con un botón o remotamente desde el dashboard web.
* **Modo Automático/Manual:** Lógica de riego automático basada en la humedad ideal según la temperatura ambiente, o riego a demanda.
* **Seguridad Integrada:** Límite máximo de riego de 15 segundos y tiempo de enfriamiento de 1 minuto para evitar inundaciones o daños en la bomba.
* **Indicadores Físicos:** Pantalla LCD I2C y LED RGB para conocer el estado de la planta sin necesidad de conectarse al servidor.

---

## 🔌 Hardware y Conexiones

| Componente | Pin ESP32 | Descripción |
| :--- | :--- | :--- |
| **DHT11** | `GPIO 14` | Sensor de Temperatura y Humedad Ambiente |
| **Sensor de Suelo** | `GPIO 34` | Sensor de humedad de tierra (ADC) |
| **Bomba de Agua** | `GPIO 32` | Activación del relé de la bomba |
| **Botón Físico** | `GPIO 16` | Botón modo Auto/Manual (INPUT_PULLUP) |
| **LED RGB (Rojo)** | `GPIO 27` | Indicador visual |
| **LED RGB (Verde)**| `GPIO 26` | Indicador visual |
| **LED RGB (Azul)** | `GPIO 25` | Indicador visual |
| **Pantalla LCD** | `GPIO 21 (SDA) / 22 (SCL)` | Comunicación I2C |

---

## 📁 Estructura del Proyecto

Para que el sistema de archivos LittleFS funcione correctamente, el proyecto debe mantener esta estructura exacta:

```text
pfa_esp32/
├── data/
│   ├── index.html     # Estructura del dashboard y lógica JavaScript (Fetch y JustGage)
│   └── style.css      # Estilos de la interfaz web
└── pfa_esp32.ino      # Código principal en C++
