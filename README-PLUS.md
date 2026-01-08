# FOSSA pour WT32-SC01 PLUS

Ce document décrit le portage de FOSSA sur le WT32-SC01 PLUS (ESP32-S3).

## Différences matérielles principales

| Caractéristique | WT32-SC01 (original) | WT32-SC01 PLUS |
|-----------------|---------------------|----------------|
| MCU | ESP32 dual-core | ESP32-S3 |
| Interface écran | SPI | Parallèle 8-bit |
| Écran tactile | Résistif (XPT2046) | Capacitif (FT6336) |
| Communication tactile | GPIO direct | I2C (SDA: GPIO 6, SCL: GPIO 5) |

## Modifications apportées

### 1. Configuration de l'écran (tft_config_plus.txt)
- Interface parallèle 8-bit au lieu de SPI
- Nouveaux pins pour les données parallèles (D0-D7)
- Pins de contrôle adaptés à l'ESP32-S3

### 2. Pins GPIO remappés (fossa-plus/fossa.ino)

Les pins utilisés pour les accepteurs et l'imprimante ont été remappés vers le connecteur EXT :

| Fonction | WT32-SC01 original | WT32-SC01 PLUS (EXT) |
|----------|-------------------|----------------------|
| BILL_RX | GPIO 32 | GPIO 10 (EXT_IO1) |
| BILL_TX | GPIO 33 | GPIO 11 (EXT_IO2) |
| COIN_TX | GPIO 4 | GPIO 12 (EXT_IO3) |
| COIN_INHIBIT | GPIO 2 | GPIO 13 (EXT_IO4) |
| PRINTER_RX | GPIO 22 | GPIO 14 (EXT_IO5) |
| PRINTER_TX | GPIO 23 | GPIO 21 (EXT_IO6) |

**IMPORTANT**: Vous devez connecter vos accepteurs et imprimante aux pins du connecteur EXT du WT32-SC01 PLUS.

### 3. Tactile capacitif
- Bibliothèque bb_captouch utilisée pour gérer le FT6336
- Communication I2C sur GPIO 5 (SCL) et GPIO 6 (SDA)
- Fonction `touchWasReleased()` remplace les appels à `BTNA.wasReleased()`

### 4. Build
- FQBN changé à `esp32:esp32:esp32s3`
- TFT_eSPI mis à jour vers 2.5.51 pour support ESP32-S3
- Nouveau script de build : `build-plus.sh`

## Compilation

### Locale

```bash
chmod +x build-plus.sh
./build-plus.sh
```

### Flash

```bash
chmod +x debug.sh
./debug.sh /dev/ttyUSB0
```

## Web Installer

Le fichier `versions.json` a été mis à jour pour inclure le device "esp32s3". Le web installer pourra flasher les deux versions selon le board sélectionné.

## Pins disponibles sur le connecteur EXT

Le WT32-SC01 PLUS expose 6 GPIO via le connecteur EXT :
- EXT_IO1: GPIO 10
- EXT_IO2: GPIO 11
- EXT_IO3: GPIO 12
- EXT_IO4: GPIO 13
- EXT_IO5: GPIO 14
- EXT_IO6: GPIO 21

## Schéma de câblage

Le câblage pour les accepteurs de billets/pièces et l'imprimante doit maintenant utiliser les pins du connecteur EXT au lieu des GPIO directs de l'ESP32.

**Connecteur bill acceptor** :
- TX → EXT_IO2 (GPIO 11)
- RX → EXT_IO1 (GPIO 10)
- 12V+ → Terminal LIVE
- GND → Terminal GROUND

**Connecteur coin acceptor** :
- Serial Out → EXT_IO3 (GPIO 12)
- Interrupt → EXT_IO4 (GPIO 13)
- 12V → Terminal LIVE
- GND → Terminal GROUND

**Imprimante thermique** :
- RX → EXT_IO5 (GPIO 14)
- TX → EXT_IO6 (GPIO 21)

## Sources et références

- [TFT_eSPI ESP32-S3 support](https://github.com/Bodmer/TFT_eSPI/discussions/2319)
- [WT32-SC01 PLUS pinout](https://doc.riot-os.org/group__boards__esp32s3__wt32__sc01__plus.html)
- [bb_captouch library](https://github.com/bitbank2/bb_captouch)
