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
```

## ⚙️ Instalación y Configuración (Importante)

El entorno del ESP32 puede ser sensible a las versiones de las librerías. Para asegurar que el proyecto compile y se ejecute sin reiniciar la placa (evitando el error `tcp_alloc`), sigue estos pasos con precisión:

### 1. Configuración del Arduino IDE
1. Abre el Gestor de Placas en Arduino IDE.
2. Busca **`esp32 by Espressif Systems`**.
3. **⚠️ CRÍTICO:** Instala específicamente la **versión 2.0.17** de esp32 by Espressif Systems". 
   *(Nota: Las versiones 3.x.x causan incompatibilidad con el servidor web asíncrono, provocando errores de compilación con las funciones MD5 o un reinicio constante de la placa al conectarse al Wi-Fi).*

### 2. Librerías Requeridas
Instala las siguientes dependencias desde el Gestor de Librerías de Arduino:
* `DHT sensor library` (por Adafruit)
* `LiquidCrystal I2C`
* `ESPAsyncWebServer` (de me-no-dev o lacamera)
* `AsyncTCP` (de me-no-dev o lacamera)

### 3. Preparación del Sistema de Archivos (LittleFS)
Para poder alojar la página web HTML y CSS dentro del ESP32, es necesario instalar un plugin en Arduino IDE 2.x:
1. Descarga el archivo `.vsix` de la herramienta [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload/releases).
2. Crea una carpeta llamada `plugins` en la ruta de instalación oculta: `C:\Usuarios\<TuUsuario>\.arduinoIDE\plugins\`.
3. Pega el archivo `.vsix` descargado dentro de esa carpeta (sin descomprimir).
4. Reinicia el Arduino IDE por completo.

### 4. Subida del Código y la Interfaz Web
Existe un bug conocido en Arduino IDE 2.x donde el plugin LittleFS no detecta el puerto COM asignado, arrojando el error *"No port specified"*. Para solucionarlo, realiza la subida en este orden estricto:

1. Configura tus credenciales de Wi-Fi en el archivo `.ino` (`ssid` y `password`).
2. Conecta el ESP32 por USB y **asegúrate de que el Monitor Serie esté cerrado**.
3. Sube el código `.ino` normalmente con el botón **Upload** (la flecha verde). Esto fuerza al IDE a registrar el puerto COM temporalmente.
4. Inmediatamente después de que termine la subida, presiona `F1` (o `Ctrl+Shift+P`), busca y ejecuta el comando **`>Upload LittleFS to Pico/ESP8266/ESP32`**. Esto transferirá la carpeta `data/` a la memoria interna de la placa.
